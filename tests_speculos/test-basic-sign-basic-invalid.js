'use strict';

import * as common from './common.js';
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';

var scriptName = common.path.basename(fileURLToPath(import.meta.url));

common.testStart(scriptName);

const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(common.mockTransport);

//
//
//
console.log(common.humanTime() + " v".repeat(64) + " test: sign basic - invalid");

console.log(common.humanTime() + " // using FlowApp below with common.mockTransport() to grab apdu command without sending it");
const slot = 1;
const address = "e467b9dd11fa00df"
const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${scheme}'/0/0`;

common.curlScreenShot(scriptName); console.log(common.humanTime() + " // screen shot before sending first apdu command");

console.log(common.humanTime() + " -".repeat(64) + " await app.setSlot() // slot=" + slot  + " address=" + address + " path=" + path);
await app.setSlot(slot , address, path);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "331200001d01e467b9dd11fa00df2c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Set Account 1");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Account e467..");
common.curlScreenShot(scriptName); common.curlButton('right', "; navigate the address / path; Path 44'/..");
common.curlScreenShot(scriptName); common.curlButton('both', "; confirm; Approve");
common.curlScreenShot(scriptName); console.log(common.humanTime() + " // back to main screen");
var hexResponse = await common.curlApduResponseWait();
var hexExpected = "9000";
common.compare(hexResponse, hexExpected, "apdu response", {returnCode:4, unexpected:9999});

console.log(common.humanTime() + " -".repeat(64) + " await app.getAddressAndPubKey() // slot=" + slot);
await app.getAddressAndPubKey(slot);
var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "330100000101";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexExpected);
var hexResponse = await common.curlApduResponseWait();
var hexEXpected = "e467b9dd11fa00df04d7482bbaff7827035d5b238df318b10604673dc613808723efbd23fbc4b9fad34a415828d924ec7b83ac0eddf22ef115b7c203ee39fb080572d7e51775ee54be9000";
common.compare(hexResponse, hexEXpected, "apdu response", {address:8, publicKey:65, returnCode:4, unexpected:9999});

let invalidMessage = Buffer.from(
	"1234567890",
	"hex",
);
invalidMessage += "1";

console.log(common.humanTime() + " -".repeat(64) + " await app.sign() // path=" + path + " invalidMessage=..");
await app.sign(path, invalidMessage);

var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "33020000142c0000801b020080010200800000000000000000";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexEXpected = "9000";
common.compare(hexResponse, hexEXpected, "apdu response", {signatureCompact:29, returnCode:4, unexpected:9999});

var hexOutgoing = common.hexApduCommandViaMockTransportArray.shift();
var hexExpected = "330202000812345678efbfbd31";
common.compare(hexOutgoing, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, payload:9999});
common.asyncCurlApduSend(hexOutgoing);
var hexResponse = await common.curlApduResponseWait();
var hexEXpected = "7061727365725f726c705f6572726f725f696e76616c69645f6b696e646984";
common.compare(hexResponse, hexEXpected, "apdu response", {signatureCompact:29, returnCode:4, unexpected:9999});

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
