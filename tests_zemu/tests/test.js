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
import FlowApp from "@zondax/ledger-flow";
import jsSHA from "jssha";
import * as secp256k1 from "secp256k1";

const Resolve = require("path").resolve;
const APP_PATH = Resolve("../app/bin/app.elf");

const APP_SEED = "equip will roof matter pink blind book anxiety banner elbow sun young"
const sim_options = {
    logging: true,
    start_delay: 3000,
    custom: `-s "${APP_SEED}"`
    , X11: true
};

jest.setTimeout(25000)

describe('Basic checks', function () {
    it('can start and stop container', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
        } finally {
            await sim.close();
        }
    });

    it('app version', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
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

    it('get address', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
            const app = new FlowApp(sim.getTransport());

            const scheme = 0x301;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const resp = await app.getAddressAndPubKey(path);

            console.log(resp)

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "044c301ff7be3fdd59e667219cb1e454e7d831fb01ff16970c3374821fc1035f8274f72645ec83cebd35d8513829756d0b1203f713115c93038620ec2da14b0a21";
            const expected_pk = "044c301ff7be3fdd59e667219cb1e454e7d831fb01ff16970c3374821fc1035f8274f72645ec83cebd35d8513829756d0b1203f713115c93038620ec2da14b0a21";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);

        } finally {
            await sim.close();
        }
    });

    it('show address', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
            const app = new FlowApp(sim.getTransport());

            // Derivation path. First 3 items are automatically hardened!
            const scheme = 0x301;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const respRequest = app.showAddressAndPubKey(path);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "show_address", 5);

            const resp = await respRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "044c301ff7be3fdd59e667219cb1e454e7d831fb01ff16970c3374821fc1035f8274f72645ec83cebd35d8513829756d0b1203f713115c93038620ec2da14b0a21";
            const expected_pk = "044c301ff7be3fdd59e667219cb1e454e7d831fb01ff16970c3374821fc1035f8274f72645ec83cebd35d8513829756d0b1203f713115c93038620ec2da14b0a21";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

    it('show address - expert', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
            const app = new FlowApp(sim.getTransport());

            // Enable expert mode
            await sim.clickRight();
            await sim.clickBoth();

            // Derivation path. First 3 items are automatically hardened!
            const scheme = 0x301;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const respRequest = app.showAddressAndPubKey(path);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "show_address_expert", 7);

            const resp = await respRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "044c301ff7be3fdd59e667219cb1e454e7d831fb01ff16970c3374821fc1035f8274f72645ec83cebd35d8513829756d0b1203f713115c93038620ec2da14b0a21";
            const expected_pk = "044c301ff7be3fdd59e667219cb1e454e7d831fb01ff16970c3374821fc1035f8274f72645ec83cebd35d8513829756d0b1203f713115c93038620ec2da14b0a21";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

    it('sign basic & verify - transfer', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
            const app = new FlowApp(sim.getTransport());

            const scheme = 0x301;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                "f9027ef90256b901be696d706f72742046756e6769626c65546f6b656e2066726f6d203078663233336463656538386665306162650a696d706f727420466c6f77546f6b656e2066726f6d203078313635343635333339393034306136310a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df853a27b2274797065223a22554669783634222c2276616c7565223a223534352e3737227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162",
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

            await sim.compareSnapshotsAndAccept(".", "sign_basic_verify_transfer", 12);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Verify signature
            const pk = Uint8Array.from(pkResponse.publicKey)

            const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("UINT8ARRAY")

            const sigArray = Uint8Array.from(resp.signatureCompact.slice(0, 64));
            const signatureOk = secp256k1.ecdsaVerify(sigArray, digest, pk);
            expect(signatureOk).toEqual(true);
        } finally {
            await sim.close();
        }
    });

    it('sign basic & verify - create', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
            const app = new FlowApp(sim.getTransport());

            const scheme = 0x301;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                "f902b5f9028db89e7472616e73616374696f6e287075626c69634b6579733a205b5b55496e74385d5d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b6579290a7d0a7d0a7df901aab901a77b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a2255496e7438222c2276616c7565223a38377d2c7b2274797065223a2255496e7438222c2276616c7565223a3134397d2c7b2274797065223a2255496e7438222c2276616c7565223a3132367d2c7b2274797065223a2255496e7438222c2276616c7565223a3233387d5d7d2c7b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a2255496e7438222c2276616c7565223a3232317d2c7b2274797065223a2255496e7438222c2276616c7565223a3132337d2c7b2274797065223a2255496e7438222c2276616c7565223a37387d2c7b2274797065223a2255496e7438222c2276616c7565223a357d2c7b2274797065223a2255496e7438222c2276616c7565223a33347d2c7b2274797065223a2255496e7438222c2276616c7565223a3232357d2c7b2274797065223a2255496e7438222c2276616c7565223a3230317d2c7b2274797065223a2255496e7438222c2276616c7565223a3230317d5d7d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162",
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

            await sim.compareSnapshotsAndAccept(".", "sign_basic_verify_create", 12);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Verify signature
            const pk = Uint8Array.from(pkResponse.publicKey)

            const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("UINT8ARRAY")

            const sigArray = Uint8Array.from(resp.signatureCompact.slice(0, 64));
            const signatureOk = secp256k1.ecdsaVerify(sigArray, digest, pk);
            expect(signatureOk).toEqual(true);
        } finally {
            await sim.close();
        }
    });

    it('sign basic - invalid', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
            const app = new FlowApp(sim.getTransport());

            const scheme = 0x301;
            const path = `m/44'/539'/${scheme}'/0/0`;

            let invalidMessage = Buffer.from(
                "1234567890",
                "hex",
            );
            invalidMessage += "1";

            const pkResponse = await app.getAddressAndPubKey(path);
            console.log(pkResponse);
            expect(pkResponse.returnCode).toEqual(0x9000);
            expect(pkResponse.errorMessage).toEqual("No errors");

            // do not wait here..
            const signatureResponse = await app.sign(path, invalidMessage);
            console.log(signatureResponse);

            expect(signatureResponse.returnCode).toEqual(0x6984);
            expect(signatureResponse.errorMessage).toEqual("Data is invalid : parser_rlp_error_invalid_kind");
        } finally {
            await sim.close();
        }
    });
});
