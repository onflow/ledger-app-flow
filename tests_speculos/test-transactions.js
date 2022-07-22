'use strict';

import { testStart, testEnd, testCombo, compareInAPDU, compareOutAPDU, noMoreAPDUs, getScriptName, getSpeculosDefaultConf, humanTime } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { getButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { transactionTest } from "./speculos-transaction.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath } from 'url';
import fs from "fs";
import jsSHA from "jssha";

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);

//Now we test the transactions
const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1, pathCode: 0x200 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256, pathCode: 0x300};

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256, pathCode: 0x01};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256, pathCode: 0x03};

const sigAlgos = [ECDSA_SECP256K1, ECDSA_P256];
const hashAlgos = [SHA2_256, SHA3_256];

const exampleTransferBlob        = "f9023ff9023bb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df861b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
const exampleTransferScHash      = "ca80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2"
const exampleCreateAccountBlob   = "f90289f90261b8a97472616e73616374696f6e287075626c69634b6579733a205b537472696e675d29207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a6c65742061636374203d20417574684163636f756e742870617965723a207369676e6572290a666f72206b657920696e207075626c69634b657973207b0a616363742e6164645075626c69634b6579286b65792e6465636f64654865782829290a7d0a7d0a7df90173b901707b2274797065223a224172726179222c2276616c7565223a5b7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227d2c7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303331227d5d7da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
const exampleCreateAccountScHash = "eef2d0494448554177612e63026256258339230cbc6931ded78d6149443c6173"
const exampleAddKeyBlob          = "f90186f9015eb86e7472616e73616374696f6e287075626c69634b65793a20537472696e6729207b0a70726570617265287369676e65723a20417574684163636f756e7429207b0a7369676e65722e6164645075626c69634b6579287075626c69634b65792e6465636f64654865782829290a7d0a7df8acb8aa7b2274797065223a22537472696e67222c2276616c7565223a2266383435623834303934343838613739356130373730306336666238336530363663663537646664383766393263653730636263383163623362643366656132646637623637303733623730653336623434663335373862343364363464336661613265386534313565663663326235666534333930643561373865323338353831633665346263333033303330227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7e4e38004a0f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162"
const exampleAddKeyScHash        = "595c86561441b32b2b91ee03f9e10ca6efa7b41bcc994f51317ec0aa9d8f8a42"

{
	const transactions = [
		{
			title: "Transfer FLOW",
			blob: exampleTransferBlob,
			hash: exampleTransferScHash
		},
		{
			title: "Create Account",
			blob: exampleCreateAccountBlob,
			hash: exampleCreateAccountScHash
		},
		{
			title: "Add Key",
			blob: exampleAddKeyBlob,
			hash: exampleAddKeyScHash
		},
	];

	for (let i=0; i < transactions.length; ++i ) {
		for (let j=0; j < sigAlgos.length; ++j ) {
			for (let k=0; k < hashAlgos.length; ++k ) {
				const testTitle = `basic sign: ${transactions[i].title} - ${sigAlgos[j].name} / ${hashAlgos[k].name}`; // e.g. basic sign: Transfer FLOW - secp256k1 / SHA-256
				const scriptNameCombo = (scriptName + "." + testTitle).replace(new RegExp("([:/ \-]+)","gm"),"-").toLowerCase(); 
				testCombo(scriptNameCombo);
				const device = getButtonsAndSnapshots(scriptNameCombo, speculosConf);
				await device.makeStartingScreenshot();

				await transactionTest(
					app,
					transport,
					device,
					transactions[i].blob, 
					transactions[i].hash, 
					sigAlgos[j],
					hashAlgos[k],
					13
				);
			}
		}
	}
}

// todo: consider adding mechanism to run subset of tests so that tests can be run in parallel on different speculos instances
{
	const transactions = JSON.parse(fs.readFileSync("../tests/testvectors/manifestEnvelopeCases.json"));

	for (var i=0; i < transactions.length; ++i ) {
		if (transactions[i].chainID == "Mainnet") {
			var testTitle = `staking sign: ${transactions[i].title} - ${ECDSA_P256.name} / ${SHA3_256.name}`; // e.g. staking sign: TH.01 - Withdraw Unlocked FLOW - p256 / SHA3-256
			const scriptNameCombo = (scriptName + "." + testTitle).replace(new RegExp("([:/ \-]+)","gm"),"-").toLowerCase(); 
			testCombo(scriptNameCombo);
			const device = getButtonsAndSnapshots(scriptNameCombo, speculosConf);
			await device.makeStartingScreenshot();

			const hasher = new jsSHA("SHA-256", "TEXT")
			hasher.update(transactions[i].payloadMessage.script)
			const scriptHash = hasher.getHash("HEX")

			await transactionTest(
				app,
				transport,
				device,
				transactions[i].encodedTransactionEnvelopeHex,
				scriptHash,
				ECDSA_P256,
				SHA3_256,
				13
			);
		}
	}
}

await transport.close()
testEnd(scriptName);
process.stdin.pause()
