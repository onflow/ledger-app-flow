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
console.log(common.humanTime() + " v".repeat(64) + " test: app version");

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");

console.log(common.humanTime() + " // screen shot before sending first apdu command");
common.curlScreenShot(scriptName);

console.log(common.humanTime() + " -".repeat(64) + " await app.getVersion()");
await app.getVersion();
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "3300000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});

common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "0000090b00311000049000";
common.compare(hexResponse, hexExpected, "apdu response", {testMode:1, major:1, minor:1, patch:1, deviceLocked:1, targetId:4, returnCode:4, unexpected:9999});

//screen shot should not change so do not: common.curlScreenShot(scriptName);

common.testEnd(scriptName);

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
