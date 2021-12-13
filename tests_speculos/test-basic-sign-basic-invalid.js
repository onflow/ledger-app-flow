'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

await common.sleep(3000)

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
const slot = 1;
const address = "e467b9dd11fa00df"
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

//send invalid message
let invalidMessage = Buffer.from(
	"1234567890",
	"hex",
);
invalidMessage += "1";

common.testStep(" - - -", "await app.sign() // path=" + path + " invalidMessage=..");
const signResponse = await app.sign(path, invalidMessage);
assert.equal(signResponse.returnCode, 0x6984);
assert.equal(signResponse.errorMessage, "Data is invalid : parser_rlp_error_invalid_kind");

assert.equal(common.mockTransport.hexApduCommandOut.length, 2)
assert.equal(common.mockTransport.hexApduCommandIn.length, 2)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "33020000142c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, path:20, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "330202000812345678efbfbd31";
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "7061727365725f726c705f6572726f725f696e76616c69645f6b696e646984";
common.compare(hexIncomming, hexExpected, "apdu response", {signatureCompact:29, returnCode:2, unexpected:9999});

common.testEnd(scriptName);