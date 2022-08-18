'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, compareGetVersionAPDUs, noMoreAPDUs, getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { getButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { transactionTest } from "./speculos-transaction.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

/* This test is supposed to run on 0.9.12 version of ledger-app-flow */

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = getButtonsAndSnapshots(scriptName, speculosConf);

let hexExpected = "";

const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1, pathCode: 0x0200};
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256, pathCode: 0x0300};

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256, pathCode: 0x01};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256 ,pathCode: 0x03};

const sigAlgos = [ECDSA_SECP256K1, ECDSA_P256];
const hashAlgos = [SHA2_256, SHA3_256];

const expected_pk = [
    [
        "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be",
        "04fab6ad32374b117678fda1932cf57c0e086a55753109c5265d7804a3122ef180251ef9cec48f8706ab229f19dbccb143c26fc2561ad18a9f8614b7418ec37ab2",
    ],
    
    [
        "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3",
        "049ed0923564ab3f06a58300814b71836a6cf47d6da9e9f0f2ce2289b53a931ab63bfce9d876a6ec51a03493d29a8369aca27083ae89212f2c499ea5d45a82f693",
    ]
]


await device.makeStartingScreenshot();

testStep(" - - -", "await app.getVersion()");
const getVersionResponse = await app.getVersion();
assert.equal(getVersionResponse.returnCode, 0x9000);
assert.equal(getVersionResponse.errorMessage, "No errors");
assert.equal(getVersionResponse.major, 0);
assert.equal(getVersionResponse.minor, 9);
assert.equal(getVersionResponse.patch, 12)
assert.ok("testMode" in getVersionResponse)
assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)

hexExpected = "3300000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, unexpected:9999});
hexExpected = speculosConf.isNanoX ? "0000090c00330000049000" : "0000090c00311000049000";
compareInAPDU(transport, hexExpected, "apdu response", {testMode:1, major:1, minor:1, patch:1, deviceLocked:1, targetId:4, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);


for (let i=0; i < sigAlgos.length; ++i ) {
    for (let j=0; j < hashAlgos.length; ++j ) {
        //Test pubkey derivation
        const path = `m/44'/539'/${sigAlgos[i].pathCode | hashAlgos[j].pathCode}'/0/0`;
        testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path);
        const options = sigAlgos[i].code | hashAlgos[j].code;

        const getPubkeyResponse = await app.getAddressAndPubKey(path, options);
        assert.equal(getPubkeyResponse.returnCode, 0x9000);
        assert.equal(getPubkeyResponse.errorMessage, "No errors");
        
        assert.equal(getPubkeyResponse.address.toString(), expected_pk[i][j]);
        assert.equal(getPubkeyResponse.publicKey.toString('hex'), expected_pk[i][j]);
        
        compareGetVersionAPDUs(transport);
        hexExpected = "3301000014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
        compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
        hexExpected = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx9000";
        compareInAPDU(transport, hexExpected, "apdu response", {do_not_compare_publicKey:65, do_not_compare_publicKey_hex:130, returnCode:2, unexpected:9999});
        noMoreAPDUs(transport);

        //Test tx being signed correctly
        const exampleAddKeyBlob        = "f90186f9015eb86e7472616e73616374696f6e287075626c69634b65793a20537472696e6729207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a7369676e65722e6164645075626c69634b6579287075626c69634b65792e6465636f64654865782829290a7d0a7df8acb8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"

        await transactionTest(
            app,
            transport,
            device,
            exampleAddKeyBlob, 
            sigAlgos[i],
            hashAlgos[j],
            12
        ); 
    }
}


//Test setSlot
const expectedSlot = 10;
const expectedAccount = "e467b9dd11fa00df";
const expectedPath = `m/44'/539'/513'/0/0`;
testStep(" - - -", "app.setSlot()");
const setSlotPromise1 = app.setSlot(10, expectedAccount, expectedPath, 0);
await device.review("Set slot 10");
const setSlotResponse1 = await setSlotPromise1;

assert.equal(setSlotResponse1.returnCode, 0x9000);

compareGetVersionAPDUs(transport);
hexExpected = "331200001d0ae467b9dd11fa00df2c0000801b020080010200800000000000000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);


//Test getSlot
testStep(" - - -", "await app.getSlot(); Get slot 10 back");
const getSlotResponse2 = await app.getSlot(expectedSlot);

assert.equal(getSlotResponse2.returnCode, 0x9000);
assert.equal(getSlotResponse2.account, expectedAccount);
assert.equal(getSlotResponse2.path, expectedPath);

hexExpected = "33110000010a";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, unexpected:9999});
hexExpected = "e467b9dd11fa00df2c0000801b0200800102008000000000000000009000";
compareInAPDU(transport, hexExpected, "apdu response", {account:8, path:20, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);



await transport.close()
testEnd(scriptName);
process.stdin.pause()
