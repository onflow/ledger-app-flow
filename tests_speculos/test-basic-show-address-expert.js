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
const slot = 5;
const address = "e467b9dd11fa00df"
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

console.log(common.humanTime() + " -".repeat(64) + " Enable expert mode");
common.curlButton('right', "; Expert mode: disabled"); common.curlScreenShot(scriptName);
common.curlButton('both' , "; Expert mode: enabled" ); common.curlScreenShot(scriptName);
common.curlButton('left' , "; Flow Ready"           ); common.curlScreenShot(scriptName);

console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // slot=" + slot  + " address=" + address + " path=" + path);
await app.setSlot(slot , address, path);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d05e467b9dd11fa00df2c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Set Account 1");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

console.log(common.humanTime() + " -".repeat(64) + " await app.showAddressAndPubKey() // slot=" + slot);
await app.showAddressAndPubKey(slot);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "330101000105";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexExpected);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [1/4] 04d7..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [2/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [3/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [4/4] ..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Warning[1/2] Ledger does not check if the on-ch"); // todo: reformat this message so words not broken?
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Warning[2/2] ain account includes the pub key!"); // todo: reformat this message so words not broken?
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Your Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?
var hexResponse = await common.curlApduResponseWait();
var hexEXpected = "e467b9dd11fa00df04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be9000";
common.compare(hexResponse, hexEXpected, "apdu response", {address:8, publicKey:65, returnCode:4, unexpected:9999});

common.testEnd(scriptName);

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("show address - expert", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Enable expert mode
            await sim.clickRight();
            await sim.clickBoth();
            await sim.clickLeft();

            // Derivation path. First 3 items are automatically hardened!
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "e467b9dd11fa00df"

            await prepareSlot(sim, app, 1, address, path)

            const respRequest = app.showAddressAndPubKey(1);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 8);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            const resp = await respRequest;

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "e467b9dd11fa00df";
            const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });
*/
