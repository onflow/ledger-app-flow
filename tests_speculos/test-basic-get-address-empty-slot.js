'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
const slot = 6;
const address = "0000000000000000"
const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
const path = `m/0/0/0/0/0`;

common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

common.testStep(" - - -", "await app.setSlot() // slot=" + slot  + " address=" + address + " path=" + path);
await app.setSlot(slot , address, path);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d0600000000000000000000000000000000000000000000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
common.testStep(" >    ", "APDU out");
common.asyncCurlApduSend(hexOutgoing);
common.testStep("   +  ", "buttons");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Set Account 6");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account 0000..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Path 0/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
common.testStep("     <", "APDU in");
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

common.testStep(" - - -", "await app.getAddressAndPubKey() // slot=" + slot);
await app.getAddressAndPubKey(slot);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "330100000106";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, unexpected:9999});
common.testStep(" >    ", "APDU out");
common.asyncCurlApduSend(hexExpected);
common.testStep("     <", "APDU in");
var hexResponse = await common.curlApduResponseWait();
var hexEXpected = "6984";
common.compare(hexResponse, hexEXpected, "apdu response", {returnCode:2, unexpected:9999});

common.testEnd(scriptName);

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("get address - empty slot", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/0/0/0/0/0`;
            const address = "0000000000000000"

            await prepareSlot(sim, app, 0, address, path)

            const resp = await app.getAddressAndPubKey(0);

            console.log(resp);

            expect(resp.returnCode).toEqual(0x6984); 
            expect(resp.errorMessage).toEqual("Data is invalid");
        } finally {
            await sim.close();
        }
    });
*/
