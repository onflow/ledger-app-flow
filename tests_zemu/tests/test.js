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
    , X11: true
};

jest.setTimeout(60000)

describe('Basic checks', function () {
    it('can start and stop container', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
        } finally {
            await sim.close();
        }
    });

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

    it('sign basic - invalid', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
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

    // accounts

    it('slot status - set', async function () {
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
            await sim.compareSnapshotsAndAccept(".", "slot_status_set", 4);

            const resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });

    it('slot status - update', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Set slot 10
            const expectedAccount = "0001020304050607"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            let expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respRequest = app.setSlot(10, expectedAccount, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            await sim.clickRight();
            await sim.clickRight();
            await sim.clickRight();
            await sim.clickBoth();

            resp = await respRequest;
            await Zemu.sleep(1000);

            expectedPath = `m/44'/539'/${scheme}'/0/1`;
            respRequest = app.setSlot(10, expectedAccount, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "slot_status_update", 6);

            let resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });

    it('slot status - delete', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Set slot 10
            const expectedAccount = "0001020304050607"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            let expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respRequest = app.setSlot(10, expectedAccount, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            await sim.clickRight();
            await sim.clickRight();
            await sim.clickRight();
            await sim.clickBoth();

            resp = await respRequest;
            await Zemu.sleep(1000);

            // Try to delete
            respRequest = app.setSlot(10, "0000000000000000", `m/0/0/0/0/0`);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "slot_status_delete", 4);

            let resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });

    it('slot status - full', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Check initial status
            let respStatus = await app.slotStatus();
            console.log(respStatus);
            expect(respStatus.returnCode).toEqual(0x9000);
            expect(respStatus.errorMessage).toEqual("No errors");
            expect(respStatus).toHaveProperty("status");

            let expectedBuffer = Buffer.alloc(64);
            expectedBuffer.fill(0);
            expect(respStatus.status).toEqual(expectedBuffer);

            // Get empty slot should error
            let respSlot = await app.getSlot(3);
            console.log(respSlot);
            expect(respSlot.returnCode).toEqual(0x6982);
            expect(respSlot.errorMessage).toEqual("Empty Buffer");

            // Set slot 10
            const expectedAccount = "0001020304050607"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const expectedPath = `m/44'/539'/${scheme}'/0/0`;
            let respSetRequest = app.setSlot(10, expectedAccount, expectedPath);

            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "slot_status_full", 4);

            let respSet = await respSetRequest;
            console.log(respSet);
            expect(respSet.returnCode).toEqual(0x9000);

            // Get slot status
            let respStatus2 = await app.slotStatus();
            console.log(respStatus2);
            expect(respStatus2.returnCode).toEqual(0x9000);
            expect(respStatus2.errorMessage).toEqual("No errors");
            expectedBuffer = Buffer.alloc(64);
            expectedBuffer.fill(0);
            expectedBuffer[10] = 1;
            expect(respStatus2.status).toEqual(expectedBuffer);

            // Get slot 10 back
            let respGet2 = await app.getSlot(10);
            console.log(respGet2);
            expect(respGet2.returnCode).toEqual(0x9000);
            expect(respGet2.account).toEqual(expectedAccount);
            expect(respGet2.path).toEqual(expectedPath);

        } finally {
            await sim.close();
        }
    });

    // secp256k1

    it('get address - secp256k1', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const resp = await app.getAddressAndPubKey(path);

            console.log(resp)

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";
            const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);

        } finally {
            await sim.close();
        }
    });

    it('show address - secp256k1', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Derivation path. First 3 items are automatically hardened!
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const respRequest = app.showAddressAndPubKey(path);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "show_address_secp256k1", 5);

            const resp = await respRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";
            const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

    it('show address - expert', async function () {
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

            const respRequest = app.showAddressAndPubKey(path);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "show_address_expert", 6);

            const resp = await respRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";
            const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

    const exampleTransferBlob = "f9023ff9023bb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df861b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
    const exampleCreateBlob = "f90289f90261b8a97472616e73616374696f6e287075626c69634b6579733a205b537472696e675d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b65792e6465636f64654865782829290a7d0a7d0a7df90173b901707b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303331227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
    const exampleAddKeyBlob = "f90186f9015eb86e7472616e73616374696f6e287075626c69634b65793a20537472696e6729207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a7369676e65722e6164645075626c69634b6579287075626c69634b65792e6465636f64654865782829290a7d0a7df8acb8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
    const exampleRegisterNodeBlob = "f904fef904d6b902da696d706f7274204c6f636b6564546f6b656e732066726f6d203078393565303139613137643065323364370a696d706f7274205374616b696e6750726f78792066726f6d203078376161643932653561303731356432310a0a7472616e73616374696f6e2869643a20537472696e672c20726f6c653a2055496e74382c206e6574776f726b696e67416464726573733a20537472696e672c206e6574776f726b696e674b65793a20537472696e672c207374616b696e674b65793a20537472696e672c20616d6f756e743a2055466978363429207b0a0a202020206c657420686f6c6465725265663a20264c6f636b6564546f6b656e732e546f6b656e486f6c6465720a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e686f6c646572526566203d206163636f756e742e626f72726f773c264c6f636b6564546f6b656e732e546f6b656e486f6c6465723e2866726f6d3a204c6f636b6564546f6b656e732e546f6b656e486f6c64657253746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f20546f6b656e486f6c64657222290a202020207d0a0a2020202065786563757465207b0a20202020202020206c6574206e6f6465496e666f203d205374616b696e6750726f78792e4e6f6465496e666f2869643a2069642c20726f6c653a20726f6c652c206e6574776f726b696e67416464726573733a206e6574776f726b696e67416464726573732c206e6574776f726b696e674b65793a206e6574776f726b696e674b65792c207374616b696e674b65793a207374616b696e674b6579290a0a202020202020202073656c662e686f6c6465725265662e6372656174654e6f64655374616b6572286e6f6465496e666f3a206e6f6465496e666f2c20616d6f756e743a20616d6f756e74290a202020207d0a7d0af901b6b85c7b2274797065223a22537472696e67222c2276616c7565223a2266303837343230346162326632666631633131323432316334396265393630386363666334386635653839306638396138666535306538663563623336643964227d9c7b2274797065223a2255496e7438222c2276616c7565223a2233227db85c7b2274797065223a22537472696e67222c2276616c7565223a2266303837343230346162326632666631633131323432316334396265393630386363666334386635653839306638396138666535306538663563623336643964227db85c7b2274797065223a22537472696e67222c2276616c7565223a2266303837343230346162326632666631633131323432316334396265393630386363666334386635653839306638396138666535306538663563623336643964227db85c7b2274797065223a22537472696e67222c2276616c7565223a2266303837343230346162326632666631633131323432316334396265393630386363666334386635653839306638396138666535306538663563623336643964227da07b2274797065223a22554669783634222c2276616c7565223a2231302e30227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a8899a8ac2c71d4f6bd040a8899a8ac2c71d4f6bdc98899a8ac2c71d4f6bde4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
    
    it('sign secp256k1 basic & verify SHA2-256 - transfer', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleTransferBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_transfer_sha2_256", 13);

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

    it('sign secp256k1 basic & verify SHA2-256 - create', async function () {
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

    it('sign secp256k1 basic & verify SHA2-256 - add new key', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleAddKeyBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_addNewKey_sha2_256", 16);

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

    it('sign secp256k1 basic & verify SHA3-256 - transfer', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA3_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleTransferBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_transfer_sha3_256", 13);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA3-256", "UINT8ARRAY");
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

    it('sign secp256k1 basic & verify SHA3-256 - create', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA3_256;
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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_create_sha3_256", 21);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA3-256", "UINT8ARRAY");
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

    it('sign secp256k1 basic & verify SHA3-256 - add new key', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA3_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleAddKeyBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_addNewKey_sha3_256", 16);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA3-256", "UINT8ARRAY");
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

    // p256

    it('get address - secp256r1', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const resp = await app.getAddressAndPubKey(path);

            console.log(resp)

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";
            const expected_pk = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);

        } finally {
            await sim.close();
        }
    });

    it('show address - secp256r1', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Derivation path. First 3 items are automatically hardened!
            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const respRequest = app.showAddressAndPubKey(path);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            await sim.compareSnapshotsAndAccept(".", "show_address_secp256r1", 5);

            const resp = await respRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";
            const expected_pk = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

    it('sign p256 basic & verify SHA2-256 - transfer', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleTransferBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_transfer_sha2_256", 13);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("p256");
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

    it('sign p256 basic & verify SHA2-256 - create', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_create_sha2_256", 21);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("p256");
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

    it('sign p256 basic & verify SHA2-256 - add new key', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleAddKeyBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_addNewKey_sha3_256", 16);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("p256");
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


    it('sign p256 basic & verify SHA2-256 - register node', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleRegisterNodeBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_registerNode_sha2_256", 16);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("p256");
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

    it('sign p256 basic & verify SHA3-256 - transfer', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA3_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleTransferBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_transfer_sha3_256", 13);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA3-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("p256");
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

    it('sign p256 basic & verify SHA3-256 - create', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA3_256;
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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_create_sha3_256", 21);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA3-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("p256");
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

    it('sign p256 basic & verify SHA3-256 - add new key', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA3_256;
            const path = `m/44'/539'/${scheme}'/0/0`;

            const txBlob = Buffer.from(
                exampleAddKeyBlob,
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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_addNewKey_sha3_256", 16);

            let resp = await signatureRequest;
            console.log(resp);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            // Prepare digest
            const hasher = new jsSHA("SHA3-256", "UINT8ARRAY");
            hasher.update(txBlob)
            const digest = hasher.getHash("HEX");

            // Verify signature
            const ec = new EC("p256");
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
