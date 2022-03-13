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

const options = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${0x201}'/0/0`;
let hexExpected = "";

await device.makeStartingScreenshot();

//get pubkey
testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path + " options=" + options);
const getPubkeyResponse = await app.getAddressAndPubKey(path, options);
assert.equal(getPubkeyResponse.returnCode, 0x9000);
assert.equal(getPubkeyResponse.errorMessage, "No errors");

const expected_pk = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be";
assert.equal(getPubkeyResponse.address.toString(), expected_pk);
assert.equal(getPubkeyResponse.publicKey.toString('hex'), expected_pk);

compareGetVersionAPDUs(transport);
hexExpected = "3301000016xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0103";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
hexExpected = "04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be303464373438326262616666373832373033356435623233386466333138623130363034363733646336313338303837323365666264323366626334623966616433346134313538323864393234656337623833616330656464663232656631313562376332303365653339666230383035373264376535313737356565353462659000";
compareInAPDU(transport, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path + " options=" + options);
const options2 = FlowApp.Signature.P256 | FlowApp.Hash.SHA2_256;
const getPubkeyResponse2 = await app.getAddressAndPubKey(path, options2);
assert.equal(getPubkeyResponse2.returnCode, 0x9000);
assert.equal(getPubkeyResponse2.errorMessage, "No errors");

const expected_pk2 = "04e6736d9f4952a92aced4fb4f6a9bdf23c5db7057a8325d2303167df8fbc9a8e9c08e9c520881bc48fb23b8ae53d1efa8b24dda6555937ab049f557c65228857e";
assert.equal(getPubkeyResponse2.address.toString(), expected_pk2);
assert.equal(getPubkeyResponse2.publicKey.toString('hex'), expected_pk2);

compareGetVersionAPDUs(transport);
hexExpected = "3301000016xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx0102";
compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_path:20, options:2, unexpected:9999});
hexExpected = "04e6736d9f4952a92aced4fb4f6a9bdf23c5db7057a8325d2303167df8fbc9a8e9c08e9c520881bc48fb23b8ae53d1efa8b24dda6555937ab049f557c65228857e303465363733366439663439353261393261636564346662346636613962646632336335646237303537613833323564323330333136376466386662633961386539633038653963353230383831626334386662323362386165353364316566613862323464646136353535393337616230343966353537633635323238383537659000";
compareInAPDU(transport, hexExpected, "apdu response", {publicKey:65, publicKey_hex:130, returnCode:2, unexpected:9999});
noMoreAPDUs(transport);

await transport.close()
testEnd(scriptName);