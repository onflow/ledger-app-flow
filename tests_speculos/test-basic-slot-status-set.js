'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.test_start(scriptName);

console.log(common.humanTime() + " // calling FlowApp with common.mockTransport() to create payload");
const FlowApp = OnflowLedgerMod.default;
const expectedAccount = "0001020304050607"
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const expectedPath = `m/44'/539'/${scheme}'/0/0`;
const app = new FlowApp(common.mockTransport);
let respRequest = app.setSlot(10, expectedAccount, expectedPath);

common.curl_screen_shot(scriptName);

var curl_apdu_object = common.curl_apdu(common.hexPayloadViaMockTransport, 500, scriptName);

await common.sleep(500);
common.curl_screen_shot(scriptName);

for (let i = 0; i < 4; i++) {
	common.curl_button('right');
	common.curl_screen_shot(scriptName);
}

common.curl_button('both');
common.curl_screen_shot(scriptName);

await common.curl_apdu_response(curl_apdu_object, '{"data": "6986"}')

common.test_end();

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("slot status - set", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Set slot 10
            const expectedAccount = "0001020304050607"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respRequest = app.setSlot(10, expectedAccount, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 3);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());
    
            const resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });
*/
