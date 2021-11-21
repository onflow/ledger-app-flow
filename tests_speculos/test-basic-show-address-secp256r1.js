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
console.log(common.humanTime() + " // Derivation path. First 3 items are automatically hardened!");
const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`; 

common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

//gepubkey 
common.testStep(" - - -", "app.showAddressAndPubKey() // goodSlot=" + path);
const showPubkeyPromise = app.showAddressAndPubKey(path);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [1/4] 04db..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [2/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [3/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [4/4] ..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const showPubkeyResponse = await showPubkeyPromise
assert.equal(showPubkeyResponse.returnCode, 0x9000);
assert.equal(showPubkeyResponse.errorMessage, "No errors");
const expected_pk = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";
assert.equal(showPubkeyResponse.address, expected_pk);
assert.equal(showPubkeyResponse.publicKey.toString('hex'), expected_pk);

assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "3301010014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3303464623061313433363465356266343361376464646136303335323264646665653935633566663132623438633439343830663036326537616139643230653834323135656566396238623736313735663332383032663735656435343131306532396337646337363035346632346330323863333132303938653731373761339000";
common.compare(hexIncomming, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});

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
