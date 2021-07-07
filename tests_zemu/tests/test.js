/** ******************************************************************************
 *  (c) 2020 Zondax GmbH
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************* */

import jest, {expect} from "jest";
import Zemu from "@zondax/zemu";
import FlowApp from "@onflow/ledger";
import jsSHA from "jssha";
import {ec as EC} from "elliptic";

const Resolve = require("path").resolve;
const APP_PATH = Resolve("../app/bin/app.elf");

const APP_SEED = "equip will roof matter pink blind book anxiety banner elbow sun young"
const simOptions = {
    logging: true,
    start_delay: 1500,
    custom: `-s "${APP_SEED}"`
    , X11: false
};

jest.setTimeout(60000)

describe('Basic checks', function () {
    /*it('can start and stop container', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
        } finally {
            await sim.close();
        }
    });*/

    it('app version', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());
            const resp = await app.getVersion();

            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");
            expect(resp).toHaveProperty("testMode");
            expect(resp).toHaveProperty("major");
            expect(resp).toHaveProperty("minor");
            expect(resp).toHaveProperty("patch");
        } finally {
            await sim.close();
        }
    });

    it('sign secp256k1 basic & verify SHA2-256 - create', async function () {
        const exampleCreateBlob = "f90289f90261b8a97472616e73616374696f6e287075626c69634b6579733a205b537472696e675d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b65792e6465636f64654865782829290a7d0a7d0a7df90173b901707b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303331227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"

        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleCreateBlob,
                "hex",
            );

            const pkResponse = await app.getAddressAndPubKey(path);
            console.log(pkResponse);
            expect(pkResponse.returnCode).toEqual(0x9000);
            expect(pkResponse.errorMessage).toEqual("No errors");

            // do not wait here..
            const signatureRequest = app.sign(path, txBlob);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_create_sha2_256", 21);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("secp256k1");
            const sig = resp.signatureDER.toString("hex");
            const pk = pkResponse.publicKey.toString("hex");
            console.log(digest);
            console.log(sig);
            console.log(pk);
            const signatureOk = ec.verify(digest, sig, pk, 'hex');
            expect(signatureOk).toEqual(true);
        } finally {
            await sim.close();
        }
    });

});
