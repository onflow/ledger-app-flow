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
        let snapshotCount = 0;

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
            await sim.compareSnapshotsAndAccept(".", "show_address", 6);

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
        let snapshotCount = 0;

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

    it.skip('sign basic & verify', async function () {
        const snapshotPrefixGolden = "snapshots/sign-basic/";
        const snapshotPrefixTmp = "snapshots-tmp/sign-basic/";
        let snapshotCount = 0;

        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(sim_options);
            const app = new FlowApp(sim.getTransport());

            const scheme = 0x301;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                "1234567890",
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

            await sim.compareSnapshotsAndAccept(".", "sign_basic_verify", 9);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Verify signature
            // const pk = Uint8Array.from(pkResponse.compressed_pk)
            // const digest = getDigest( txBlob );
            // const signature = secp256k1.signatureImport(Uint8Array.from(resp.signature_der));
            // const signatureOk = secp256k1.ecdsaVerify(signature, digest, pk);
            // expect(signatureOk).toEqual(true);
        } finally {
            await sim.close();
        }
    });

    it.skip('sign basic - invalid', async function () {
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
            expect(signatureResponse.errorMessage).toEqual("Data is invalid : Unexpected data type");
        } finally {
            await sim.close();
        }
    });
});
