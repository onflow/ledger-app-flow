import { expect } from "jest";
import Zemu from "@zondax/zemu";
import jsSHA from "jssha";
import {ec as EC} from "elliptic";

import FlowApp from "../app";
import { APP_PATH, simOptions } from "./setup";

describe('Basic checks', function () {

  it('app version', async function () {
    const sim = new Zemu(APP_PATH);

    try {
      await sim.start(simOptions);
      const app = new FlowApp(sim.getTransport());
      const resp = await app.getVersion();

      console.log(resp);

      expect(resp.returnCode).toEqual(0x9000);
      expect(resp.errorMessage).toEqual("No errors");
      expect(resp).toHaveProperty("testMode");
      expect(resp).toHaveProperty("major");
      expect(resp).toHaveProperty("minor");
      expect(resp).toHaveProperty("patch");
    } finally {
      await sim.close();
    }
  });

  it('sign secp256k1 basic & verify SHA2-256 - create', async function () {

    const sim = new Zemu(APP_PATH);

    try {
      await sim.start(simOptions);
      const app = new FlowApp(sim.getTransport());

      const scheme = FlowApp.Signature.SECP256K1 | FlowApp.Hash.SHA2_256;
      const path = `m/44'/539'/${scheme}'/0/0`;

      const randomData = "00000000111111FFFFFFFFFFFFFFFFFFFF"
      const txBlob = Buffer.from(
        randomData,
        "hex",
      );

      const pkResponse = await app.getAddressAndPubKey(path);
      console.log(pkResponse);
      expect(pkResponse.returnCode).toEqual(0x9000);
      expect(pkResponse.errorMessage).toEqual("No errors");

      // sign request 
      const signatureRequest = app.sign(path, txBlob);

      const resp = await signatureRequest;

      expect(resp.returnCode).toEqual(0x9000);
      expect(resp.errorMessage).toEqual("No errors");

      // Prepare digest
      const hasher = new jsSHA("SHA-256", "UINT8ARRAY");
      hasher.update(txBlob)
      const digest = hasher.getHash("HEX");

      // Verify signature
      const ec = new EC("secp256k1");
      const sig = resp.signatureDER.toString("hex");
      const pk = pkResponse.publicKey.toString("hex");

      const signatureOk = ec.verify(digest, sig, pk, 'hex');
      expect(signatureOk).toEqual(true);
    } finally {
      await sim.close();
    }
  });
});
