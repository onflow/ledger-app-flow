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

import { expect } from "jest";
import Zemu from "@zondax/zemu";

import FlowApp from "@onflow/ledger";


import { APP_PATH, simOptions, verifyAndAccept, prepareSlot } from "./setup";

describe("Basic checks", function () {
   test("can start and stop container", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
        } finally {
            await sim.close();
        }
    });

    test("app version", async function () {
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

    test("sign basic - invalid", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "0123456789abcdef"

            await prepareSlot(sim, app, 1, address, path)

            let invalidMessage = Buffer.from(
                "1234567890",
                "hex",
            );
            invalidMessage += "1";

            const pkResponse = await app.getAddressAndPubKey(1);
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

    test("slot status - update", async function () {
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
            const snapshots = await verifyAndAccept(sim, 5);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            let resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });

    test("slot status - delete", async function () {
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
            const snapshots = await verifyAndAccept(sim, 3);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            let resp = await respRequest;
            console.log(resp);
            expect(resp.returnCode).toEqual(0x9000);

        } finally {
            await sim.close();
        }
    });

    test("slot status - full", async function () {
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
            const snapshots = await verifyAndAccept(sim, 3);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

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

    test("get address - secp256k1", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "0123456789abcdef"

            await prepareSlot(sim, app, 0, address, path)

            const resp = await app.getAddressAndPubKey(0);

            console.log(resp)

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "0123456789abcdef";
            const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);

        } finally {
            await sim.close();
        }
    });

     test("show address - secp256k1", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Derivation path. First 3 items are automatically hardened!
            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "0123456789abcdef"

            await prepareSlot(sim, app, 63, address, path)

            const respRequest = app.showAddressAndPubKey(63);
            const snapshots = await verifyAndAccept(sim, 7);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());
                        
            const resp = await respRequest;

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "0123456789abcdef";
            const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

    // p256

    test("get address - secp256r1", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "0123456789abcdef"

            await prepareSlot(sim, app, 0, address, path)

            const resp = await app.getAddressAndPubKey(0);

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "0123456789abcdef";
            const expected_pk = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);

        } finally {
            await sim.close();
        }
    });

    test("show address - secp256r1", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            // Derivation path. First 3 items are automatically hardened!
            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "0123456789abcdef"
            const fakeAddress = "0000000000000000"

            await prepareSlot(sim, app, 1, address, path)
            await prepareSlot(sim, app, 0, fakeAddress, path)

            const respRequest = app.showAddressAndPubKey(1);
            // Wait until we are not in the main menu
            await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot());

            // Now navigate the address / path
            const snapshots = await verifyAndAccept(sim, 7);
            snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

            const resp = await respRequest;

            expect(resp.returnCode).toEqual(0x9000);
            expect(resp.errorMessage).toEqual("No errors");

            const expected_address_string = "0123456789abcdef";
            const expected_pk = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

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
            const address = "0123456789abcdef"

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

            const expected_address_string = "0123456789abcdef";
            const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

            expect(resp.address).toEqual(expected_address_string);
            expect(resp.publicKey.toString('hex')).toEqual(expected_pk);
        } finally {
            await sim.close();
        }
    });

    test("get address - empty slot", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
            const path = `m/0/0/0/0/0`;
            const address = "0000000000000000"

            await prepareSlot(sim, app, 0, address, path)

            const resp = await app.getAddressAndPubKey(0);

            console.log(resp);

            expect(resp.returnCode).toEqual(0x6984); 
            expect(resp.errorMessage).toEqual("Data is invalid");
        } finally {
            await sim.close();
        }
    });
});
