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
console.log(common.humanTime() + " // Derivation path. First 3 items are automatically hardened!");
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

console.log(common.humanTime() + " // screen shot before sending first apdu command");
await common.curlScreenShot(scriptName);

//showAddress slot 0 is empty
common.testStep(" - - -", "app.showAddressAndPubKey() // path=" + path);
const showPubkeyPromise = app.showAddressAndPubKey(path);
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [1/4] 04d7..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [2/4] ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [3/4] ..");
if (!(process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox")) {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [4/4] ..");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Not saved on the device. ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?*/
await common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const showPubkeyResponse = await showPubkeyPromise
assert.equal(showPubkeyResponse.returnCode, 0x9000);
assert.equal(showPubkeyResponse.errorMessage, "No errors");
const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";
assert.equal(showPubkeyResponse.address.toString(), expected_pk);
assert.equal(showPubkeyResponse.publicKey.toString('hex'), expected_pk);

assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "3301010014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
common.compare(hexIncomming, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});

//We set slot 0 to some other path
const account = "e467b9dd11fa00de"; //this is not a proper account but the app does not test it
const path2 = `m/44'/539'/${scheme}'/0/1`;
const slot = 0
common.testStep(" - - -", "app.setSlot() // slot=" + 0 + " account=" + account + " path=" + path2 + "; Set slot 0");
const setSlotPromise = app.setSlot(slot, account, path2);
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Set slot 0 ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Path ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; Address ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?*/
await common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const setSlotResponse = await setSlotPromise
assert.equal(setSlotResponse.returnCode, 0x9000);

assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "331200001d00e467b9dd11fa00de2c0000801b020080010200800000000001000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//Call showpubkey for original path
common.testStep(" - - -", "app.showAddressAndPubKey() // path=" + path);
const showPubkeyPromise2 = app.showAddressAndPubKey(path);
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [1/4] 04d7..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [2/4] ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [3/4] ..");
if (!(process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox")) {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [4/4] ..");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; Other path saved on the device. ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?*/
await common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const showPubkeyResponse2 = await showPubkeyPromise2
assert.equal(showPubkeyResponse2.returnCode, 0x9000);
assert.equal(showPubkeyResponse2.errorMessage, "No errors");
assert.equal(showPubkeyResponse2.address.toString(), expected_pk);
assert.equal(showPubkeyResponse2.publicKey.toString('hex'), expected_pk);

assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "3301010014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
common.compare(hexIncomming, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});

//Call showpubkey for other path
common.testStep(" - - -", "app.showAddressAndPubKey() // path=" + path2);
const showPubkeyPromise3 = app.showAddressAndPubKey(path2);
if (process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox") {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; Please review");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [1/4] 04d7..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [2/4] ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [3/4] ..");
if (!(process.env.TEST_DEVICE && process.env.TEST_DEVICE == "nanox")) {
    await common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Pub Key [4/4] ..");
}
await common.curlScreenShot(scriptName); common.curlButton('right', "; address ...");
await common.curlScreenShot(scriptName); common.curlButton('right', "; warning text screen 1 ..");
await common.curlScreenShot(scriptName); common.curlButton('right', "; warning text screen 1 ..");
await common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve"); // todo: should showAddressAndPubKey() need an 'Approve' dialog?*/
await common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
const showPubkeyResponse3 = await showPubkeyPromise3
assert.equal(showPubkeyResponse3.returnCode, 0x9000);
assert.equal(showPubkeyResponse3.errorMessage, "No errors");
const expected_pk2 = "04d525023d85460e62b82e69a1e60fadaa81338cdb4112d84fe018851283ae53727dd0233c273cb396eb477404e3125971aef36e35b52a8930b48ffe6c03ba604b";
assert.equal(showPubkeyResponse3.address.toString(), expected_pk2);
assert.equal(showPubkeyResponse3.publicKey.toString('hex'), expected_pk2);

assert.equal(transport.hexApduCommandOut.length, 1)
assert.equal(transport.hexApduCommandIn.length, 1)
var hexOutgoing = transport.hexApduCommandOut.shift();
var hexExpected = "3301010014xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, unexpected:9999});
var hexIncomming = transport.hexApduCommandIn.shift();
var hexExpected = "04d525023d85460e62b82e69a1e60fadaa81338cdb4112d84fe018851283ae53727dd0233c273cb396eb477404e3125971aef36e35b52a8930b48ffe6c03ba604b303464353235303233643835343630653632623832653639613165363066616461613831333338636462343131326438346665303138383531323833616535333732376464303233336332373363623339366562343737343034653331323539373161656633366533356235326138393330623438666665366330336261363034629000";
common.compare(hexIncomming, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});


await transport.close()
common.testEnd(scriptName);