'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, noMoreAPDUs, getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { getButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = getButtonsAndSnapshots(scriptName, speculosConf);
let hexExpected = "";

await device.makeStartingScreenshot();

//getVersion
testStep(" - - -", "await app.getVersion()");
const getVersionResponse = await app.getVersion();
assert.equal(getVersionResponse.returnCode, 0x9000);
assert.equal(getVersionResponse.errorMessage, "No errors");
assert.equal(getVersionResponse.major, 0);
assert.equal(getVersionResponse.minor, 10);
assert.equal(getVersionResponse.patch, 4)
assert.ok("testMode" in getVersionResponse)
assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)

hexExpected = "3300000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, unexpected:9999});
hexExpected = {
    "nanos":  "00000a0400311000049000",
    "nanox":  "00000a0400330000049000",
    "nanosp": "00000a0400331000049000",
}[speculosConf.deviceType];
compareInAPDU(transport, hexExpected, "apdu response", {testMode:1, major:1, minor:1, patch:1, deviceLocked:1, targetId:4, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

await transport.close()
testEnd(scriptName);
process.stdin.pause()
