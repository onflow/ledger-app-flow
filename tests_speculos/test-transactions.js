'use strict';

import { testStart, testStep, testEnd, testCombo, compareInAPDU, compareOutAPDU, noMoreAPDUs, getScriptName, getSpeculosDefaultConf, humanTime } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { ButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';
import pkg from 'elliptic';
const {ec: EC} = pkg;
import fs from "fs";
import jsSHA from "jssha";

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);

function getKeyPath(sigAlgo, hashAlgo) {
    const scheme = sigAlgo | hashAlgo;
    const path = `m/44'/539'/${scheme}'/0/0`;
    return path;
}

async function transactionTest(testTitle, transactionTitle, txHexBlob, sigAlgo, hashAlgo) {

	// e.g. test-transactions.js.basic-sign-transfer-flow-secp256k1-sha-256
	testCombo(scriptName + "; " + testTitle);
	const scriptNameCombo = (scriptName + "." + testTitle).replace(new RegExp("([:/ \-]+)","gm"),"-").toLowerCase(); 

	const device = new ButtonsAndSnapshots(scriptNameCombo, speculosConf);
	let hexExpected = "";

    await device.makeStartingScreenshot();

	//getPubkey
	const path = getKeyPath(sigAlgo.code, hashAlgo.code);

	testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path);
	const getPubkeyResponse = await app.getAddressAndPubKey(path);

	assert.equal(getPubkeyResponse.returnCode, 0x9000);
	assert.equal(getPubkeyResponse.errorMessage, "No errors");
	const pubkeyHex = getPubkeyResponse.publicKey.toString("hex")
	console.log(humanTime() + " publicKeyHex=" + pubkeyHex);
	
	hexExpected = "3301000014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
	compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
	hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
	compareInAPDU(transport, hexExpected, "apdu response", {do_not_compare_publicKey:65, do_not_compare_publicKey_hex:130, returnCode:2, unexpected:9999});
	noMoreAPDUs(transport)

	//sign
	const txBlob = Buffer.from(txHexBlob, "hex");

	testStep(" - - -", "app.sign() // path=" + path + " txBlob=" + txBlob.length + ":" + txHexBlob.substring(0, 16) + ".. AKA " + transactionTitle);
	const signPromise =  app.sign(path, txBlob);
	//sign is multiAPDU operation. To help the snapshotter with synchronization we await last APDU beign sent
	await transport.waitForAPDU(0x33, 0x02, 0x02);	
    device.review("Show address 1 - empty slot");
	const signResponse = await signPromise;

	assert.equal(signResponse.returnCode, 0x9000);
	assert.equal(signResponse.errorMessage, "No errors");
	const signatureDERHex = signResponse.signatureDER.toString("hex");
	console.log(humanTime() + " signatureDERHex=" + signatureDERHex);

	//compare first APDU
	hexExpected = "33020000142c0000801b020080010200800000000000000000";
	compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
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

//Now we test the transactions
const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256 };

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256};

const sigAlgos = [ECDSA_SECP256K1, ECDSA_P256];
const hashAlgos = [SHA2_256, SHA3_256];

const exampleTransferBlob      = "f9023ff9023bb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df861b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
const exampleCreateAccountBlob = "f90289f90261b8a97472616e73616374696f6e287075626c69634b6579733a205b537472696e675d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b65792e6465636f64654865782829290a7d0a7d0a7df90173b901707b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303331227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
const exampleAddKeyBlob        = "f90186f9015eb86e7472616e73616374696f6e287075626c69634b65793a20537472696e6729207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a7369676e65722e6164645075626c69634b6579287075626c69634b65792e6465636f64654865782829290a7d0a7df8acb8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"

{
	const transactions = [
		{
			title: "Transfer FLOW",
			blob: exampleTransferBlob,
		},
		{
			title: "Create Account",
			blob: exampleCreateAccountBlob,
		},
		{
			title: "Add Key",
			blob: exampleAddKeyBlob,
		},
	];

	for (let i=0; i < transactions.length; ++i ) {
		for (let j=0; j < sigAlgos.length; ++j ) {
			for (let k=0; k < hashAlgos.length; ++k ) {
				const testTitle = `basic sign: ${transactions[i].title} - ${sigAlgos[j].name} / ${hashAlgos[k].name}`; // e.g. basic sign: Transfer FLOW - secp256k1 / SHA-256
				await transactionTest(
					testTitle,
					transactions[i].title,
					transactions[i].blob, 
					sigAlgos[j], 
					hashAlgos[k],
				);
			}
		}
	}
}

// todo: consider adding mechanism to run subset of tests so that tests can be run in parallel on different speculos instances
{
	const transactions = JSON.parse(fs.readFileSync("../tests/testvectors/manifestEnvelopeCases.json"));

	for (var i=0; i < transactions.length; ++i ) {
		if (transactions[i].chainID == "Mainnet") {
			var testTitle = `staking sign: ${transactions[i].title} - ${ECDSA_P256.name} / ${SHA3_256.name}`; // e.g. staking sign: TH.01 - Withdraw Unlocked FLOW - p256 / SHA3-256
			await transactionTest(
				testTitle,
				transactions[i].title,
				transactions[i].encodedTransactionEnvelopeHex,
				ECDSA_P256,
				SHA3_256,
			);
		}
	}
}

await transport.close()
testEnd(scriptName);
