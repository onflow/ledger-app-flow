'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

//
//
//
console.log(common.humanTime() + " v".repeat(64) + " test: slot status - set - mainnet");

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
const expectedSlot = 10;
const expectedAccount = "e467b9dd11fa00df";
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const expectedPath = `m/44'/539'/${scheme}'/0/0`;

console.log(common.humanTime() + " // screen shot before sending first apdu command");
common.curlScreenShot(scriptName);

console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPath=" + expectedPath);
await app.setSlot(expectedSlot, expectedAccount, expectedPath);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d0ae467b9dd11fa00df2c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});

common.asyncCurlApduSend(hexOutgoing);

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Set Account 10");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Account e467..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Path 44'/..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // confirm; Approve");
common.curlButton('both');

console.log(common.humanTime() + " // main screen");
common.curlScreenShot(scriptName);

var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

const invalidAddress = "fd00fa11ddb967e4";
console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // expectedSlot=" + expectedSlot + " invalidAddress=" + invalidAddress + " expectedPath=" + expectedPath);
await app.setSlot(expectedSlot, invalidAddress, expectedPath);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d0afd00fa11ddb967e42c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});

common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "6984";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

//
//
//
console.log(common.humanTime() + " v".repeat(64) + " test: slot status - update");

const expectedPathUpdate = `m/44'/539'/${scheme}'/0/1`;
console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPathUpdate=" + expectedPathUpdate);
await app.setSlot(10, expectedAccount, expectedPathUpdate);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d0ae467b9dd11fa00df2c0000801b020080010200800000000001000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});

common.asyncCurlApduSend(hexOutgoing);

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Update Account 10");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Old Account e467..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Old Path 44'/..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; New Account e467..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; New Path 44'/..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // confirm; Approve");
common.curlButton('both');

console.log(common.humanTime() + " // main screen");
common.curlScreenShot(scriptName);

var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

//
//
//
console.log(common.humanTime() + " v".repeat(64) + " test: slot status - delete");

const expectedAccountDelete = "0000000000000000";
const expectedPathDelete = `m/0/0/0/0/0`;
console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccountDelete=" + expectedAccountDelete + " expectedPathDelete=" + expectedPathDelete);
// Try to delete
await app.setSlot(10, expectedAccountDelete, expectedPathDelete);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d0a00000000000000000000000000000000000000000000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});

common.asyncCurlApduSend(hexOutgoing);

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Delete Account 10");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Old Account e467..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // navigate the address / path; Old Path 44'/..");
common.curlButton('right');

common.curlScreenShot(scriptName);
console.log(common.humanTime() + " // confirm; Approve");
common.curlButton('both');

console.log(common.humanTime() + " // main screen");
common.curlScreenShot(scriptName);

var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

//screen shot should not change so do not: common.curlScreenShot(scriptName);

common.testEnd();

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
