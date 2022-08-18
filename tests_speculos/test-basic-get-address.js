'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs, getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { getButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = getButtonsAndSnapshots(scriptName, speculosConf);

const options = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
let hexExpected = "";

await device.makeStartingScreenshot();

const paths = [
    `m/44'/539'/0'/0/0`, 
    `m/44'/539'/0'/0/${0x7FFFFFFF}`, 
    `m/44'/539'/${0x7FFFFFFF}'/0/0`, 
    `m/44'/539'/${0x7FFFFFFF}'/0/${0x7FFFFFFF}`
];
const optionsCurve = [FlowApp.Signature.SECP256K1, FlowApp.Signature.P256];
const optionsHash = [FlowApp.Hash.SHA2_256, FlowApp.Hash.SHA3_256];
const expectedPathHex = [
    "2c0000801b020080000000800000000000000000", 
    "2c0000801b02008000000080000000007FFFFFFF", 
    "2c0000801b020080ffffffff0000000000000000", 
    "2c0000801b020080ffffffff000000007FFFFFFF"
];
const expectedCurveHex = ["03", "02"];
const expectedhashHex = ["01", "03"];
const expectedPubkey = [
    [
        "040c77dedaa9a718b1eda82b66076ea44bb04dd0c6f583dc89a0d7fca32d12f672b659a8f8af25f1d0b74574b830f835ac6b4aacf6bbcd12f4554524149c54592d",
        "043c5762d024fa87a71b92710d5f27c987669e3419179f2129f369a4dca451583382cb6252a5046a2db2af6a30c248c3ee828506c1190bbaf1bce631d345ca6968"
    ],
    [
        "04e77443015147d9527fffa716b2feff94cf1685b65af5047d8fc8f1e3ec4b7a8c136a309f5ad4405214f3357d97320d2e6272d3e2eba4ea387f21e046c8be0cd7",
        "0447aa8d233210ecb44945c5a2210892fd0a90beb89a18fcacdaea969d36358b8bfa8b5a37b1a458e53a2668e0198f506862188ae13d9fb4a103c89ba5ff60d8ac"
    ],
    [
        "04410415a045f9c7be286dfba64cb4b2c8d16676f1968f83825b32b45d418d0c948d765eaaa9a58418708f54a1a18a6c07d53394b5bf5c9b49dec981545dc57f2c",
        "045f5bf140f0ce32ec50e67b591c22dbdf7bc588f617e4f88de74f7494a1cde2566e4976f68a5dc9c07e6f2714d01b7d822e64b4c6c92ca012ab6a0d3c8f1a1c08"
    ],
    [
        "040470d7395e71c6f3c769fa8a28349bb0edc23008e71197cce340da8a8404f1dc0619ee21d47704625fe0aa98c29b215f36717389431f597217f6b05c848d292d",
        "040a3c2082a87af4be48a92eec662843842ff985e8b3cf5bd3b510e23021e0b4bac77af9248b85ff1155e1910fb29383563b5895150763921ec2516159dcbdae85"
    ],
];

let testVectors = [];
for(let i=0; i<paths.length; i++) {
    for(let j=0; j<optionsCurve.length; j++) {
        for(let k=0; k<optionsHash.length; k++) {
            testVectors.push({
                path: paths[i],
                options: optionsCurve[j] | optionsHash[k],
                expectedPublicKey: expectedPubkey[i][j],
                expectedAPDUOut: "3301000016"+expectedPathHex[i]+expectedhashHex[k]+expectedCurveHex[j],
                expectedAPDUIn: expectedPubkey[i][j] + Buffer.from(expectedPubkey[i][j]).toString("hex") + "9000"
            })
        }
    }
}

testVectors.push({
    path: `m/44'/1'/769/0/0`,
    options:  FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256,
    expectedPublicKey: "0494488a795a07700c6fb83e066cf57dfd87f92ce70cbc81cb3bd3fea2df7b67073b70e36b44f3578b43d64d3faa2e8e415ef6c2b5fe4390d5a78e238581c6e4bc",
    expectedAPDUOut: "33010000162c000080010000800103000000000000000000000102",
    expectedAPDUIn: "0494488a795a07700c6fb83e066cf57dfd87f92ce70cbc81cb3bd3fea2df7b67073b70e36b44f3578b43d64d3faa2e8e415ef6c2b5fe4390d5a78e238581c6e4bc303439343438386137393561303737303063366662383365303636636635376466643837663932636537306362633831636233626433666561326466376236373037336237306533366234346633353738623433643634643366616132653865343135656636633262356665343339306435613738653233383538316336653462639000"
})

testVectors.push({
    path: `m/44'/1'/769/0/${0x7FFFFFFF}`,
    options:  FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256,
    expectedPublicKey: "04fe9cdee069d113f31187aaff564cce44feb1438a3fa99fc08cef06c0c03abb007a0935a9900af65879b9b526c547a576f76f5f0c84841d1de27a158bfbe71dac",
    expectedAPDUOut: "33010000162c000080010000800103000000000000ffffff7f0102",
    expectedAPDUIn: "04fe9cdee069d113f31187aaff564cce44feb1438a3fa99fc08cef06c0c03abb007a0935a9900af65879b9b526c547a576f76f5f0c84841d1de27a158bfbe71dac303466653963646565303639643131336633313138376161666635363463636534346665623134333861336661393966633038636566303663306330336162623030376130393335613939303061663635383739623962353236633534376135373666373666356630633834383431643164653237613135386266626537316461639000"
})

//get pubkey
for(const tv of testVectors) {
    testStep(" - - -", "await app.getAddressAndPubKey() // path=" + tv.path + " options=" + tv.options);
    const getPubkeyResponse = await app.getAddressAndPubKey(tv.path, tv.options);
    assert.equal(getPubkeyResponse.returnCode, 0x9000);
    assert.equal(getPubkeyResponse.errorMessage, "No errors");

    assert.equal(getPubkeyResponse.address.toString(), tv.expectedPublicKey);
    assert.equal(getPubkeyResponse.publicKey.toString('hex'), tv.expectedPublicKey);

    compareGetVersionAPDUs(transport);
    compareOutAPDU(transport, tv.expectedAPDUOut, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
    compareInAPDU(transport, tv.expectedAPDUIn, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});
    noMoreAPDUs(transport);
}

await transport.close()
testEnd(scriptName);
process.stdin.pause()
