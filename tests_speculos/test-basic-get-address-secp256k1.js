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
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

console.log(common.humanTime() + " // screen shot before sending first apdu command");
common.curlScreenShot(scriptName);

//get pubkey
common.testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path);
const getPubkeyResponse = await app.getAddressAndPubKey(path);
assert.equal(getPubkeyResponse.returnCode, 0x9000);
assert.equal(getPubkeyResponse.errorMessage, "No errors");
const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";
assert.equal(getPubkeyResponse.address.toString(), expected_pk);
assert.equal(getPubkeyResponse.publicKey.toString('hex'), expected_pk);

var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "3301000014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
common.compare(hexIncomming, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});


common.testEnd(scriptName);

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("get address - secp256k1", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "e467b9dd11fa00df"

            await prepareSlot(sim, app, 0, address, path)

            const resp = await app.getAddressAndPubKey(0);

            console.log(resp)

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
