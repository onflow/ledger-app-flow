'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

//
//
//
common.testStep(" v v v", "block: slot status - set - mainnet");

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
const expectedSlot = 10;
const expectedAccount = "e467b9dd11fa00df";
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const expectedPath = `m/44'/539'/${scheme}'/0/0`;

common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

//setSlot valid account
common.testStep(" - - -", "app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPath=" + expectedPath);
const setSlotPromise = app.setSlot(expectedSlot, expectedAccount, expectedPath);
common.testStep("   +  ", "buttons");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Set Account 10");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
common.testStep(" - - -", "await setSlotPromise // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPath=" + expectedPath);
const setSlotResponse = await setSlotPromise
assert.equal(setSlotResponse.returnCode, 0x9000);

assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "331200001d0ae467b9dd11fa00df2c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//setSlot invalid account
const invalidAddress = "fd00fa11ddb967e4";
common.testStep(" - - -", "await app.setSlot() // expectedSlot=" + expectedSlot + " invalidAddress=" + invalidAddress + " expectedPath=" + expectedPath);
const setSlotResponse2 = await app.setSlot(expectedSlot, invalidAddress, expectedPath);
assert.equal(setSlotResponse2.returnCode, 0x6984);

assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "331200001d0afd00fa11ddb967e42c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "6984";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//
//
//
common.testStep(" v v v", "block: slot status - update");

const expectedPathUpdate = `m/44'/539'/${scheme}'/0/1`;
common.testStep(" - - -", "app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPathUpdate=" + expectedPathUpdate);
const setSlotPromise3 = app.setSlot(expectedSlot, expectedAccount, expectedPathUpdate);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Update Account 10");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; New Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; New Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const setSlotResponse3 = await setSlotPromise3
assert.equal(setSlotResponse3.returnCode, 0x9000);

assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "331200001d0ae467b9dd11fa00df2c0000801b020080010200800000000001000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//
//
//
common.testStep(" v v v", "block: slot status - delete");

const expectedAccountDelete = "0000000000000000";
const expectedPathDelete = `m/0/0/0/0/0`;
common.testStep(" - - -", "await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccountDelete=" + expectedAccountDelete + " expectedPathDelete=" + expectedPathDelete);
const setSlotPromise4 = app.setSlot(expectedSlot, expectedAccountDelete, expectedPathDelete);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Delete Account 10");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const setSlotResponse4 = await setSlotPromise4
assert.equal(setSlotResponse4.returnCode, 0x9000);

assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "331200001d0a00000000000000000000000000000000000000000000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//screen shot should not change so do not: common.curlScreenShot(scriptName);

common.testEnd(scriptName);

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("slot status - set - mainnet", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Set slot 10, mainnet, valid address
            const expectedAddress = "e467b9dd11fa00df"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respRequest = app.setSlot(10, expectedAddress, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 3);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());
    
            const resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

            // Set slot 10, mainnet, invalid address
            const invalidAddress = "fd00fa11ddb967e4"
            let respRequest2 = app.setSlot(10, invalidAddress, expectedPath);
            const resp2 = await respRequest2;
            console.log(resp2);
            expect(resp2.returnCode).toEqual(0x6984);

        } finally {
            await sim.close();
        }
    });

    test("slot status - update", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Set slot 10
            const expectedAddress = "e467b9dd11fa00df"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            let expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respRequest = await prepareSlot(sim, app, 10, expectedAddress, expectedPath);
            await Zemu.sleep(1000);

            expectedPath = `m/44'/539'/${scheme}'/0/1`;
            respRequest = app.setSlot(10, expectedAddress, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 5);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            let resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });

    test("slot status - delete", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Set slot 10
            const expectedAddress = "e467b9dd11fa00df"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            let expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respRequest = prepareSlot(sim, app, 10, expectedAddress, expectedPath);
            await Zemu.sleep(1000);

            // Try to delete
            respRequest = app.setSlot(10, "0000000000000000", `m/0/0/0/0/0`);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 3);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            let resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });
*/
