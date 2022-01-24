'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const transport = await common.getSpyTransport()
const app = new FlowApp(transport);

console.log(common.humanTime() + " // using FlowApp below with transport() to grab apdu command without sending it");

await common.curlScreenShot(scriptName); common.curlButton('right', "; Menu start ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Expert mode ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Version ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Show address");
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Not saved");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Approve");

await common.curlScreenShot(scriptName); //Flow Ready


//We set slot 0
const account = "e467b9dd11fa00de"; //this is not a proper account but the app does not test it
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;
const slot = 0
common.testStep(" - - -", "app.setSlot() // slot=" + 0 + " account=" + account + " path=" + path + "; Set slot 0");
const setSlotPromise = app.setSlot(slot, account, path);
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Set slot 0 ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Path ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Address ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?
await common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const setSlotResponse = await setSlotPromise
assert.equal(setSlotResponse.returnCode, 0x9000);

assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "331200001d00e467b9dd11fa00de2c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//Check address
common.curlButton('right', "; Menu start ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Expert mode ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Version ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Show address");
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 1");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 2");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 3");
if (!(process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox")) {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 4");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Address");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Message 1");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Message 2");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Approve");
await common.curlScreenShot(scriptName); //back to main screen

//We set slot 0 to something else
const account2 = "f3f7b9dd11fa00de"; //this is not a proper account but the app does not test it
const scheme2 = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
const path2 = `m/44'/539'/${scheme2}'/0/1`;
common.testStep(" - - -", "app.setSlot() // slot=" + 0 + " account=" + account2 + " path=" + path2 + "; Set slot 0");
const setSlotPromise2 = app.setSlot(slot, account2, path2);
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Update slot 0 ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Old account");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Old path");
await common.curlScreenShot(scriptName); common.curlButton('right', "; New Account");
await common.curlScreenShot(scriptName); common.curlButton('right', "; New Path");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Approve");
await common.curlScreenShot(scriptName); //back to main screen
const setSlotResponse2 = await setSlotPromise2
assert.equal(setSlotResponse2.returnCode, 0x9000);

assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "331200001d00f3f7b9dd11fa00de2c0000801b020080010300800000000001000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//set expert mode
common.curlButton('right', "; Expert mode ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Secret mode on");
await common.curlScreenShot(scriptName); common.curlButton('left', "; Flow ready ..");
await common.curlScreenShot(scriptName);

//Check address
common.curlButton('right', "; Menu start ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Expert mode ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Version ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Show address");
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 1");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 2");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 3");
if (!(process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox")) {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Pubkey 4");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Address");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Message 1");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Message 2");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Derivation path");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Approve");
await common.curlScreenShot(scriptName); //back to main screen


//unset expert mode
common.curlButton('right', "; Expert mode ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Secret mode on");
await common.curlScreenShot(scriptName); common.curlButton('left', "; Flow ready ..");
await common.curlScreenShot(scriptName);

//screen shot should not change so do not: common.curlScreenShot(scriptName);
await transport.close()
common.testEnd(scriptName);