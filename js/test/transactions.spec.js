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

async function transactionTest(app, path, publicKey, txHexBlob, txExpectedPageCount, sigAlgo, hashAlgo) {
        const txBlob = Buffer.from(txHexBlob, "hex");

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
        const pk = publicKey.toString("hex");

        const signatureOk = ec.verify(digest, sig, pk, 'hex');
        expect(signatureOk).equal(true);
}

const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256 };

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256};

const sigAlgos = [ECDSA_SECP256K1, ECDSA_P256];
const hashAlgos = [SHA2_256, SHA3_256];

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

describe("Staking transactions", () => {
    const transactions = JSON.parse(fs.readFileSync("../tests/testvectors/manifestEnvelopeCases.json"));

    //set of encodedTransactionEnvelope to test
    const to_test = new Set([
        "f9075cf90758b906b32f2f2054686973207472616e73616374696f6e2077697468647261777320465553442066726f6d20746865207369676e65722773206163636f756e7420616e64206465706f7369747320697420696e746f206120726563697069656e74206163636f756e742e200a2f2f2054686973207472616e73616374696f6e2077696c6c206661696c2069662074686520726563697069656e7420646f6573206e6f74206861766520616e20465553442072656365697665722e200a2f2f204e6f2066756e647320617265207472616e73666572726564206f72206c6f737420696620746865207472616e73616374696f6e206661696c732e0a2f2f0a2f2f20506172616d65746572733a0a2f2f202d20616d6f756e743a2054686520616d6f756e74206f66204655534420746f207472616e736665722028652e672e2031302e30290a2f2f202d20746f3a2054686520726563697069656e74206163636f756e7420616464726573732e0a2f2f0a2f2f2054686973207472616e73616374696f6e2077696c6c206661696c20696620656974686572207468652073656e646572206f7220726563697069656e7420646f6573206e6f7420686176650a2f2f20616e2046555344207661756c742073746f72656420696e207468656972206163636f756e742e20546f20636865636b20696620616e206163636f756e74206861732061207661756c740a2f2f206f7220696e697469616c697a652061206e6577207661756c742c2075736520636865636b5f667573645f7661756c745f73657475702e63646320616e642073657475705f667573645f7661756c742e6364630a2f2f20726573706563746976656c792e0a0a696d706f72742046756e6769626c65546f6b656e2066726f6d203078663233336463656538386665306162650a696d706f727420465553442066726f6d203078336335393539623536383839363339330a0a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a0a202020202f2f20546865205661756c74207265736f75726365207468617420686f6c64732074686520746f6b656e73207468617420617265206265696e67207472616e736665727265640a202020206c65742073656e745661756c743a204046756e6769626c65546f6b656e2e5661756c740a0a2020202070726570617265287369676e65723a20417574684163636f756e7429207b0a20202020202020202f2f204765742061207265666572656e636520746f20746865207369676e657227732073746f726564207661756c740a20202020202020206c6574207661756c74526566203d207369676e65722e626f72726f773c26465553442e5661756c743e2866726f6d3a202f73746f726167652f667573645661756c74290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f77207265666572656e636520746f20746865206f776e65722773205661756c742122290a0a20202020202020202f2f20576974686472617720746f6b656e732066726f6d20746865207369676e657227732073746f726564207661756c740a202020202020202073656c662e73656e745661756c74203c2d207661756c745265662e776974686472617728616d6f756e743a20616d6f756e74290a202020207d0a0a2020202065786563757465207b0a20202020202020202f2f204765742074686520726563697069656e742773207075626c6963206163636f756e74206f626a6563740a20202020202020206c657420726563697069656e74203d206765744163636f756e7428746f290a0a20202020202020202f2f204765742061207265666572656e636520746f2074686520726563697069656e7427732052656365697665720a20202020202020206c6574207265636569766572526566203d20726563697069656e742e6765744361706162696c697479282f7075626c69632f66757364526563656976657229212e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e28290a2020202020202020202020203f3f2070616e69632822436f756c64206e6f7420626f72726f77207265636569766572207265666572656e636520746f2074686520726563697069656e742773205661756c7422290a0a20202020202020202f2f204465706f736974207468652077697468647261776e20746f6b656e7320696e2074686520726563697069656e7427732072656365697665720a202020202020202072656365697665725265662e6465706f7369742866726f6d3a203c2d73656c662e73656e745661756c74290a202020207d0a7d0af860b07b2274797065223a22554669783634222c2276616c7565223a2239323233333732303336382e3534373735383038227dae7b2274797065223a22537472696e67222c2276616c7565223a22307865343637623964643131666130306466227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f19c161bc24cf4b4040a88f19c161bc24cf4b4c988f19c161bc24cf4b4c0",
    ]);

    test(`sign transaction`, async () => {
        const transport = await TransportNodeHid.create();
        const app = new FlowApp(transport);
       
        const sigAlgo = ECDSA_P256;
        const hashAlgo = SHA3_256;

        const path = getKeyPath(sigAlgo.code, hashAlgo.code);
        console.log(path)
        
        const res1 = await app.setSlot(5, "e467b9dd11fa00df", path)
        console.log(res1)
        expect(res1.returnCode).equal(0x9000)
    
        const res2 = await app.getAddressAndPubKey(5)
        console.log(res2)
        expect(res2.returnCode).equal(0x9000)
        expect(res2.errorMessage).equal("No errors");

        for(const tx of transactions.filter((tx) => to_test.has(tx.encodedTransactionEnvelopeHex))) {
            console.log("${tx.title} - ${ECDSA_P256.name} / ${SHA3_256.name}")
            const txExpectedPageCount = getTransactionPageCount(tx.envelopeMessage);

            await transactionTest(
                app,
                path,
                res2.publicKey,
                tx.encodedTransactionEnvelopeHex, 
                txExpectedPageCount, 
                sigAlgo, 
                hashAlgo
            );
        }

        await transport.close();
    }, 1000000);

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


