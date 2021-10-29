'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import jsSHA from "jssha";
import fs from "fs";

//import {ec as EC} from "elliptic"; // SyntaxError: Named export 'ec' not found. The requested module 'elliptic' is a CommonJS module, which may not support all module.exports as named exports.
import pkg from 'elliptic';
const {ec: EC} = pkg;

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

const CHAIN_ID_PAGE_COUNT = 1;
const REF_BLOCK_PAGE_COUNT = 2;
const GAS_LIMIT_PAGE_COUNT = 1;
const PROP_KEY_ADDRESS_PAGE_COUNT = 1;
const PROP_KEY_ID_PAGE_COUNT = 1;
const PROP_KEY_SEQNUM_PAGE_COUNT = 1;
const PAYER_PAGE_COUNT = 1;
const AUTHORIZER_PAGE_COUNT = 1;
const ACCEPT_PAGE_COUNT = 1;

const PAGE_SIZE = 34;

function getArgumentPageCount(arg) {
    if (arg.type == "Array") {
        return getArgumentsPageCount(arg.value);
    }

    if (arg.type == "String") {
        const count = Math.ceil(arg.value.length / PAGE_SIZE);
        return count;
    }

    if (arg.type == "Optional" && arg.value != null) {
        return getArgumentPageCount(arg.value);
    }

    return 1;
}

function getArgumentsPageCount(args) {
    return args.reduce((count, arg) => count + getArgumentPageCount(arg), 0);
}

function getTransactionPageCount(tx) {
    return (
        CHAIN_ID_PAGE_COUNT +
        REF_BLOCK_PAGE_COUNT +
        getArgumentsPageCount(tx.arguments) +
        GAS_LIMIT_PAGE_COUNT +
        PROP_KEY_ADDRESS_PAGE_COUNT +
        PROP_KEY_ID_PAGE_COUNT +
        PROP_KEY_SEQNUM_PAGE_COUNT +
        PAYER_PAGE_COUNT +
        (AUTHORIZER_PAGE_COUNT * tx.authorizers.length) +
        ACCEPT_PAGE_COUNT
    );
}

function getKeyPath(sigAlgo, hashAlgo) {
    const scheme = sigAlgo | hashAlgo;
    const path = `m/44'/539'/${scheme}'/0/0`;
    return path;
}

async function transactionTest(testTitle, transactionTitle, txHexBlob, txExpectedPageCount, sigAlgo, hashAlgo) {
	common.testCombo(scriptName + "; " + testTitle);

	var scriptNameCombo = (scriptName + "." + testTitle).replace(new RegExp("([:/ \-]+)","gm"),"-").toLowerCase(); // e.g. test-transactions.js.basic-sign-transfer-flow-secp256k1-sha-256

	const txBlob = Buffer.from(txHexBlob, "hex");

	const path = getKeyPath(sigAlgo.code, hashAlgo.code);
	const address = "e467b9dd11fa00df";

	console.log(common.humanTime() + " // screen shot before sending first apdu command");
	common.curlScreenShot(scriptNameCombo);

	/*
	await prepareSlot(sim, app, 1, address, path)
	*/
	const slot = 1
	common.testStep(" - - -", "await app.setSlot() // slot=" + slot  + " address=" + address + " path=" + path);
	await app.setSlot(slot, address, path);
	var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
	var hexExpected = "331200001d01e467b9dd11fa00df2c0000801b020080010200800000000000000000";
	common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, do_not_compare_slotBytes:28, unexpected:9999});
	common.testStep(" >    ", "APDU out");
	common.asyncCurlApduSend(hexOutgoing);
	common.testStep("   +  ", "buttons");
	common.curlScreenShot(scriptNameCombo); common.curlButton('right', "; navigate the address / path; Set Account 1");
	common.curlScreenShot(scriptNameCombo); common.curlButton('right', "; navigate the address / path; Account e467..");
	common.curlScreenShot(scriptNameCombo); common.curlButton('right', "; navigate the address / path; Path 44'/..");
	common.curlScreenShot(scriptNameCombo); common.curlButton('both', "; confirm; Approve");
	common.curlScreenShot(scriptNameCombo); console.log(common.humanTime() + " // back to main screen");
	common.testStep("     <", "APDU in");
	var hexResponse = await common.curlApduResponseWait();
	var hexExpected = "9000";
	common.compare(hexResponse, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

	/*
	const pkResponse = await app.getAddressAndPubKey(1);
	expect(pkResponse.returnCode).toEqual(0x9000);
	expect(pkResponse.errorMessage).toEqual("No errors");
	*/

	common.testStep(" - - -", "await app.getAddressAndPubKey() // slot=" + slot);
	await app.getAddressAndPubKey(slot);
	var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
	var hexExpected = "330100000101";
	common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, unexpected:9999});
	common.testStep(" >    ", "APDU out");
	common.asyncCurlApduSend(hexExpected);
	common.testStep("     <", "APDU in");
	var hexResponse = await common.curlApduResponseWait();
	var hexEXpected = "e467b9dd11fa00df04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be9000";
	common.compare(hexResponse, hexEXpected, "apdu response", {address:8, do_not_compare_publicKey:65, returnCode:2, unexpected:9999});

	const publicKeyHex = hexResponse.substring((8*2), (8*2)+(65*2));
	console.log(common.humanTime() + " extracted: publicKeyHex=" + publicKeyHex);

	/*
	// WARNING: do not block for this request until transaction
	// has been accepted in the Zemu emulator
	const signatureRequest = app.sign(path, txBlob);
	*/

	common.testStep(" - - -", "await app.sign() // path=" + path + " txBlob=" + txBlob.length + ":" + txHexBlob.substring(0, 16) + ".. AKA " + transactionTitle);
	await app.sign(path, txBlob);

	var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
	var hexExpected = "33020000142c0000801b020080010200800000000000000000";
	common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
	common.testStep(" >    ", "APDU out");
	common.asyncCurlApduSend(hexOutgoing);
	common.testStep("     <", "APDU in");
	var hexResponse = await common.curlApduResponseWait();
	var hexEXpected = "9000";
	common.compare(hexResponse, hexEXpected, "apdu response", {returnCode:2, unexpected:9999});

	// split message to sign into 250 byte chunks and send them, and don't wait for the response for the last chunk
	const maxHexMessagePartChars = 500;
	var p = 0;
	while (p < txHexBlob.length) {
		var lenHex = txHexBlob.length - p;
		var p1;
		if (lenHex > maxHexMessagePartChars) {
			lenHex = maxHexMessagePartChars;
			p1 = "01";
		} else {
			p1 = "02";
		}
		var lenBytes = (lenHex / 2);
		var lenBytesAsPaddedHex = ('0' + (lenBytes & 0xFF).toString(16)).slice(-2); // convert 0 to 255 value to padded hex e.g. 0x00 to 0xff

		common.testStep("   *  ", "APDU (chunk) sanity");
		var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
		var hexExpected = "3302" + p1 + "00" + lenBytesAsPaddedHex + txHexBlob.substring(p, p + lenHex);
		common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, messagePart:lenBytes, unexpected:9999});
		common.testStep(" >    ", "APDU (chunk) out");
		common.asyncCurlApduSend(hexOutgoing);

		if (p1 == "01") {
			common.testStep("     <", "APDU (chunk) in");
			var hexResponse = await common.curlApduResponseWait();
			var hexEXpected = "9000";
			common.compare(hexResponse, hexEXpected, "apdu response", {returnCode:2, unexpected:9999});
		}

		p += lenHex;
	}

	/*
	// Click through each approval page and accept transaction
	// Capture a snapshot of each page
	const snapshots = await verifyAndAccept(sim, txExpectedPageCount);

	// Expect new snapshots to match saved snapshots
	snapshots.forEach((image) => expect(image).toMatchImageSnapshot());
	*/

	// Click through each approval page and accept transaction
	// Capture a snapshot of each page
	common.testStep("   +  ", "buttons");
	for (var right=0; right < txExpectedPageCount; ++right ) {
		// e.g.  1 "Type Token Transfer >"
		// e.g.  2 "Chain ID Emulator >"
		// e.g.  3 "Amount 184467440737.9.. >"
		// e.g.  4 "Destination 0xf8de.. >"
		// e.g.  5 "Ref Block [1/2] f0e4.. >"
		// e.g.  6 "Ref Block [2/2] 14d4.. >"
		// e.g.  7 "Gas Limit 42 >"
		// e.g.  8 "Prop Key Addr f8d6.. >"
		// e.g.  9 "Prop Key Id 4 >"
		// e.g. 10 "Prop Key Seq Num 10 >"
		// e.g. 11 "Payer f8d6.. >"
		// e.g. 12 "Authorizer 1 f8d6.. >"
		common.curlScreenShot(scriptNameCombo); common.curlButton('right', "");
	}
	common.curlScreenShot(scriptNameCombo); common.curlButton('both', "; confirm; Approve");

	/*
	let resp = await signatureRequest;
	expect(resp.returnCode).toEqual(0x9000);
	expect(resp.errorMessage).toEqual("No errors");
	*/

	common.testStep("     <", "APDU (chunk) in (last)");
	var hexResponse = await common.curlApduResponseWait();
	var returnCodeLen = 2;
	var signatureCompactLen = 65;
	var signatureDERLen = (hexResponse.length/2) - signatureCompactLen - returnCodeLen; // make this part variable length; just like the javascript does
	var hexEXpected = "01".repeat(signatureCompactLen) + "02".repeat(signatureDERLen) + "9000";
	common.compare(hexResponse, hexEXpected, "apdu response", {do_not_compare_signatureCompact:signatureCompactLen, do_not_compare_signatureDER:signatureDERLen, returnCode:returnCodeLen, unexpected:9999});

	// todo: consider getting common.compare() returning a hash of keys instead of using .substring below
	var signatureDERHex = hexResponse.substring((signatureCompactLen*2), (signatureCompactLen*2)+(signatureDERLen*2));
	console.log(common.humanTime() + " extracted: signatureDERHex=" + signatureDERHex);

	/*
	// Prepare digest by hashing transaction
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-transaction");
	const hasher = new jsSHA(hashAlgo.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(txBlob);
	const digest = hasher.getHash("HEX");

	// Verify transaction signature against the digest
	const ec = new EC(sigAlgo.name);
	const sig = resp.signatureDER.toString("hex");
	const pk = pkResponse.publicKey.toString("hex");
	const signatureOk = ec.verify(digest, sig, pk, 'hex');
	expect(signatureOk).toEqual(true);
	*/

	common.testStep("   ?  ", "transaction signature verification against digest");

	// Prepare digest by hashing transaction
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-transaction");
	const hasher = new jsSHA(hashAlgo.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(txBlob);
	const digestHex = hasher.getHash("HEX");
	console.log(common.humanTime() + " digestHex=" + digestHex); // e.g. 841058f2f41b3e15b3add2eb7d6b588f443577efeaa0a31e5915e770208f6f20

	// Verify transaction signature against digest
	const ec = new EC(sigAlgo.name);
	const signatureOk = ec.verify(digestHex, signatureDERHex, publicKeyHex, 'hex');
	if (signatureOk) {
		console.log(common.humanTime() + " transaction signature verification against digest PASSED");
	} else {
		console.log(common.humanTime() + " transaction signature verification against digest FAILED");
		throw new Error();
	}

	const expectedAccountDelete = "0000000000000000";
	const expectedPathDelete = `m/0/0/0/0/0`;
	common.testStep(" - - -", "await app.setSlot() // slot=" + slot + " expectedAccountDelete=" + expectedAccountDelete + " expectedPathDelete=" + expectedPathDelete);
	await app.setSlot(slot, expectedAccountDelete, expectedPathDelete);
	var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
	var hexExpected = "331200001d0100000000000000000000000000000000000000000000000000000000";
	common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
	common.testStep(" >    ", "APDU out");
	common.asyncCurlApduSend(hexOutgoing);
	common.testStep("   +  ", "buttons");
	common.curlScreenShot(scriptNameCombo); common.curlButton('right', "; navigate the address / path; Delete Account 1");
	common.curlScreenShot(scriptNameCombo); common.curlButton('right', "; navigate the address / path; Old Account e467..");
	common.curlScreenShot(scriptNameCombo); common.curlButton('right', "; navigate the address / path; Old Path 44'/..");
	common.curlScreenShot(scriptNameCombo); common.curlButton('both', "; confirm; Approve");
	common.curlScreenShot(scriptNameCombo); console.log(common.humanTime() + " // back to main screen");
	common.testStep("     <", "APDU in");
	var hexResponse = await common.curlApduResponseWait();
	var hexExpected = "9000";
	common.compare(hexResponse, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
}

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
			pageCount: 12,
		},
		{
			title: "Create Account",
			blob: exampleCreateAccountBlob,
			pageCount: 20,
		},
		{
			title: "Add Key",
			blob: exampleAddKeyBlob,
			pageCount: 15,
		},
	];

	for (var i=0; i < transactions.length; ++i ) {
		for (var j=0; j < sigAlgos.length; ++j ) {
			for (var k=0; k < hashAlgos.length; ++k ) {
				var testTitle = `basic sign: ${transactions[i].title} - ${sigAlgos[j].name} / ${hashAlgos[k].name}`; // e.g. basic sign: Transfer FLOW - secp256k1 / SHA-256
				await transactionTest(
					testTitle,
					transactions[i].title,
					transactions[i].blob, 
					transactions[i].pageCount, 
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
			const txExpectedPageCount = getTransactionPageCount(transactions[i].envelopeMessage);
			var testTitle = `staking sign: ${transactions[i].title} - ${ECDSA_P256.name} / ${SHA3_256.name}`; // e.g. staking sign: TH.01 - Withdraw Unlocked FLOW - p256 / SHA3-256
			await transactionTest(
				testTitle,
				transactions[i].title,
				transactions[i].encodedTransactionEnvelopeHex,
				txExpectedPageCount,
				ECDSA_P256,
				SHA3_256,
			);
		}
	}
}

common.testEnd(scriptName);
