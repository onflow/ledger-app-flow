'use strict';

import { testStep, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs ,humanTime } from "./speculos-common.js";
import assert from 'assert/strict';
import pkg from 'elliptic';
const {ec: EC} = pkg;
import jsSHA from "jssha";

function getKeyPath(sigAlgo, hashAlgo) {
    const scheme = sigAlgo | hashAlgo;
    const path = `m/44'/539'/${scheme}'/0/0`;
    return path;
}

async function transactionTest(app, transport, device, txHexBlob, sigAlgo, hashAlgo, appVersion, givenPath = "") {
	let hexExpected = "";

    const path = givenPath ? givenPath : getKeyPath(sigAlgo.code, hashAlgo.code);

	//getPubkey
	testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path);
	const getPubkeyResponse = await app.getAddressAndPubKey(path);

	assert.equal(getPubkeyResponse.returnCode, 0x9000);
	assert.equal(getPubkeyResponse.errorMessage, "No errors");
	const pubkeyHex = getPubkeyResponse.publicKey.toString("hex")
	console.log(humanTime() + " publicKeyHex=" + pubkeyHex);
	
	compareGetVersionAPDUs(transport);
	if (appVersion <=12)  {
		hexExpected = "3301000014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"; 
		compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
	}
	else {
		hexExpected = "3301000016xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0000"; 
		compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
	}
	hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
	compareInAPDU(transport, hexExpected, "apdu response", {do_not_compare_publicKey:65, do_not_compare_publicKey_hex:130, returnCode:2, unexpected:9999});
	noMoreAPDUs(transport)

	//sign
	const txBlob = Buffer.from(txHexBlob, "hex");

	testStep(" - - -", "app.sign() // path=" + path + " txBlob=" + txBlob.length + ":" + txHexBlob.substring(0, 16));
	const signPromise =  app.sign(path, txBlob);
	//sign is multiAPDU operation. To help the snapshotter with synchronization we await last APDU beign sent
	await transport.waitForAPDU(0x33, 0x02, 0x02);	
    device.review("Show address 1 - empty slot");
	const signResponse = await signPromise;

	assert.equal(signResponse.returnCode, 0x9000);
	assert.equal(signResponse.errorMessage, "No errors");
	const signatureDERHex = signResponse.signatureDER.toString("hex");
	console.log(humanTime() + " signatureDERHex=" + signatureDERHex);

	compareGetVersionAPDUs(transport);
	//compare first APDU
	if (appVersion <=12)  {
		hexExpected = "33020000142c0000801b020080010200800000000000000000";
		compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
	}
	else {
		hexExpected = "33020000162c0000801b0200800102008000000000000000000000";
		compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
	}
	hexExpected = "9000";
	compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
	assert.equal(transport.hexApduCommandOut.length, transport.hexApduCommandIn.length);

    //compare other APDUs, let us calculate original txHexBlob
	let txHexFromAPDUs = "";
	const outLen = transport.hexApduCommandOut.length;
	for(let p = 0; p < outLen; p++) {
		const p1 = ((p + 1 == outLen) ? "02" : "01")
		const hexOutgoing = transport.hexApduCommandOut.shift();
        assert.equal(hexOutgoing.substring(0, 8), "3302"+p1+"00")
		const chunkLen = parseInt(hexOutgoing.substring(8, 10), 16)
		assert.equal(hexOutgoing.length, 10 + 2*chunkLen)
		txHexFromAPDUs = txHexFromAPDUs.concat(hexOutgoing.substring(10, 10 + 2*chunkLen))

		if (p1 == "01") { // not last APDU
			hexExpected = "9000";
			compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});	
		}
		if (p1 == "02") { // last APDU
			const returnCodeLen = 2;
			const signatureCompactLen = 65;
			const signatureDERLen = transport.hexApduCommandIn[0].length / 2 - signatureCompactLen - returnCodeLen;
			const hexExpected = "01".repeat(signatureCompactLen) + "02".repeat(signatureDERLen) + "9000";
			compareInAPDU(transport, hexExpected, "apdu response", {do_not_compare_signatureCompact:signatureCompactLen, do_not_compare_signatureDER:signatureDERLen, returnCode:returnCodeLen, unexpected:9999});
		}
	}
	//Verify that the APDU's contain the correct tx
	assert.equal(txHexFromAPDUs, txHexBlob);
	noMoreAPDUs(transport);

	// Prepare digest by hashing transaction
	testStep("   ?  ", "transaction signature verification against digest");
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-transaction");
	const hasher = new jsSHA(hashAlgo.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(txBlob);
	const digestHex = hasher.getHash("HEX");
	console.log(humanTime() + " digestHex=" + digestHex); // e.g. 841058f2f41b3e15b3add2eb7d6b588f443577efeaa0a31e5915e770208f6f20

	// Verify transaction signature against digest
	const ec = new EC(sigAlgo.name);
	const signatureOk = ec.verify(digestHex, signatureDERHex, pubkeyHex, 'hex');
	if (signatureOk) {
		console.log(humanTime() + " transaction signature verification against digest PASSED");
	} else {
		console.log(humanTime() + " transaction signature verification against digest FAILED");
		assert.ok(false)
	}
}
//End async function transactionTest(

export { transactionTest, getKeyPath };