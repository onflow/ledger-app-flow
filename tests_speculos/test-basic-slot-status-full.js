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
console.log(common.humanTime() + " v".repeat(64) + " test: slot status - full");

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");

console.log(common.humanTime() + " // screen shot before sending first apdu command");
common.curlScreenShot(scriptName);

console.log(common.humanTime() + " -".repeat(64) + " await app.slotStatus() // Check initial status");
await app.slotStatus();
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "3310000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009000";
common.compare(hexResponse, hexExpected, "apdu response", {slotStatus:64, returnCode:4, unexpected:9999});

const emptySlot = 3;
console.log(common.humanTime() + " -".repeat(64) + " await app.getSlot() // emptySlot=" + emptySlot + "; Get empty slot should error");
common.mockTransportSetNextFakeResponseToError();
await app.getSlot(emptySlot);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331100000103";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "6982";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

const expectedSlot = 10;
const expectedAccount = "e467b9dd11fa00df";
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const expectedPath = `m/44'/539'/${scheme}'/0/0`;
console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPath=" + expectedPath + "; Set slot 10");
await app.setSlot(expectedSlot, expectedAccount, expectedPath);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d0ae467b9dd11fa00df2c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});

common.asyncCurlApduSend(hexOutgoing);

common.curlScreenShot(scriptName);
common.curlButton('right', "; navigate the address / path; Set Account 10");

common.curlScreenShot(scriptName);
common.curlButton('right', "; navigate the address / path; Account e467..");

common.curlScreenShot(scriptName);
common.curlButton('right', "; navigate the address / path; Path 44'/..");

common.curlScreenShot(scriptName);
common.curlButton('both', "; confirm; Approve");

console.log(common.humanTime() + " // main screen");
common.curlScreenShot(scriptName);

var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

console.log(common.humanTime() + " -".repeat(64) + " await app.slotStatus() // Get slot status");
await app.slotStatus();
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "3310000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "000000000000000000000100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000009000";
common.compare(hexResponse, hexExpected, "apdu response", {slotStatus:64, returnCode:4, unexpected:9999});

console.log(common.humanTime() + " -".repeat(64) + " await app.getSlot() // expectedSlot=" + expectedSlot + "; Get slot 10 back");
common.mockTransportSetNextFakeResponseToError();
await app.getSlot(expectedSlot);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "33110000010a";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "e467b9dd11fa00df2c0000801b0200800102008000000000000000009000";
common.compare(hexResponse, hexExpected, "apdu response", {account:8, path:20, returnCode:4, unexpected:9999});

const expectedAccountDelete = "0000000000000000";
const expectedPathDelete = `m/0/0/0/0/0`;
console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccountDelete=" + expectedAccountDelete + " expectedPathDelete=" + expectedPathDelete + "; Delete slot 10");
await app.setSlot(10, expectedAccountDelete, expectedPathDelete);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d0a00000000000000000000000000000000000000000000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Delete Account 10");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

//screen shot should not change so do not: common.curlScreenShot(scriptName);

common.testEnd();

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("slot status - full", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Check initial status
            let respStatus = await app.slotStatus();
            console.log(respStatus);
            expect(respStatus.returnCode).toEqual(0x9000);
            expect(respStatus.errorMessage).toEqual("No errors");
            expect(respStatus).toHaveProperty("status");

            let expectedBuffer = Buffer.alloc(64);
            expectedBuffer.fill(0);
            expect(respStatus.status).toEqual(expectedBuffer);

            // Get empty slot should error
            let respSlot = await app.getSlot(3);
            console.log(respSlot);
            expect(respSlot.returnCode).toEqual(0x6982);
            expect(respSlot.errorMessage).toEqual("Empty Buffer");

            // Set slot 10
            const expectedAddress = "e467b9dd11fa00df"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respSetRequest = app.setSlot(10, expectedAddress, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 3);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            let respSet = await respSetRequest;
            console.log(respSet);
            expect(respSet.returnCode).toEqual(0x9000);

            // Get slot status
            let respStatus2 = await app.slotStatus();
            console.log(respStatus2);
            expect(respStatus2.returnCode).toEqual(0x9000);
            expect(respStatus2.errorMessage).toEqual("No errors");
            expectedBuffer = Buffer.alloc(64);
            expectedBuffer.fill(0);
            expectedBuffer[10] = 1;
            expect(respStatus2.status).toEqual(expectedBuffer);

            // Get slot 10 back
            let respGet2 = await app.getSlot(10);
            console.log(respGet2);
            expect(respGet2.returnCode).toEqual(0x9000);
            expect(respGet2.account).toEqual(expectedAddress);
            expect(respGet2.path).toEqual(expectedPath);

        } finally {
            await sim.close();
        }
    });
*/
