'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

//getVersion
common.testStep(" - - -", "await app.getVersion()");
const getVersionResponse = await app.getVersion();
assert.equal(getVersionResponse.returnCode, 0x9000);
assert.equal(getVersionResponse.errorMessage, "No errors");
assert.equal(getVersionResponse.major, 0);
assert.equal(getVersionResponse.minor, 9);
assert.equal(getVersionResponse.patch, 12)
assert.ok("testMode" in getVersionResponse)
assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)

var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "3300000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "0000090c00311000049000";
common.compare(hexIncomming, hexExpected, "apdu response", {testMode:1, major:1, minor:1, patch:1, deviceLocked:1, targetId:4, returnCode:2, unexpected:9999});

common.testEnd(scriptName);

// todo: remove all commented zemu source code from speculos tests once zemu to speculos port PR has been accepted
// todo: remove tests_zemu/ folder once zemu to speculos port PR has been accepted
// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("app version", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());
            const resp = await app.getVersion();

            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");
            expect(resp).toHaveProperty("testMode");
            expect(resp).toHaveProperty("major");
            expect(resp).toHaveProperty("minor");
            expect(resp).toHaveProperty("patch");
        } finally {
            await sim.close();
        }
    });
*/
