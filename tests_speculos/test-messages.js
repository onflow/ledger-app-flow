'use strict';

import { testStart, testEnd, testStep, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs, getScriptName, getSpeculosDefaultConf, humanTime } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { getButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath, pathToFileURL } from 'url';
import assert from 'assert/strict';
import pkg from 'elliptic';
const {ec: EC} = pkg;
import jsSHA from "jssha";

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = getButtonsAndSnapshots(scriptName, speculosConf);

const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1, pathCode: 0x200 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256, pathCode: 0x300};

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256, pathCode: 0x01};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256, pathCode: 0x03};

const path = `m/44'/539'/0'/0/0`;
const options = ECDSA_P256.code | SHA3_256.code

await device.makeStartingScreenshot();


// We get pubkey so we can verify signature
testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path);
const getPubkeyResponse = await app.getAddressAndPubKey(path, options);
assert.equal(getPubkeyResponse.returnCode, 0x9000);
assert.equal(getPubkeyResponse.errorMessage, "No errors");
const pubkeyHex = getPubkeyResponse.publicKey.toString("hex")


{
	const message = Buffer.from("This is a nice message that has only displayable characters and is short enough to be displayed")

	testStep(" - - -", "Message with displayable characters");
    const signPromise =  app.signMessage(path, message, options);
    await device.review("Review message");
	const signResponse = await signPromise;
	assert.equal(signResponse.returnCode, 0x9000);
	assert.equal(signResponse.errorMessage, "No errors");
	
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-user");
	const hasher = new jsSHA(SHA3_256.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(message);
	const digestHex = hasher.getHash("HEX");
	const ec = new EC(ECDSA_P256.name);
	assert.ok(ec.verify(digestHex, signResponse.signatureDER.toString("hex"), pubkeyHex, 'hex'));
}

{
	const message = Buffer.alloc(1000, 0x40);

    testStep(" - - -", "Message too long to display");
    const signPromise =  app.signMessage(path, message, options);
    await device.review("Review message");
	const signResponse = await signPromise;
	assert.equal(signResponse.returnCode, 0x9000);
	assert.equal(signResponse.errorMessage, "No errors");
	
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-user");
	const hasher = new jsSHA(SHA3_256.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(message);
	const digestHex = hasher.getHash("HEX");
	const ec = new EC(ECDSA_P256.name);
	assert.ok(ec.verify(digestHex, signResponse.signatureDER.toString("hex"), pubkeyHex, 'hex'));
}

await device.toggleExpertMode("ON");

{
	const message = Buffer.from("This is a short message in expert mode");

	testStep(" - - -", "A short message, expert mode");
    const signPromise =  app.signMessage(path, message, options);
    await device.review("Review message");
	const signResponse = await signPromise;
	assert.equal(signResponse.returnCode, 0x9000);
	assert.equal(signResponse.errorMessage, "No errors");
	
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-user");
	const hasher = new jsSHA(SHA3_256.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(message);
	const digestHex = hasher.getHash("HEX");
	const ec = new EC(ECDSA_P256.name);
	assert.ok(ec.verify(digestHex, signResponse.signatureDER.toString("hex"), pubkeyHex, 'hex'));
}

await device.toggleExpertMode("OFF");

{
	const message = Buffer.concat([Buffer.from("This is a short message in expert mode"), Buffer.from("ee", "hex")]);

	testStep(" - - -", "A short message, non displayable character");
    const signPromise =  app.signMessage(path, message, options);
	const signResponse = await signPromise;
	console.log(signResponse)
	assert.equal(signResponse.returnCode, 0x6984);
	assert.equal(signResponse.errorMessage, "Data is invalid : Invalid message");
}

await transport.close()
testEnd(scriptName);
process.stdin.pause()
