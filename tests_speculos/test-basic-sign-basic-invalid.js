'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

await common.sleep(3000)

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const transport = await common.getSpyTransport()
const app = new FlowApp(transport);

console.log(common.humanTime() + " // using FlowApp below with transport() to grab apdu command without sending it");
const slot = 1;
const address = "e467b9dd11fa00df"
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

await common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

//send invalid message
let invalidMessage = Buffer.from(
	"1234567890",
	"hex",
);
invalidMessage += "1";

common.testStep(" - - -", "await app.sign() // path=" + path + " invalidMessage=..");
const signResponse = await app.sign(path, invalidMessage);
assert.equal(signResponse.returnCode, 0x6984);
assert.equal(signResponse.errorMessage, "Data is invalid");

assert.equal(transport.hexApduCommandOut.length, 2)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "33020000142c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, path:20, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "330202000812345678efbfbd31";
common.compare(hexOutgoing, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, message:8, unexpected:9999});

await transport.close()
common.testEnd(scriptName);