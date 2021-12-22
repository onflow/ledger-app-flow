'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const transport = await common.getSpyTransport()
const app = new FlowApp(transport);

console.log(common.humanTime() + " // using FlowApp below with transport() to grab apdu command without sending it");
const scheme = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

console.log(common.humanTime() + " // screen shot before sending first apdu command");
common.curlScreenShot(scriptName);

//get pubkey
common.testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path);
const getPubkeyResponse = await app.getAddressAndPubKey(path);
assert.equal(getPubkeyResponse.returnCode, 0x9000);
assert.equal(getPubkeyResponse.errorMessage, "No errors");
const expected_pk = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3";
assert.equal(getPubkeyResponse.address, expected_pk);
assert.equal(getPubkeyResponse.publicKey.toString('hex'), expected_pk);

assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "3301000014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "04db0a14364e5bf43a7ddda603522ddfee95c5ff12b48c49480f062e7aa9d20e84215eef9b8b76175f32802f75ed54110e29c7dc76054f24c028c312098e7177a3303464623061313433363465356266343361376464646136303335323264646665653935633566663132623438633439343830663036326537616139643230653834323135656566396238623736313735663332383032663735656435343131306532396337646337363035346632346330323863333132303938653731373761339000";
common.compare(hexIncomming, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});

await transport.close()
common.testEnd(scriptName);