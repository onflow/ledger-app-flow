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

import FlowApp from "../src";
import TransportNodeHid from "@ledgerhq/hw-transport-node-hid";
import chai, { expect } from "chai"
import jsSHA from "jssha";
import {ec as EC} from "elliptic";
import fs from "fs";

jest.setTimeout(1000000);

const CHAIN_ID_PAGE_COUNT = 1;
const REF_BLOCK_PAGE_COUNT = 2;
const GAS_LIMIT_PAGE_COUNT = 1;
const PROP_KEY_ADDRESS_PAGE_COUNT = 1;
const PROP_KEY_ID_PAGE_COUNT = 1;
const PROP_KEY_SEQNUM_PAGE_COUNT = 1;
const PAYER_PAGE_COUNT = 1;
const AUTHORIZER_PAGE_COUNT = 1;
const ACCEPT_PAGE_COUNT = 1;

const PAGE_SIZE = 34;

function getArgumentPageCount(arg) {
    if (arg.type == "Array") {
        return getArgumentsPageCount(arg.value);
    }

    if (arg.type == "String") {
        const count = Math.ceil(arg.value.length / PAGE_SIZE);
        return count;
    }

    if (arg.type == "Optional" && arg.value != null) {
        return getArgumentPageCount(arg.value);
    }

    return 1;
}

function getArgumentsPageCount(args) {
    return args.reduce((count, arg) => count + getArgumentPageCount(arg), 0);
}

function getTransactionPageCount(tx) {
    return (
        CHAIN_ID_PAGE_COUNT +
        REF_BLOCK_PAGE_COUNT +
        getArgumentsPageCount(tx.arguments) +
        GAS_LIMIT_PAGE_COUNT +
        PROP_KEY_ADDRESS_PAGE_COUNT +
        PROP_KEY_ID_PAGE_COUNT +
        PROP_KEY_SEQNUM_PAGE_COUNT +
        PAYER_PAGE_COUNT +
        (AUTHORIZER_PAGE_COUNT * tx.authorizers.length) +
        ACCEPT_PAGE_COUNT
    );
}

function getKeyPath(sigAlgo, hashAlgo) {
    const scheme = sigAlgo | hashAlgo;
    const path = `m/44'/539'/${scheme}'/0/0`;
    return path;
}

async function transactionTest(txHexBlob, txExpectedPageCount, sigAlgo, hashAlgo) {

        const transport = await TransportNodeHid.create();

        const app = new FlowApp(transport);


        const txBlob = Buffer.from(txHexBlob, "hex");

        const path = getKeyPath(sigAlgo.code, hashAlgo.code);
        
        const res1 = await app.setSlot(5, "e467b9dd11fa00df", path)
        expect(res1.returnCode).equal(0x9000)
        const res2 = await app.getAddressAndPubKey(5)
        expect(res2.returnCode).equal(0x9000)
        expect(res2.errorMessage).equal("No errors");

        let resp = await app.sign(path, txBlob);

        expect(resp.returnCode).equal(0x9000);
        expect(resp.errorMessage).equal("No errors");

        // Prepare digest by hashing transaction
        let tag = Buffer.alloc(32);
        tag.write("FLOW-V0.0-transaction");

        const hasher = new jsSHA(hashAlgo.name, "UINT8ARRAY");

        hasher.update(tag);
        hasher.update(txBlob);

        const digest = hasher.getHash("HEX");

        // Verify transaction signature against the digest
        const ec = new EC(sigAlgo.name);
        const sig = resp.signatureDER.toString("hex");
        const pk = res2.publicKey.toString("hex");

        const signatureOk = ec.verify(digest, sig, pk, 'hex');
        expect(signatureOk).equal(true);
        
        await transport.close();
}

const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256 };

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256};

const sigAlgos = [ECDSA_SECP256K1, ECDSA_P256];
const hashAlgos = [SHA2_256, SHA3_256];

describe("Staking transactions", () => {
    const transactions = JSON.parse(fs.readFileSync("../tests/testvectors/manifestEnvelopeCases.json"));

    //set of encodedTransactionEnvelope to test
    const to_test = new Set([
        "f9029ef9029ab90226696d706f727420466c6f7749445461626c655374616b696e672066726f6d203078383632346235326639646463643034610a0a7472616e73616374696f6e286e6577416464726573733a20537472696e6729207b0a0a202020202f2f204c6f63616c207661726961626c6520666f722061207265666572656e636520746f20746865206e6f6465206f626a6563740a202020206c6574207374616b65725265663a2026466c6f7749445461626c655374616b696e672e4e6f64655374616b65720a0a202020207072657061726528616363743a20417574684163636f756e7429207b0a20202020202020202f2f20626f72726f772061207265666572656e636520746f20746865206e6f6465206f626a6563740a202020202020202073656c662e7374616b6572526566203d20616363742e626f72726f773c26466c6f7749445461626c655374616b696e672e4e6f64655374616b65723e2866726f6d3a20466c6f7749445461626c655374616b696e672e4e6f64655374616b657253746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f77207265666572656e636520746f207374616b696e672061646d696e22290a202020207d0a0a2020202065786563757465207b0a0a202020202020202073656c662e7374616b65725265662e7570646174654e6574776f726b696e6741646472657373286e657741646472657373290a0a202020207d0a7df0af7b2274797065223a22537472696e67222c2276616c7565223a22666c6f772d6e6f64652e746573743a33353639227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4c988f19c161bc24cf4b4c0",
        "f90343f9033fb9026c696d706f727420466c6f775374616b696e67436f6c6c656374696f6e2066726f6d203078386430653837623635313539616536330a0a2f2f2f204368616e67657320746865206e6574776f726b696e67206164647265737320666f722074686520737065636966696564206e6f64650a0a7472616e73616374696f6e286e6f646549443a20537472696e672c206e6577416464726573733a20537472696e6729207b0a202020200a202020206c6574207374616b696e67436f6c6c656374696f6e5265663a2026466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e0a0a2020202070726570617265286163636f756e743a20417574684163636f756e7429207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e526566203d206163636f756e742e626f72726f773c26466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e3e2866726f6d3a20466c6f775374616b696e67436f6c6c656374696f6e2e5374616b696e67436f6c6c656374696f6e53746f7261676550617468290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f772072656620746f205374616b696e67436f6c6c656374696f6e22290a202020207d0a0a2020202065786563757465207b0a202020202020202073656c662e7374616b696e67436f6c6c656374696f6e5265662e7570646174654e6574776f726b696e6741646472657373286e6f646549443a206e6f646549442c206e6577416464726573733a206e657741646472657373290a202020207d0a7d0af88eb85c7b2274797065223a22537472696e67222c2276616c7565223a2238383534393333356531646237623562343663326164353864646237306237613435653737306363356665373739363530626132366631306536626165356536227daf7b2274797065223a22537472696e67222c2276616c7565223a22666c6f772d6e6f64652e746573743a33353639227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4c988f19c161bc24cf4b4c0"
    ]);

    transactions
        .filter((tx) => to_test.has(tx.encodedTransactionEnvelopeHex))
        .forEach((tx) => {        
            test(`sign transaction (${tx.title}) - ${ECDSA_P256.name} / ${SHA3_256.name}`, async () => {
                const txExpectedPageCount = getTransactionPageCount(tx.envelopeMessage);

                await transactionTest(
                    tx.encodedTransactionEnvelopeHex, 
                    txExpectedPageCount, 
                    ECDSA_P256, 
                    SHA3_256,
                );
            });
        }, 100000);
});

describe("Show address", () => {
    test("Test", async () => {
        const transport = await TransportNodeHid.create();
        const app = new FlowApp(transport);
        const res1 = await app.setSlot(0, "e467b9dd11fa00df", "m/44\'/539\'/512\'/0/0")
        console.log(res1)
        const res2 = await app.showAddressAndPubKey(0)
        console.log(res2)
    })
})


