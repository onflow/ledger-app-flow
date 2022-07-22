'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs ,getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { getButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = getButtonsAndSnapshots(scriptName, speculosConf);
let hexExpected = "";

await device.makeStartingScreenshot();

//send invalid message
const options = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${0x201}'/0/0`;
const invalidMessage = Buffer.from(
	"1234567890",
	"hex",
);
const randomScriptHash = "ca80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2"

testStep(" - - -", "await app.sign() // path=" + path + " invalidMessage=..");
const signResponse = await app.sign(path, invalidMessage, options, randomScriptHash);
assert.equal(signResponse.returnCode, 0x6984);
assert.equal(signResponse.errorMessage, "Data is invalid");

compareGetVersionAPDUs(transport);
hexExpected = "33020000162c0000801b0200800102008000000000000000000103";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, path:20, options:2, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
hexExpected = "33020100051234567890";
compareOutAPDU(transport, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, message:5, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//there are further Merkle tree APDUs
/*
//Second incoming APDU not cached by SpyTransport as SpeculosTransport throws an exception.
noMoreAPDUs(transport);
*/

await transport.close()
testEnd(scriptName);
process.stdin.pause()
