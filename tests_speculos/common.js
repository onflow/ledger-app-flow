'use strict';

import { spawn, spawnSync } from 'child_process';

import * as path from 'path';

var test_speculos_api_port = process.env.TEST_SPECULOS_API_PORT ? process.env.TEST_SPECULOS_API_PORT : 5000;

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

var png_num = 1;

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

function test_start(scriptName) { // e.g. test-basic-slot-status-set.js
	console.log(humanTime() + " test_start() // " + scriptName + " // re-run with TEST_REGEN_PNGS=1 to regenerate PNGs, TEST_IGNORE_SHA256_SUMS=1 to ignore all PNGs, TEST_DEBUG=1 to debug");
	syncBackTicks('rm ' + scriptName + '.*.png.new.png');
	if (process.env.TEST_REGEN_PNGS >= 1) {
		console.log(humanTime() + " test start: deleting PNGs due to TEST_REGEN_PNGS");
		syncBackTicks('rm ' + scriptName + '.*.png');
	}
}

function test_end() {
	console.log(humanTime() + " test_end() // <-- and passed if you read this!");
}

function curl_apdu(payload) {
	//curl -d '{"data":"331200001d0a00010203040506072c0000801b020080010200800000000000000000"}' 
	console.log(humanTime() + " curl_apdu() // " + payload);
	return asyncBackTicks('curl --silent --show-error --data \'{"data":"' + payload + '"}\' http://127.0.0.1:' + test_speculos_api_port + '/apdu 2>&1');
}

async function curl_apdu_response(curl_apdu_object, expected_result) {
	console.log(humanTime() + " curl_apdu_response() // expected_result=" + expected_result);
	for await (const data of curl_apdu_object.stdout) {
		var result = data.toString().trim();
		if ((process.env.TEST_REGEN_PNGS >= 1) || (result != expected_result)) {
			console.log(result);
		}
		if (result != expected_result) {
			console.log(humanTime() + " curl_apdu_response(): warning: result NOT expected result: " + expected_result);
			throw new Error();
		}
	};
}

function curl_button(which) { // e.g. which: 'left', 'right', or 'both'
	console.log(humanTime() + " curl_button() // " + which);
	var output = syncBackTicks('curl --silent --show-error --data \'{"action":"press-and-release"}\' http://127.0.0.1:' + test_speculos_api_port + '/button/' + which + ' 2>&1');
	if (output != '{}') {
		console.log(humanTime() + " ERROR: unexpected curl stdout: " + output);
		//todo: assert!
	}
}

function curl_screen_shot(scriptName) {
	//curl -o screenshot.png http://127.0.0.1:5000/screenshot
	var png = scriptName + "." + png_num.toString(10).padStart(2, '0') + ".png"
	console.log(humanTime() + " curl_screen_shot() // " + png + ".new.png");
	var cp_command = '';
	if (process.env.TEST_REGEN_PNGS >= 1) {
		cp_command = ' ; cp $PNG.new.png $PNG';
	}
	var output = syncBackTicks('export PNG=' + png + ' ; curl --silent --show-error --output $PNG.new.png http://127.0.0.1:' + test_speculos_api_port + '/screenshot 2>&1' + cp_command + ' ; echo sha256:`sha256sum $PNG` ; echo sha256:`sha256sum $PNG.new.png`');
	const errorArray = output.match(/Empty reply from server/gi);
	if (null != errorArray) {
		console.log(humanTime() + " curl: screen shot: warning: curl failed to grab screen shot");
		throw new Error();
	}
	const regex = /sha256:[^\s]*/gm;
	const sha256array = output.match(regex); // e.g. ['sha256:f3916e7cbbf8502b3eedbdf40cc6d6063b90f0e4a4814e34f2e7029bdaa4eaac','sha256:f3916e7cbbf8502b3eedbdf40cc6d6063b90f0e4a4814e34f2e7029bdaa4eaac']
	if (sha256array[0] != sha256array[1]) {
		console.log(humanTime() + " curl: screen shot: warning: sha256 sums are different // re-run with TEST_IGNORE_SHA256_SUMS=1 to ignore all PNGs");
		if (process.env.TEST_IGNORE_SHA256_SUMS >= 1) {
			// sha256 sums are different but ignore that fact!
		} else {
			throw new Error();
		}
	}
	png_num ++;
}

function timeout(ms) {
	return new Promise(resolve => setTimeout(resolve, ms));
}

async function sleep(ms) {
	console.log(humanTime() + " sleep() // " + ms + "ms");
	await timeout(ms);
}

var hexPayloadViaMockTransport;

var mockTransport = {
	//this.transport.send(_common.CLA, _common.INS.SET_SLOT, 0, 0, payload).then(...)
	send: function(cla, ins, p1, p2, pay) {
		console.log(humanTime() + " .send() // <-- this is the mockTransport.send() function");
		var len = pay.length;
		var hex_cla = cla.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() cla: 0x" + hex_cla); } // e.g. 33
		var hex_ins = ins.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() ins: 0x" + hex_ins); } // e.g. 12
		var hex_p1  =  p1.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() p1 : 0x" + hex_p1 ); } // e.g. 00
		var hex_p2  =  p2.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() p2 : 0x" + hex_p2 ); } // e.g. 00
		var hex_len = len.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() len: 0x" + hex_len); } // e.g. 1d
		var hex_pay = pay.toString('hex');               if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() pay: 0x" + hex_pay); } // e.g. 0a00010203040506072c0000801b020080010200800000000000000000
		hexPayloadViaMockTransport = ''.concat(hex_cla, hex_ins, hex_p1, hex_p2, hex_len, hex_pay); // e.g. 331200001d0a00010203040506072c0000801b020080010200800000000000000000
		return {
			then: function() {
				console.log(humanTime() + " .send().then() // ignoring!");
			}
		};
	},
};

export {test_start, test_end, curl_apdu, curl_apdu_response, curl_button, curl_screen_shot, humanTime, sleep, mockTransport, hexPayloadViaMockTransport, path};
