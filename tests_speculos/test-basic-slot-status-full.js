'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs, getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { ButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = new ButtonsAndSnapshots(scriptName, speculosConf);
let hexExpected = "";

await device.makeStartingScreenshot();

let expectedBuffer = "";

//slotStatus test when there are no slots
testStep(" - - -", "await app.slotStatus() // Check initial status");
const slotStatusResponse = await app.slotStatus();

assert.equal(slotStatusResponse.returnCode, 0x9000);
assert.equal(slotStatusResponse.errorMessage, "No errors");
expectedBuffer = Buffer.alloc(64).fill(0);
assert.equal(slotStatusResponse.status.toString("hex"), expectedBuffer.toString("hex"));

hexExpected = "3310000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, unexpected:9999});
hexExpected = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009000";
compareInAPDU(transport, hexExpected, "apdu response", {slotStatus:64, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//setSlot 10
const expectedSlot = 10;
const expectedAccount = "e467b9dd11fa00df";
const options = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const expectedPath = `m/44'/539'/${0x201}'/0/0`;

testStep(" - - -", "app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPath=" + expectedPath + "; Set slot 10");
const setSlotPromise = app.setSlot(expectedSlot, expectedAccount, expectedPath, options);
device.review("Set slot 10");
const setSlotResponse = await setSlotPromise

assert.equal(setSlotResponse.returnCode, 0x9000);

compareGetVersionAPDUs(transport);
hexExpected = "331200001f0ae467b9dd11fa00df2c0000801b0200800102008000000000000000000103";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:30, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//test slotstatus when slot 10 is used
testStep(" - - -", "await app.slotStatus() // Get slot status");
const slotStatusResponse2 = await app.slotStatus();

assert.equal(slotStatusResponse2.returnCode, 0x9000);
assert.equal(slotStatusResponse2.errorMessage, "No errors");
expectedBuffer = Buffer.alloc(64).fill(0);
expectedBuffer[10] = 1;
assert.equal(slotStatusResponse2.status.toString("hex"), expectedBuffer.toString("hex"));

hexExpected = "3310000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, unexpected:9999});
hexExpected = "000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009000";
compareInAPDU(transport, hexExpected, "apdu response", {slotStatus:64, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//test get slot on used slot 10
testStep(" - - -", "await app.getSlot(); Get slot 10 back");
const getSlotResponse = await app.getSlot(expectedSlot);

assert.equal(getSlotResponse.returnCode, 0x9000);
assert.equal(getSlotResponse.account, expectedAccount);
assert.equal(getSlotResponse.path, expectedPath);
assert.equal(getSlotResponse.options, options);

hexExpected = "33110000010a";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, unexpected:9999});
hexExpected = "e467b9dd11fa00df2c0000801b02008001020080000000000000000001039000";
compareInAPDU(transport, hexExpected, "apdu response", {account:8, path:20, options:2, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//getSlot for an empty slot
const emptySlot = 3;

testStep(" - - -", "await app.getSlot() // emptySlot=" + emptySlot + "; Get empty slot should error");
const getSlotResponse2 = await app.getSlot(emptySlot);

assert.equal(getSlotResponse2.returnCode, 0x6982);
assert.equal(getSlotResponse2.errorMessage, "Empty Buffer");

hexExpected = "331100000103";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, unexpected:9999});
//Incoming APDU not cached by SpyTransport as SpeculosTransport throws an exception.
noMoreAPDUs(transport)

//setSlot 10 - update slot
const expectedAccount2 = "e467b9dd11fa00de"; //this is not a proper account but the app does not test it
const expectedPath2 = `m/44'/539'/${0x201}'/0/1`;
const options2 = FlowApp.Signature.P256 | FlowApp.Hash.SHA3_256;

testStep(" - - -", "app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount2=" + expectedAccount2 + " expectedPath2=" + expectedPath2 + "; Set slot 10");
const setSlotPromise2 = app.setSlot(expectedSlot, expectedAccount2, expectedPath2, options2);
device.review("Update slot 10");
const setSlotResponse2 = await setSlotPromise2;

assert.equal(setSlotResponse2.returnCode, 0x9000);

compareGetVersionAPDUs(transport);
hexExpected = "331200001f0ae467b9dd11fa00de2c0000801b0200800102008000000000010000000302";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:30, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//test slotstatus when slot 10 is used
testStep(" - - -", "await app.slotStatus() // Get slot status");
const slotStatusResponse3 = await app.slotStatus();

assert.equal(slotStatusResponse3.returnCode, 0x9000);
assert.equal(slotStatusResponse3.errorMessage, "No errors");
expectedBuffer = Buffer.alloc(64).fill(0);
expectedBuffer[10] = 1;
assert.equal(slotStatusResponse3.status.toString("hex"), expectedBuffer.toString("hex"));

hexExpected = "3310000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, unexpected:9999});
hexExpected = "000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009000";
compareInAPDU(transport, hexExpected, "apdu response", {slotStatus:64, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//test get slot on used slot 10
testStep(" - - -", "await app.getSlot() // expectedSlot=" + expectedSlot + "; Get slot 10 back");
const getSlotResponse3 = await app.getSlot(expectedSlot);

assert.equal(getSlotResponse3.returnCode, 0x9000);
assert.equal(getSlotResponse3.account, expectedAccount2);
assert.equal(getSlotResponse3.path, expectedPath2);
assert.equal(getSlotResponse3.options, options2);

hexExpected = "33110000010a";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, unexpected:9999});
hexExpected = "e467b9dd11fa00de2c0000801b02008001020080000000000100000003029000";
compareInAPDU(transport, hexExpected, "apdu response", {account:8, path:20, options:2, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//Now delete the slot so that the next test starts in a clean state
const expectedAccountDelete = "0000000000000000";
const expectedPathDelete = `m/0/0/0/0/0`;

testStep(" - - -", "app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccountDelete=" + expectedAccountDelete + " expectedPathDelete=" + expectedPathDelete + "; Delete slot 10");
const setSlotPromise3 = app.setSlot(10, expectedAccountDelete, expectedPathDelete, 0);
device.review("Set slot 10");
const setSlotResponse3 = await setSlotPromise3;

assert.equal(setSlotResponse3.returnCode, 0x9000);

compareGetVersionAPDUs(transport);
hexExpected = "331200001f0a000000000000000000000000000000000000000000000000000000000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:30, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

await transport.close()
testEnd(scriptName);