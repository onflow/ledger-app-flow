'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import assert from 'assert/strict';

await common.sleep(3000)

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
const slot = 1;
const address = "e467b9dd11fa00df"
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

//setslot
common.testStep(" - - -", "app.setSlot() // slot=" + slot  + " address=" + address + " path=" + path);
const setSlotPromise = app.setSlot(slot , address, path);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Set Account 1");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
common.testStep(" - - -", "await setSlotPromise // slot=" + slot  + " address=" + address + " path=" + path);
const setSlotResponse = await setSlotPromise
assert.equal(setSlotResponse.returnCode, 0x9000);
assert.equal(setSlotResponse.errorMessage, "No errors");

assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "331200001d01e467b9dd11fa00df2c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, slotBytes:28, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

//getpubkey
common.testStep(" - - -", "await app.getAddressAndPubKey() // slot=" + slot);
const getPubkeyResponse = await app.getAddressAndPubKey(slot);
assert.equal(getPubkeyResponse.returnCode, 0x9000);
assert.equal(getPubkeyResponse.errorMessage, "No errors");
const pubkeyHex = getPubkeyResponse.publicKey.toString("hex")
console.log(common.humanTime() + " publicKeyHex=" + pubkeyHex);

assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "330100000101";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "e467b9dd11fa00df04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be9000";
common.compare(hexIncomming, hexExpected, "apdu response", {address:8, publicKey:65, returnCode:2, unexpected:9999});

//send invalid message
let invalidMessage = Buffer.from(
	"1234567890",
	"hex",
);
invalidMessage += "1";

common.testStep(" - - -", "await app.sign() // path=" + path + " invalidMessage=..");
const signResponse = await app.sign(path, invalidMessage);
assert.equal(signResponse.returnCode, 0x6984);
assert.equal(signResponse.errorMessage, "Data is invalid : parser_rlp_error_invalid_kind");

assert.equal(common.mockTransport.hexApduCommandOut.length, 2)
assert.equal(common.mockTransport.hexApduCommandIn.length, 2)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "33020000142c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, path:20, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});

var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "330202000812345678efbfbd31";
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "7061727365725f726c705f6572726f725f696e76616c69645f6b696e646984";
common.compare(hexIncomming, hexExpected, "apdu response", {signatureCompact:29, returnCode:2, unexpected:9999});


//delete the slot so that next test start with clean state
const expectedAccountDelete = "0000000000000000";
const expectedPathDelete = `m/0/0/0/0/0`;
common.testStep(" - - -", "app.setSlot() // slot=" + slot + " expectedAccountDelete=" + expectedAccountDelete + " expectedPathDelete=" + expectedPathDelete);
const setSlot2Promise = app.setSlot(slot, expectedAccountDelete, expectedPathDelete);

common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Delete Account 1");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Old Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");

common.testStep(" - - -", "await setSlot2Promise");
const setSlot2Response = await setSlot2Promise;
assert.equal(setSlot2Response.returnCode, 0x9000);
assert.equal(setSlot2Response.errorMessage, "No errors");

//I really want to remove this check as this is not SUT
assert.equal(common.mockTransport.hexApduCommandOut.length, 1)
assert.equal(common.mockTransport.hexApduCommandIn.length, 1)
var hexOutgoing = common.mockTransport.hexApduCommandOut.shift();
var hexExpected = "331200001d0100000000000000000000000000000000000000000000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, slot:1, do_not_compare_slotBytes:28, unexpected:9999});
var hexIncomming = common.mockTransport.hexApduCommandIn.shift();
var hexExpected = "9000";
common.compare(hexIncomming, hexExpected, "apdu response", {returnCode:2, unexpected:9999});



common.testEnd(scriptName);

// Above is the speculos-only / zemu-free test.
// Below is the original zemu test for comparison:
/*
    test("sign basic - invalid", async function () {
        const sim = new Zemu(APP_PATH);
        try {
            await sim.start(simOptions);
            const app = new FlowApp(sim.getTransport());

            const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
            const path = `m/44'/539'/${scheme}'/0/0`;
            const address = "e467b9dd11fa00df"

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
*/
