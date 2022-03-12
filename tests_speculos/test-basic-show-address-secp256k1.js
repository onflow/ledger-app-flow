'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs, getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { ButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = new ButtonsAndSnapshots(scriptName, speculosConf);
let hexExpected = "";

await device.makeStartingScreenshot();

//showAddress slot 0 is empty
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;
const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";

testStep("======", "app.showAddressAndPubKey() // Empty slot 0");
const showPubkeyPromise = app.showAddressAndPubKey(path);
device.review("Show address 1 - empty slot");
const showPubkeyResponse = await showPubkeyPromise;

assert.equal(showPubkeyResponse.returnCode, 0x9000);
assert.equal(showPubkeyResponse.errorMessage, "No errors");
assert.equal(showPubkeyResponse.address.toString(), expected_pk);
assert.equal(showPubkeyResponse.publicKey.toString('hex'), expected_pk);

compareGetVersionAPDUs(transport);
hexExpected = "3301010016xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
compareInAPDU(transport, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);


//We set slot 0 to some other path
const account = "e467b9dd11fa00de"; //this is not a proper account but the app does not test it
const path2 = `m/44'/539'/${scheme}'/0/1`;
const slot = 0;

testStep("======", "app.setSlot() // Set slot");
const setSlotPromise = app.setSlot(slot, account, path2);
device.review("Set slot 0");
const setSlotResponse = await setSlotPromise;
assert.equal(setSlotResponse.returnCode, 0x9000);

compareGetVersionAPDUs(transport);
hexExpected = "331200001f00e467b9dd11fa00de2c0000801b0200800102008000000000010000000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:30, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//Call showpubkey for original path
testStep("======", "app.showAddressAndPubKey() // Other path saved");
const showPubkeyPromise2 = app.showAddressAndPubKey(path);
device.review("Show address 2 - different path on slot");
const showPubkeyResponse2 = await showPubkeyPromise2
assert.equal(showPubkeyResponse2.returnCode, 0x9000);
assert.equal(showPubkeyResponse2.errorMessage, "No errors");
assert.equal(showPubkeyResponse2.address.toString(), expected_pk);
assert.equal(showPubkeyResponse2.publicKey.toString('hex'), expected_pk);

compareGetVersionAPDUs(transport);
hexExpected = "3301010016xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
compareInAPDU(transport, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//Call showpubkey for other path
testStep("======", "app.showAddressAndPubKey() // Saved path");
const showPubkeyPromise3 = app.showAddressAndPubKey(path2);
device.review("Show address 3 - path saved on slot");
const showPubkeyResponse3 = await showPubkeyPromise3
assert.equal(showPubkeyResponse3.returnCode, 0x9000);
assert.equal(showPubkeyResponse3.errorMessage, "No errors");
const expected_pk2 = "04d525023d85460e62b82e69a1e60fadaa81338cdb4112d84fe018851283ae53727dd0233c273cb396eb477404e3125971aef36e35b52a8930b48ffe6c03ba604b";
assert.equal(showPubkeyResponse3.address.toString(), expected_pk2);
assert.equal(showPubkeyResponse3.publicKey.toString('hex'), expected_pk2);

compareGetVersionAPDUs(transport);
hexExpected = "3301010016xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
hexExpected = "04d525023d85460e62b82e69a1e60fadaa81338cdb4112d84fe018851283ae53727dd0233c273cb396eb477404e3125971aef36e35b52a8930b48ffe6c03ba604b303464353235303233643835343630653632623832653639613165363066616461613831333338636462343131326438346665303138383531323833616535333732376464303233336332373363623339366562343737343034653331323539373161656633366533356235326138393330623438666665366330336261363034629000";
compareInAPDU(transport, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

//Delete slot 0
const expectedAccountDelete = "0000000000000000";
const expectedPathDelete = `m/0/0/0/0/0`;
testStep("======", "app.setSlot() // Delete slot");
const setSlotPromise3 = app.setSlot(slot, expectedAccountDelete, expectedPathDelete);
device.review("Delete slot");
const setSlotResponse3 = await setSlotPromise3;
assert.equal(setSlotResponse3.returnCode, 0x9000);

compareGetVersionAPDUs(transport);
hexExpected = "331200001f00000000000000000000000000000000000000000000000000000000000000";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:30, unexpected:9999});
hexExpected = "9000";
compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

await transport.close()
testEnd(scriptName);