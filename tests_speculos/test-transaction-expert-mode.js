'use strict';

import { testStart, testEnd, testStep, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs, getScriptName, getSpeculosDefaultConf, humanTime } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { ButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { transactionTest } from "./speculos-transaction.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath, pathToFileURL } from 'url';
import assert from 'assert/strict';

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = new ButtonsAndSnapshots(scriptName, speculosConf);

const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1, pathCode: 0x200 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256, pathCode: 0x300};

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256, pathCode: 0x01};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256, pathCode: 0x03};

const path = `m/44'/539'/0'/0/0`;

await device.makeStartingScreenshot();

device.toggleExpertMode("ON");

const exampleTxBlob = "f9023ff9023bb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df861b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0";

testStep(" - - -", "Testing tx with empty slot");
await transactionTest(
	app,
	transport,
	device,
	exampleTxBlob, 
	ECDSA_P256,
	SHA3_256,
	13,
	path,
);

testStep(" - - -", "Testing tx with empty slot");
await transactionTest(
	app,
	transport,
	device,
	exampleTxBlob, 
	ECDSA_SECP256K1,
	SHA2_256,
	13,
	path,
);


device.toggleExpertMode("OFF");

await transport.close()
testEnd(scriptName);
