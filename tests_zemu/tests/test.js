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
import {ec as EC} from "elliptic";

const Resolve = require("path").resolve;
const APP_PATH = Resolve("../app/bin/app.elf");

const APP_SEED = "equip will roof matter pink blind book anxiety banner elbow sun young"
const simOptions = {
    logging: true,
    start_delay: 3000,
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

    it('slot status - new', async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            let resp = await app.slotStatus();
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");
            expect(resp).toHaveProperty("status");

            let expectedBuffer = Buffer.alloc(64);
            expectedBuffer.fill(0);
            expect(resp.status).toEqual(expectedBuffer);

            // Get empty slot should error
            let respSlot = await app.getSlot(3);
            console.log(respSlot);
            expect(respSlot.returnCode).toEqual(0x6982);
            expect(respSlot.errorMessage).toEqual("Empty Buffer");

            // Set slot 10
            const expectedAccount = "0001020304050607"
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const expectedPath = `m/44'/539'/${scheme}'/0/0`;
            respSlot = await app.setSlot(10, expectedAccount, expectedPath);
            console.log(respSlot);
            expect(resp.returnCode).toEqual(0x9000);

            // Get slot status
            resp = await app.slotStatus();
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");
            expectedBuffer = Buffer.alloc(64);
            expectedBuffer.fill(0);
            expectedBuffer[10] = 1;
            expect(resp.status).toEqual(expectedBuffer);

            // Get empty slot should error
            respSlot = await app.getSlot(10);
            console.log(respSlot);
            expect(respSlot.returnCode).toEqual(0x9000);
            expect(respSlot.account).toEqual(expectedAccount);
            expect(respSlot.path).toEqual(expectedPath);

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

    const exampleTransferBlob = "f90256f9022eb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df854a37b2274797065223a22554669783634222c2276616c7565223a22333132382e3737227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
    const exampleCreateBlob = "f9030af902e2b8a97472616e73616374696f6e287075626c69634b6579733a205b537472696e675d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b65792e6465636f64654865782829290a7d0a7d0a7df901f4b901f17b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a226130393930353063646633383130346436636136643039653138643537623363303833376436323539666433653464636236343932656566383132616536626136376539633763626139356630393264373466353162346334393162353837663635393035643566303436363035336265663634616238643834393134343565227d2c7b2274797065223a22537472696e67222c2276616c7565223a226631306537323535323135393931356336383735306362343733626636626236643164396130386133643066623634663537393837346237353837346636653166363239623266613739343963323830346437663639346666356361323135336565356536633963616363653733313537366666623234663036666164363265227d2c7b2274797065223a22537472696e67222c2276616c7565223a226561316562356439313230666632653538356161386261643562663632663661303034653335643236636130313636626363623632336132313236393935646362646638353730393537656661396364613230633662346438646435346335386363393439353262653233366166303931346533313637623036643063383033227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
    const exampleAddKeyBlob = "f9019cf90174b8927472616e73616374696f6e287075626c69634b65793a20537472696e6729207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a616363742e6164645075626c69634b6579287075626c69634b65792e6465636f64654865782829290a7d0a7df89eb89c7b2274797065223a22537472696e67222c2276616c7565223a226536653535393565646265623930323164663036316434323038326262383266336463396637306662363931623361336333393338376361653232343433326230666535663662326666626330346534656336343766373532616461376566373335376230303930376134636536346433313136353437643461313766326438227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"

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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_transfer_sha2_256", 12);

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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_create_sha2_256", 22);

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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_addNewKey_sha3_256", 14);

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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_transfer_sha3_256", 12);

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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_create_sha3_256", 22);

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

            await sim.compareSnapshotsAndAccept(".", "sign_secp256k1_basic_verify_addNewKey_sha3_256", 14);

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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_transfer_sha2_256", 12);

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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_create_sha2_256", 22);

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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_addNewKey_sha3_256", 14);

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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_transfer_sha3_256", 12);

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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_create_sha3_256", 22);

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

            await sim.compareSnapshotsAndAccept(".", "sign_p256_basic_verify_addNewKey_sha3_256", 14);

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
