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
await common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

//getVersion
common.testStep(" - - -", "await app.getVersion()");
const getVersionResponse = await app.getVersion();
assert.equal(getVersionResponse.returnCode, 0x9000);
assert.equal(getVersionResponse.errorMessage, "No errors");
assert.equal(getVersionResponse.major, 0);
assert.equal(getVersionResponse.minor, 9);
assert.equal(getVersionResponse.patch, 12)
assert.ok("testMode" in getVersionResponse)
assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)

var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "3300000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "0000090c00311000049000";
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    var hexExpected = "0000090c00330000049000";
}
common.compare(hexIncomming, hexExpected, "apdu response", {testMode:1, major:1, minor:1, patch:1, deviceLocked:1, targetId:4, returnCode:2, unexpected:9999});

await transport.close()
common.testEnd(scriptName);
