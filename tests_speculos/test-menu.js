'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, noMoreAPDUs, getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
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

const showAddressMenuItem = 4;

//Show address in menu, no data on slot
testStep(" - - -", "Review address from menu - empty slot");
await device.enterMenuElementAndReview(showAddressMenuItem, "Review address from menu");

//We set slot 0
const account = "e467b9dd11fa00de"; //this is not a proper account but the app does not test it
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;
const slot = 0

testStep(" - - -", "app.setSlot() //  Set slot 0");
const setSlotPromise = app.setSlot(slot, account, path);
device.review("Set slot 0");
const setSlotResponse = await setSlotPromise

assert.equal(setSlotResponse.returnCode, 0x9000);

hexExpected = "331200001d00e467b9dd11fa00de2c0000801b020080010200800000000000000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//Check address
testStep(" - - -", "Review address from menu - slot has data");
await device.enterMenuElementAndReview(showAddressMenuItem, "Review address from menu");

//We set slot 0 to something else
const account2 = "f3f7b9dd11fa00de"; //this is not a proper account but the app does not test it
const scheme2 = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
const path2 = `m/44'/539'/${scheme2}'/0/1`;

testStep(" - - -", "app.setSlot() //  Update slot 0");
const setSlotPromise2 = app.setSlot(slot, account2, path2);
device.review("Update slot 0");
const setSlotResponse2 = await setSlotPromise2
assert.equal(setSlotResponse2.returnCode, 0x9000);

hexExpected = "331200001d00f3f7b9dd11fa00de2c0000801b020080010300800000000001000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//set expert mode
await device.toggleExpertMode("Turn ON");

//Check address
testStep(" - - -", "Review address from menu - slot has updated data");
await device.enterMenuElementAndReview(showAddressMenuItem, "Review updated address from menu");

//unset expert mode
await device.toggleExpertMode("Turn OFF");

//screen shot should not change so do not: common.curlScreenShot(scriptName);
await transport.close()
testEnd(scriptName);