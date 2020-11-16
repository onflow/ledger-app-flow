import Zemu from "@zondax/zemu";
import FlowApp from "@onflow/ledger";
import jsSHA from "jssha";
import {ec as EC} from "elliptic";
import fs from "fs";

import { APP_PATH, simOptions, verifyAndAccept } from "./setup";

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

    return 1;
}

function getArgumentsPageCount(args) {
    return args.reduce((count, arg) => count + getArgumentPageCount(arg), 0);
}

function getTransactionBasePageCount(tx) {
    return (
        CHAIN_ID_PAGE_COUNT +
        REF_BLOCK_PAGE_COUNT +
        GAS_LIMIT_PAGE_COUNT +
        PROP_KEY_ADDRESS_PAGE_COUNT +
        PROP_KEY_ID_PAGE_COUNT +
        PROP_KEY_SEQNUM_PAGE_COUNT +
        PAYER_PAGE_COUNT +
        (AUTHORIZER_PAGE_COUNT * tx.authorizers.length) +
        ACCEPT_PAGE_COUNT
    );
}

function getTransactionPageCount(tx) {
    return getTransactionBasePageCount(tx) + getArgumentsPageCount(tx.arguments);
}

function getKeyPath(sigAlgo, hashAlgo) {
    const scheme = sigAlgo | hashAlgo;
    const path = `m/44'/539'/${scheme}'/0/0`;
    return path;
}

async function transactionTest(txHexBlob, txExpectedPageCount, sigAlgo, hashAlgo) {

    const sim = new Zemu(APP_PATH);

    try {
        await sim.start(simOptions);

        const app = new FlowApp(sim.getTransport());

        const txBlob = Buffer.from(txHexBlob, "hex");

        const path = getKeyPath(sigAlgo.code, hashAlgo.code);

        const pkResponse = await app.getAddressAndPubKey(path);
        expect(pkResponse.returnCode).toEqual(0x9000);
        expect(pkResponse.errorMessage).toEqual("No errors");

        // WARNING: do not block for this request until transaction
        // has been accepted in the Zemu emulator
        const signatureRequest = app.sign(path, txBlob);

        // Click through each approval page and accept transaction
        // Capture a snapshot of each page
        const snapshots = await verifyAndAccept(sim, txExpectedPageCount);

        // Expect new snapshots to match saved snapshots
        snapshots.forEach((image) => expect(image).toMatchImageSnapshot());

        let resp = await signatureRequest;
        expect(resp.returnCode).toEqual(0x9000);
        expect(resp.errorMessage).toEqual("No errors");

        // Prepare digest by hashing transaction
        const hasher = new jsSHA(hashAlgo.name, "UINT8ARRAY");
        hasher.update(txBlob)
        const digest = hasher.getHash("HEX");

        // Verify transaction signature against the digest
        const ec = new EC(sigAlgo.name);
        const sig = resp.signatureDER.toString("hex");
        const pk = pkResponse.publicKey.toString("hex");

        const signatureOk = ec.verify(digest, sig, pk, 'hex');
        expect(signatureOk).toEqual(true);
    } finally {
        await sim.close();
    }
}

const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256 };

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256};

const sigAlgos = [ECDSA_SECP256K1, ECDSA_P256];
const hashAlgos = [SHA2_256, SHA3_256];

const exampleTransferBlob = "f9023ff9023bb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df861b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
const exampleCreateAccountBlob = "f90289f90261b8a97472616e73616374696f6e287075626c69634b6579733a205b537472696e675d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b65792e6465636f64654865782829290a7d0a7d0a7df90173b901707b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303331227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
const exampleAddKeyBlob = "f90186f9015eb86e7472616e73616374696f6e287075626c69634b65793a20537472696e6729207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a7369676e65722e6164645075626c69634b6579287075626c69634b65792e6465636f64654865782829290a7d0a7df8acb8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"

describe("Basic transactions", () => {

    const transactions = [
        {
            title: "Transfer FLOW",
            blob: exampleTransferBlob,
            pageCount: 12,
        },
        {
            title: "Create Account",
            blob: exampleCreateAccountBlob,
            pageCount: 20,
        },
        {
            title: "Add Key",
            blob: exampleAddKeyBlob,
            pageCount: 15,
        },
    ];

    transactions.forEach((tx) => {
        sigAlgos.forEach((sigAlgo) => {
            hashAlgos.forEach((hashAlgo) => {
                test(`sign transaction (${tx.title}) - ${sigAlgo.name} / ${hashAlgo.name}`, async () => {    
                    await transactionTest(
                        tx.blob, 
                        tx.pageCount, 
                        sigAlgo, 
                        hashAlgo,
                    );
                });
            });
        });
    });
})

describe("Staking transactions", () => {
    const transactions = JSON.parse(fs.readFileSync("../tests/testvectors/manifestEnvelopeCases.json"));

    transactions
        .filter((tx) => tx.chainID === "Mainnet")
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
        });
});
