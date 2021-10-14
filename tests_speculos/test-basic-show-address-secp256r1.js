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
console.log(common.humanTime() + " v".repeat(64) + " test: show address - secp256k1");

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
console.log(common.humanTime() + " // Derivation path. First 3 items are automatically hardened!");
const goodSlot = 4;
const fakeSlot = 3;
const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
const goodPath = `m/44'/539'/${scheme}'/0/0`; const goodAddress = "e467b9dd11fa00df";
const fakePath = `m/44'/1'/${scheme}'/0/0`  ; const fakeAddress = "8c5303eaa26202d6";

console.log(common.humanTime() + " // screen shot before sending first apdu command");
common.curlScreenShot(scriptName);

console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // goodSlot=" + goodSlot  + " goodAddress=" + goodAddress + " goodPath=" + goodPath);
await app.setSlot(goodSlot , goodAddress, goodPath);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d04e467b9dd11fa00df2c0000801b020080010300800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Set Account 4");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Path 44'/539'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // fakeSlot=" + fakeSlot  + " fakeAddress=" + fakeAddress + " fakePath=" + fakePath);
await app.setSlot(fakeSlot , fakeAddress, fakePath);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d038c5303eaa26202d62c00008001000080010300800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Set Account 3");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account 8c53..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Path 44'/1'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});


console.log(common.humanTime() + " -".repeat(64) + " await app.showAddressAndPubKey() // goodSlot=" + goodSlot);
await app.showAddressAndPubKey(goodSlot);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "330101000104";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexExpected);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [1/4] 04db..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [2/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [3/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [4/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Warning[1/2] Ledger does not check if the on-ch"); // todo: reformat this message so words not broken?
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Warning[2/2] ain account includes the pub key!"); // todo: reformat this message so words not broken?
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?
var hexResponse = await common.curlApduResponseWait();
var hexEXpected = "e467b9dd11fa00df04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a39000";
common.compare(hexResponse, hexEXpected, "apdu response", {address:8, publicKey:65, returnCode:4, unexpected:9999});

common.testEnd(scriptName);

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("show address - secp256r1", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Derivation path. First 3 items are automatically hardened!
            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "e467b9dd11fa00df"
            const fakepath = `m/44'/1'/${scheme}'/0/0`;
            const fakeAddress = "8c5303eaa26202d6";

            await prepareSlot(sim, app, 1, address, path)
            await prepareSlot(sim, app, 0, fakeAddress, fakepath)

            const respRequest = app.showAddressAndPubKey(1);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 7);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            const resp = await respRequest;

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "e467b9dd11fa00df";
            const expected_pk = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });
*/
