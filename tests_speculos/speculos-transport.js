'use strict';
import SpeculosTransport from "@ledgerhq/hw-transport-node-speculos";
import { testStep, humanTime } from "./speculos-common.js"
import TransportNodeHid from "@ledgerhq/hw-transport-node-hid"

async function getSpyTransport(conf) {
	return await (new SpyTransport()).initialize(conf);
}

class SpyTransport {
	transport

	//saves the communication
	hexApduCommandOut = []
	hexApduCommandIn = []

	//synchronization
	APDUToWait = null
	APDUToWaitPromise = null
	resolveAPDUToWaitPromise = function() {}

	async initialize(conf) {
		if (conf.testOn === "ledger") {
			this.transport = await TransportNodeHid.default.create(1000);
		}
		else {
			this.transport = await SpeculosTransport.default.open({ apduPort: conf.speculosApduPort });
		}
		return this;
	}

	//this.transport.send(_common.CLA, _common.INS.SET_SLOT, 0, 0, payload).then(...)
	async send(cla, ins, p1, p2, pay, noThrowErrorCodes) {
		const len = (typeof pay === 'undefined') ? 0 : pay.length;
		const hex_cla = cla.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() cla: 0x" + hex_cla); } // e.g. 33
		const hex_ins = ins.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() ins: 0x" + hex_ins); } // e.g. 12
		const hex_p1  =  p1.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() p1 : 0x" + hex_p1 ); } // e.g. 00
		const hex_p2  =  p2.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() p2 : 0x" + hex_p2 ); } // e.g. 00
		const hex_len = len.toString(16).padStart(2, '0'); if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() len: 0x" + hex_len); } // e.g. 1d
		const hex_pay = len ? pay.toString('hex') : ""   ; if (process.env.TEST_DEBUG >= 1) { console.log(humanTime() + " .send() pay: 0x" + hex_pay + " <- " + len + " bytes"); } // e.g. 0a00010203040506072c0000801b020080010200800000000000000000
		const hexApduCommandViaMockTransport = ''.concat(hex_cla, hex_ins, hex_p1, hex_p2, hex_len, hex_pay); // e.g. 331200001d0a00010203040506072c0000801b020080010200800000000000000000
		this.hexApduCommandOut.push(hexApduCommandViaMockTransport);		
		console.log(humanTime() + " .send() // " + hexApduCommandViaMockTransport + " <-- this is the mockTransport.send() (read: fake send) function");

		//Now we actually send
		testStep(" >    ", "APDU out");
		const sendPromise = this.transport.send(cla, ins, p1, p2, pay, noThrowErrorCodes)

		//If we are waiting for this APDU we resolve the promise
		if (!!this.APDUToWait && cla == this.APDUToWait.cla && ins == this.APDUToWait.ins && p1 == this.APDUToWait.p1) {
			this.resolveAPDUToWaitPromise()
		}
		testStep("     <", "APDU in");
		const res = await sendPromise;
		this.hexApduCommandIn.push(""+Buffer.from(res).toString("hex"));		

		return Buffer.from(res) 
	}

	async waitForAPDU(cla, ins, p1) {
		this.APDUToWait = {cla: cla, ins: ins, p1: p1};
		this.APDUToWaitPromise = new Promise(resolve => this.resolveAPDUToWaitPromise = resolve)
		await this.APDUToWaitPromise;
		this.APDUToWait = null;
		this.APDUToWaitPromise = null;
		this.resolveAPDUToWaitPromise = function() {};
	}

	async close() {
		await this.transport.close()
	}
};

export {getSpyTransport};
