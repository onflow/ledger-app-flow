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
const expectedSlot = 10;
const expectedAccount = "8c5303eaa26202d6";
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const expectedPath = `m/44'/2'/${scheme}'/0/0`;

await common.curlScreenShot(scriptName); common.curlButton('right', "; Menu start ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Expert mode ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Version ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Show address");
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Screen 1");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Screen 2");
await common.curlScreenShot(scriptName); common.curlButton('both', "; Approve");

await common.curlScreenShot(scriptName); //Flow Ready
/*

//setSlot
common.testStep(" - - -", "await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPath=" + expectedPath);
const setSlotResponse = await app.setSlot(expectedSlot, expectedAccount, expectedPath);
assert.equal(setSlotResponse.returnCode, 0x6984);
assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 0)

var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "331200001d0a8c5303eaa26202d62c00008002000080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
*/
//screen shot should not change so do not: common.curlScreenShot(scriptName);
await transport.close()
common.testEnd(scriptName);