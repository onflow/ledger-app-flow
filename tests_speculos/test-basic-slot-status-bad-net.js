'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, compareGetVersionAPDUs, getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
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

//setSlot
const expectedSlot = 10;
const expectedAccount = "8c5303eaa26202d6";
const options = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const expectedPath = `m/44'/2'/${0x201}'/0/0`;

testStep(" - - -", "await app.setSlot() // expectedSlot=" + expectedSlot + " expectedAccount=" + expectedAccount + " expectedPath=" + expectedPath);
const setSlotResponse = await app.setSlot(expectedSlot, expectedAccount, expectedPath, options);
assert.equal(setSlotResponse.returnCode, 0x6984);

compareGetVersionAPDUs(transport);
hexExpected = "331200001f0a8c5303eaa26202d62c000080020000800102008000000000000000000103";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:30, unexpected:9999});
//Incoming APDU not cached by SpyTransport as SpeculosTransport throws an exception.

await transport.close()
testEnd(scriptName);
process.stdin.pause()
