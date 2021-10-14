'use strict';

import { spawn, spawnSync } from 'child_process';

import * as path from 'path';

var test_speculos_api_port = process.env.TEST_SPECULOS_API_PORT ? process.env.TEST_SPECULOS_API_PORT : 5000;

console.log(); // blank line to visually separate the tests in the log
console.log(humanTime() + " // included: common.js; TEST_SPECULOS_API_PORT=" + test_speculos_api_port);

// see https://stackoverflow.com/questions/9763441/milliseconds-to-time-in-javascript
function msToTime(s) {
	// Pad to 2 or 3 digits, default is 2
	function pad(n, z) {
		z = z || 2;
		return ('00' + n).slice(-z);
	}

	var ms = s % 1000;
	s = (s - ms) / 1000;
	var secs = s % 60;
	s = (s - secs) / 60;
	var mins = s % 60;
	var hrs = (((s - mins) / 60) + 20) % 24;

	return pad(hrs) + ':' + pad(mins) + ':' + pad(secs) + '.' + pad(ms, 3);
}

function msToMatchSpeculosContainer() {
	return (4 * 3600 * 1000);
}

function humanTime() {
	return msToTime(Date.now() + msToMatchSpeculosContainer());
}

function syncBackTicks(command) {
	if (process.env.TEST_DEBUG >= 1) {
		console.log(humanTime() + " syncBackTicks() // command: " + command);
	}
	const curl_bash = spawnSync( 'bash', [ '-c', command ] );
	var output = curl_bash.stdout.toString().trim();
	if (process.env.TEST_DEBUG >= 1) {
		if (output != '') {
			console.log(output);
		}
	}
	return output;
}

function asyncBackTicks(command) {
	if (process.env.TEST_DEBUG >= 1) {
		console.log(humanTime() + " asyncBackTicks() // command: " + command);
	}
	const spawnObject = spawn( 'bash', [ '-c', command ] );
	return spawnObject;
}

function testStart(scriptName) { // e.g. test-basic-slot-status-set.js
	console.log(humanTime() + " testStart() // " + scriptName + "; re-run with TEST_PNG_RE_GEN_FOR=" + scriptName + " to regenerate PNGs, TEST_IGNORE_SHA256_SUMS=1 to ignore all PNGs, TEST_DEBUG=1 for extra debug output");
	syncBackTicks('rm ' + scriptName + '.*.png.new.png');
	if (process.env.TEST_PNG_RE_GEN_FOR && (scriptName.substring(0, process.env.TEST_PNG_RE_GEN_FOR.length) == process.env.TEST_PNG_RE_GEN_FOR)) {
		console.log(humanTime() + " curlScreenShot() // TEST_PNG_RE_GEN_FOR detected; deleting PNGs for this test");
		syncBackTicks('rm ' + scriptName + '.*.png');
	}
}

function testEnd(scriptName) { // e.g. test-basic-slot-status-set.js
	console.log(humanTime() + " testEnd()   // " + scriptName + "; <-- and passed if you read this!");
}

var curl_apdu_response_data = "";
var curl_apdu_response_exit = "";

function asyncCurlApduSend(apduCommand) {
	console.log(humanTime() + " asyncCurlApduSend() // " + apduCommand);
	var curl_apdu_object = asyncBackTicks('curl --silent --show-error --data \'{"data":"' + apduCommand + '"}\' http://127.0.0.1:' + test_speculos_api_port + '/apdu 2>&1');
	curl_apdu_object.stdout.on('data', function (data) { curl_apdu_response_data = data.toString().trim(); console.log(humanTime() + " asyncCurlApduSend() // async data "           + curl_apdu_response_data); });
	curl_apdu_object.on       ('exit', function (code) { curl_apdu_response_exit = code.toString().trim(); console.log(humanTime() + " asyncCurlApduSend() // async exit with code " + curl_apdu_response_exit); });
}

async function curlApduResponseWait() {
	var loops = 0;
	while (curl_apdu_response_exit === "") {
		await sleep(50, "waiting for async apdu response");
		loops += 1;
		if (loops >= 20) {
			console.log(humanTime() + " curlApduResponseWait() // ERRROR: looped too many times waiting for apdu response");
			throw new Error();
		}
	}
	var response_json = curl_apdu_response_data;
	console.log(humanTime() + " curlApduResponseWait() // response_json:" + response_json);
	var response = JSON.parse(response_json);
	curl_apdu_response_data = "";
	curl_apdu_response_exit = "";
	return response.data;
}

function curlButton(which, hint) { // e.g. which: 'left', 'right', or 'both'
	console.log(humanTime() + " curlButton() // " + which + hint);
	var output = syncBackTicks('curl --silent --show-error --data \'{"action":"press-and-release"}\' http://127.0.0.1:' + test_speculos_api_port + '/button/' + which + ' 2>&1');
	if (output != '{}') {
		console.log(humanTime() + " ERROR: unexpected curl stdout: " + output);
		throw new Error();
	}
}

var pngNum = 1;
var pngSha256Previous = "";

async function curlScreenShot(scriptName) {
	var png = scriptName + "." + pngNum.toString(10).padStart(2, '0') + ".png"
	console.log(humanTime() + " curlScreenShot() // " + png + ".new.png");
	var sleep_command = '';
	var cp_command = '';
	if (process.env.TEST_PNG_RE_GEN_FOR && (scriptName.substring(0, process.env.TEST_PNG_RE_GEN_FOR.length) == process.env.TEST_PNG_RE_GEN_FOR)) {
		console.log(humanTime() + " curlScreenShot() // TEST_PNG_RE_GEN_FOR detected; waiting to avoid partial screen capture, and re-generating this PNG for this test");
		sleep_command = 'sleep 1 ; ';
		cp_command = ' ; cp $PNG.new.png $PNG';
	}
	var loop;
	var loops = 0;
	do {
		loop = 0;
		var output = syncBackTicks(sleep_command + 'export PNG=' + png + ' ; curl --silent --show-error --output $PNG.new.png http://127.0.0.1:' + test_speculos_api_port + '/screenshot 2>&1' + cp_command + ' ; echo sha256:`sha256sum $PNG` ; echo sha256:`sha256sum $PNG.new.png`');
		const errorArray = output.match(/Empty reply from server/gi);
		if (null != errorArray) {
			console.log(humanTime() + " curl: screen shot: warning: curl failed to grab screen shot");
			throw new Error();
		}
		const regex = /sha256:[^\s]*/gm;
		const sha256Array = output.match(regex); // e.g. ['sha256:f3916e7cbbf8502b3eedbdf40cc6d6063b90f0e4a4814e34f2e7029bdaa4eaac','sha256:f3916e7cbbf8502b3eedbdf40cc6d6063b90f0e4a4814e34f2e7029bdaa4eaac']
		if (sha256Array[1] /* newly generated PNG */ == pngSha256Previous) {
			loops += 1;
			if (loops < 20) {
				console.log(humanTime() + " curlScreenShot() // matches previous screen shot SHA256 (" + pngSha256Previous + "); so requesting another screen shot");
				loop = 1;
			} else {
				console.log(humanTime() + " curlScreenShot() // matches previous screen shot SHA256 (" + pngSha256Previous + "); ERROR: giving up because too many tries");
				throw new Error();
			}
			continue;
		}
		if (sha256Array[0] != sha256Array[1]) {
			if (process.env.TEST_IGNORE_SHA256_SUMS >= 1) {
				console.log(humanTime() + " curlScreenShot() // running tests with TEST_IGNORE_SHA256_SUMS=1 to ignore all PNG differences");
				// sha256 sums are different but ignore that fact!
			} else {
				loops += 1;
				if (loops < 20) {
					// note: this deals with the case where the screen shot is of a partial screen rendering, e.g. "               >" instead of "Set Account 10 >".
					console.log(humanTime() + " curlScreenShot() // screen shot: warning: sha256 sums are different; could be partially rendered screen, so re-requesting screen shot // re-run with TEST_IGNORE_SHA256_SUMS=1 to ignore all PNGs");
					pngSha256Previous = sha256Array[1];
					loop = 1;
					continue;
				} else {
					console.log(humanTime() + " curlScreenShot() // screen shot: warning: sha256 sums are different; ERROR: re-requested screen shot too many times // re-run with TEST_IGNORE_SHA256_SUMS=1 to ignore all PNGs");
					throw new Error();
				}
			}
		}
		pngSha256Previous = sha256Array[0];
		pngNum ++;
	} while (loop);
}

function hex2ascii(hex) {
	var str = '';
	for (var i = 0; i < hex.length; i += 2)
		str += String.fromCharCode(parseInt(hex.substr(i, 2), 16));
	return str;
}

function compare(givenHex, expected, whatGiven, parts) {
	//console.log(humanTime() + " compare() // givenHex:" + givenHex + " expected:" + expected);
	var givenHexExploded = "";
	var expectedExploded = "";
	var signatureCompact = "";
	var p = 0;
	for (let [key, value] of Object.entries(parts)) {
		givenHexExploded = givenHexExploded + key + ':' + givenHex.substring(p, p + (value * 2)) + ' ';
		expectedExploded = expectedExploded + key + ':' + expected.substring(p, p + (value * 2)) + ' ';
		if (key === 'signatureCompact') {
			var hex = givenHex.substring(p, p + (value * 2));
			signatureCompact = "; signatureCompact.ascii:" + hex2ascii(hex);
		}
		p += value * 2;
	}
	console.log(humanTime() + " compare() // givenHexExploded:" + givenHexExploded + " <- " + whatGiven + signatureCompact);
	console.log(humanTime() + " compare() // expectedExploded:" + expectedExploded);
	if (givenHex != expected) {
		console.log(humanTime() + " compare() // givenHex NOT expected");
		throw new Error();
	}
}

function timeout(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

async function sleep(ms, what) {
	console.log(humanTime() + " sleep() // " + ms + "ms <- " + what);
	await timeout(ms);
}

var hexApduCommandViaMockTransportArray = [];
var fakeApduResponseViaMockTransport = 0;

var mockTransport = {
	//this.transport.send(_common.CLA, _common.INS.SET_SLOT, 0, 0, payload).then(...)
	send: async function(cla, ins, p1, p2, pay) {
		var len = (typeof pay === 'undefined') ? 0 : pay.length;
		var hex_cla = cla.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() cla: 0x" + hex_cla); } // e.g. 33
		var hex_ins = ins.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() ins: 0x" + hex_ins); } // e.g. 12
		var hex_p1  =  p1.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() p1 : 0x" + hex_p1 ); } // e.g. 00
		var hex_p2  =  p2.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() p2 : 0x" + hex_p2 ); } // e.g. 00
		var hex_len = len.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() len: 0x" + hex_len); } // e.g. 1d
		var hex_pay = len ? pay.toString('hex') : ""   ; if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() pay: 0x" + hex_pay + " <- " + len + " bytes"); } // e.g. 0a00010203040506072c0000801b020080010200800000000000000000
		var hexApduCommandViaMockTransport = ''.concat(hex_cla, hex_ins, hex_p1, hex_p2, hex_len, hex_pay); // e.g. 331200001d0a00010203040506072c0000801b020080010200800000000000000000
		hexApduCommandViaMockTransportArray.push(hexApduCommandViaMockTransport);
		console.log(humanTime() + " .send() // " + hexApduCommandViaMockTransport + " <-- this is the mockTransport.send() (read: fake send) function");
		return {
			then: function(functionHandleResponseAsSuccess, functionHandleRepsonseAsError) {
				if (0 == fakeApduResponseViaMockTransport) {
					console.log(humanTime() + " .send().then() // dishing up fake apdu response; success");
					functionHandleResponseAsSuccess(Buffer.from([0x90, 0x00]));
				} else {
					fakeApduResponseViaMockTransport = 0; // auto set next fake response to success
					console.log(humanTime() + " .send().then() // dishing up fake apdu response; error");
					functionHandleResponseAsError(Buffer.from([0x69, 0x82]));
				}
				return;
			}
		};
	},
};

function mockTransportSetNextFakeResponseToError() {
	fakeApduResponseViaMockTransport = 1
}

export {testStart, testEnd, compare, asyncCurlApduSend, curlApduResponseWait, curlButton, curlScreenShot, humanTime, sleep, mockTransport, mockTransportSetNextFakeResponseToError, hexApduCommandViaMockTransportArray, path};
