'use strict';

import { testStart, testStep, testEnd, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs ,getScriptName, getSpeculosDefaultConf } from "./speculos-common.js";
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
let hexExpected = "";

await device.makeStartingScreenshot();

//send invalid message
const options = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
const path = `m/44'/539'/${0x201}'/0/0`;

{
	const invalidMessage = Buffer.from(
		"1234567890",
		"hex",
	);
	const randomCorrectScriptHash = "ca80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2"
		testStep(" - - -", "await app.sign() // path=" + path + " invalidMessage=..");
	const signResponse = await app.sign(path, invalidMessage, options, randomCorrectScriptHash);
	assert.equal(signResponse.returnCode, 0x6984);
	assert.equal(signResponse.errorMessage, "Data is invalid");

	compareGetVersionAPDUs(transport);
	hexExpected = "33020000162c0000801b0200800102008000000000000000000103";
	compareOutAPDU(transport, hexExpected, "apdu command", {cla:1, ins:1, p1:1, p2:1, len:1, path:20, options:2, unexpected:9999});
	hexExpected = "9000";
	compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
	hexExpected = "33020100051234567890";
	compareOutAPDU(transport, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, message:5, unexpected:9999});
	hexExpected = "9000";
	compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
	hexExpected = "330203009903ca80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2d56f4e1d2355cdcfacfd01e471459c6ef168bfdf84371a685ccf31cf3cdedc2d47851586d962335e3f7d9e5d11a4c527ee4b5fd1c3895e3ce1b9c2821f60b166546f6b656e205472616e73666572000201416d6f756e74000055466978363400030144657374696e6174696f6e0001416464726573730003";
	compareOutAPDU(transport, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, metadata:0x99, unexpected:9999});
	hexExpected = "9000";
	compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
	hexExpected = "33020400e027b42f7104fa842ba664b1c199325f8339dd65db575938cef8498d082727c3efcaf67b7d0995d18f289676386108de6b4c134059b000a6f1ec0c80b857b0ec380c538359a1be1553e729aad363de4eec2dcdcc666320eb85899c2f978c5fbf94033592e53bc3280098d0043e4770cfaf7675a56b9c56d836a5e2252de61dc3a85f7f784ed0a40905ed96c8b96588a84b025eb04d593a7662bb66b9eb3a7e962a12c65f0494d2165cbeaa141d705647d8071b528999782dd54a84a44ad2cf3d7b6730cd08974b8ccd1102dd2f5159a8181cc1ede512f6d537d8eea46556e16944";
	compareOutAPDU(transport, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_hashes:0xe0, unexpected:9999});
	hexExpected = "9000";
	compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
	hexExpected = "33020400e0aa44049d7eb26d667c3714b8fcf78380878e658e22268510ff9f72e69b29decd969676717f03d5697f4d83eda34ad3ae02eb547ac971f0a0b4e118d32db6ef73a1b4c2165e1fceba7324712485844e04e5e955706ae328f6cfd64c49686a810417009ee11f842f7783df52789f4ecaf8d689eb878a053167b43302678f4cd224c9967fd752058341822bc98880abb34e04d3afef58de0e256f2264991e54812353b33a3b57dd772b1dff0636374eb01bc7839efd7677d32399380c37f9a2d03604b126988dc8f00c41d922c13e0ae5a4e7d83f18b44935103cdbbd7c2229d53f";
	compareOutAPDU(transport, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_hashes:0xe0, unexpected:9999});
	hexExpected = "9000";
	compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
	hexExpected = "33020400e0d6f168b2ac8da11c1a93cc44fd3126eeb7fbd7046a11cd664fd4eafb51502e7788bd487007bf1a5be47cea944d797895181258aba33c77e8c75fe7e38ad9192988bd487007bf1a5be47cea944d797895181258aba33c77e8c75fe7e38ad9192988bd487007bf1a5be47cea944d797895181258aba33c77e8c75fe7e38ad9192988bd487007bf1a5be47cea944d797895181258aba33c77e8c75fe7e38ad9192988bd487007bf1a5be47cea944d797895181258aba33c77e8c75fe7e38ad9192988bd487007bf1a5be47cea944d797895181258aba33c77e8c75fe7e38ad91929";
	compareOutAPDU(transport, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_hashes:0xe0, unexpected:9999});
	hexExpected = "9000";
	compareInAPDU(transport, hexExpected, "apdu response", {returnCode:2, unexpected:9999});
	hexExpected = "33020500e06edc3a498a77648f820dc3b2d563f74314480e8569c4efbcd69c600f59cec2d094a4bf5f458f2def50f807bf419501bfd5e77a084c30592aa3803a522a3c272e94a4bf5f458f2def50f807bf419501bfd5e77a084c30592aa3803a522a3c272e94a4bf5f458f2def50f807bf419501bfd5e77a084c30592aa3803a522a3c272e94a4bf5f458f2def50f807bf419501bfd5e77a084c30592aa3803a522a3c272e94a4bf5f458f2def50f807bf419501bfd5e77a084c30592aa3803a522a3c272e94a4bf5f458f2def50f807bf419501bfd5e77a084c30592aa3803a522a3c272e";
	compareOutAPDU(transport, hexExpected, "apdu response", {cla:1, ins:1, p1:1, p2:1, len:1, do_not_compare_hashes:0xe0, unexpected:9999});
	//Second incoming APDU not cached by SpyTransport as SpeculosTransport throws an exception.
	noMoreAPDUs(transport);
}

{
	//Provided script hash not known - merkle index only knows first bytes of the hash so we have to make error at the beginning
	const correctMessage = "f9023ff9023bb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df861b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
	const incorrectScriptHash = "da80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2" //First element changed
	testStep(" - - -", "await app.sign() // hash not in merkle tree");
	const signResponse = await app.sign(path, correctMessage, options, incorrectScriptHash);	
	assert.equal(signResponse.returnCode, 0xffff);
	assert.equal(signResponse.errorMessage, "Script hash not in the list of known scripts.");
}

//Argument missing
{
	//This message has second argument removed
	const incorrectMessage = "f9020ef9020ab90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df1b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
	const correctScriptHash = "ca80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2"
	testStep(" - - -", "await app.sign() // argument missing");
	const signResponse = await app.sign(path, incorrectMessage, options, correctScriptHash);	
	assert.equal(signResponse.returnCode, 0x6984);
	assert.equal(signResponse.errorMessage, "Data is invalid");
}

//Argument extra
{
	//This message has a third argument
	const incorrectMessage = "f90240f9023cb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df862b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241646472657373222c2276616c7565223a22307866386436653035383662306132306337227d00a0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
	const correctScriptHash = "ca80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2"
	testStep(" - - -", "await app.sign() // argument extra");
	const signResponse = await app.sign(path, incorrectMessage, options, correctScriptHash);	
	assert.equal(signResponse.returnCode, 0x6984);
	assert.equal(signResponse.errorMessage, "Data is invalid");
}

//Wrong argument type
{
	//In second argument Address is misspelled as Aedress
	const incorrectMessage = "f9023ff9023bb90195696d706f72742046756e6769626c65546f6b656e2066726f6d203078656538323835366266323065326161360a7472616e73616374696f6e28616d6f756e743a205546697836342c20746f3a204164647265737329207b0a6c6574207661756c743a204046756e6769626c65546f6b656e2e5661756c740a70726570617265287369676e65723a20417574684163636f756e7429207b0a73656c662e7661756c74203c2d207369676e65720a2e626f72726f773c267b46756e6769626c65546f6b656e2e50726f76696465727d3e2866726f6d3a202f73746f726167652f666c6f77546f6b656e5661756c7429210a2e776974686472617728616d6f756e743a20616d6f756e74290a7d0a65786563757465207b0a6765744163636f756e7428746f290a2e6765744361706162696c697479282f7075626c69632f666c6f77546f6b656e526563656976657229210a2e626f72726f773c267b46756e6769626c65546f6b656e2e52656365697665727d3e2829210a2e6465706f7369742866726f6d3a203c2d73656c662e7661756c74290a7d0a7df861b07b2274797065223a22554669783634222c2276616c7565223a223138343436373434303733372e39353531363135227daf7b2274797065223a2241656472657373222c2276616c7565223a22307866386436653035383662306132306337227da0f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b2a88f8d6e0586b0a20c7040a88f8d6e0586b0a20c7c988f8d6e0586b0a20c7c0"
	const correctScriptHash = "ca80b628d985b358ae1cb136bcd976997c942fa10dbabfeafb4e20fa66a5a5e2"
	testStep(" - - -", "await app.sign() // argument extra");
	const signResponse = await app.sign(path, incorrectMessage, options, correctScriptHash);	
	assert.equal(signResponse.returnCode, 0x6984);
	assert.equal(signResponse.errorMessage, "Data is invalid");
}



await transport.close()
testEnd(scriptName);
process.stdin.pause()
