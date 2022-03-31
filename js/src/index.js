/** ******************************************************************************
 *  (c) 2019-2020 Zondax GmbH
 *  (c) 2016-2017 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************* */

import {serializePathv1, printBIP44Path, signSendChunkv1} from "./helperV1";
import {
  CHUNK_SIZE,
  CLA,
  ERROR_CODE,
  errorCodeToString,
  getVersion,
  INS,
  P1_VALUES,
  PKLEN,
  processErrorResponse,
} from "./common";

function processGetAddrResponse(response) {
  let partialResponse = response;

  const errorCodeData = partialResponse.slice(-2);
  const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

  const publicKey = Buffer.from(partialResponse.slice(0, PKLEN));
  partialResponse = partialResponse.slice(PKLEN);
  const address = Buffer.from(partialResponse.slice(0, -2)).toString();

  return {
    publicKey,
    address,
    returnCode,
    errorMessage: errorCodeToString(returnCode),
  };
}

export default class FlowApp {
  static get ErrorCode() {
    return ERROR_CODE;
  }

  static get Hash() {
    return {
      SHA2_256: 0x01,
      SHA3_256: 0x03,
    }
  }

  static get Signature() {
    return {
      SECP256K1: 0x0200,
      P256: 0x0300,
    }
  }

  constructor(transport) {
    if (!transport) {
      throw new Error("Transport has not been defined");
    }
    this.transport = transport;
  }

  static prepareChunks(serializedPathBuffer, message, sigalgo, hashalgo) {
    const chunks = [];

    const pathAndCryptoOpts = Buffer.concat([ serializedPathBuffer, Buffer.from([hashalgo, sigalgo]) ])

    // First chunk (only path + crypto opts)
    chunks.push(pathAndCryptoOpts);

    const messageBuffer = Buffer.from(message)

    const buffer = Buffer.concat([messageBuffer])
    for (let i = 0; i < buffer.length; i += CHUNK_SIZE) {
      let end = i + CHUNK_SIZE;
      if (i > buffer.length) {
        end = buffer.length;
      }
      chunks.push(buffer.slice(i, end));
    }

    return chunks;
  }

  async signGetChunks(path, message, sigalgo, hashalgo) {
    return FlowApp.prepareChunks(serializePathv1(path), message, sigalgo, hashalgo);
  }

  async getVersion() {
    return getVersion(this.transport)
      .then((response) => {
        return response;
      })
      .catch((err) => processErrorResponse(err));
  }

  async appInfo() {
    return this.transport.send(0xb0, 0x01, 0, 0).then((response) => {
      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

      const result = {};

      let appName = "err";
      let appVersion = "err";
      let flagLen = 0;
      let flagsValue = 0;

      if (response[0] !== 1) {
        // Ledger responds with format ID 1. There is no spec for any format != 1
        result.errorMessage = "response format ID not recognized";
        result.returnCode = 0x9001;
      } else {
        const appNameLen = response[1];
        appName = response.slice(2, 2 + appNameLen).toString("ascii");
        let idx = 2 + appNameLen;
        const appVersionLen = response[idx];
        idx += 1;
        appVersion = response.slice(idx, idx + appVersionLen).toString("ascii");
        idx += appVersionLen;
        const appFlagsLen = response[idx];
        idx += 1;
        flagLen = appFlagsLen;
        flagsValue = response[idx];
      }

      return {
        returnCode,
        errorMessage: errorCodeToString(returnCode),
        // //
        appName,
        appVersion,
        flagLen,
        flagsValue,
        // eslint-disable-next-line no-bitwise
        flagRecovery: (flagsValue & 1) !== 0,
        // eslint-disable-next-line no-bitwise
        flagSignedMcuCode: (flagsValue & 2) !== 0,
        // eslint-disable-next-line no-bitwise
        flagOnboarded: (flagsValue & 4) !== 0,
        // eslint-disable-next-line no-bitwise
        flagPINValidated: (flagsValue & 128) !== 0,
      };
    }, processErrorResponse);
  }

  async getAddressAndPubKey(path, sigalgo, hashalgo) {
    const serializedPathBuffer = serializePathv1(path);
    console.log(serializedPathBuffer);

    const data = Buffer.concat([ serializedPathBuffer, Buffer.from([hashalgo, sigalgo]) ])

    return this.transport
      .send(CLA, INS.GET_PUBKEY, P1_VALUES.ONLY_RETRIEVE, 0, data, [0x9000])
      .then(processGetAddrResponse, processErrorResponse);
  }

  async showAddressAndPubKey(path, sigalgo, hashalgo) {
    const serializedPathBuffer = serializePathv1(path);
    console.log(serializedPathBuffer);

    const data = Buffer.concat([ serializedPathBuffer, Buffer.from([hashalgo, sigalgo]) ])

    return this.transport
      .send(CLA, INS.GET_PUBKEY, P1_VALUES.SHOW_ADDRESS_IN_DEVICE, 0, data, [0x9000])
      .then(processGetAddrResponse, processErrorResponse);
  }

  async signSendChunk(chunkIdx, chunkNum, chunk) {
    return signSendChunkv1(this, chunkIdx, chunkNum, chunk);
  }

  async sign(path, message, sigalgo, hashalgo) {
    return this.signGetChunks(path, message, sigalgo, hashalgo).then((chunks) => {
      return this.signSendChunk(1, chunks.length, chunks[0]).then(async (response) => {
        let result = {
          returnCode: response.returnCode,
          errorMessage: response.errorMessage,
          signatureCompact: null,
          signatureDER: null,
        };

        for (let i = 1; i < chunks.length; i += 1) {
          // eslint-disable-next-line no-await-in-loop
          result = await this.signSendChunk(1 + i, chunks.length, chunks[i]);
          if (result.returnCode !== ERROR_CODE.NoError) {
            break;
          }
        }

        return {
          returnCode: result.returnCode,
          errorMessage: result.errorMessage,
          // ///
          signatureCompact: result.signatureCompact,
          signatureDER: result.signatureDER,
        };
      }, processErrorResponse);
    }, processErrorResponse);
  }

  async slotStatus() {
    return this.transport.send(CLA, INS.SLOT_STATUS, 0, 0).then((response) => {
      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

      return {
        returnCode,
        errorMessage: errorCodeToString(returnCode),
        status: response.slice(0, 64),
      };
    }, processErrorResponse);
  }

  async getSlot(slotIdx) {
    if (isNaN(slotIdx) || slotIdx < 0 || slotIdx > 63) {
      return {
        returnCode: 0,
        errorMessage: "slotIdx should be a number between 0 and 63 inclusively",
      };
    }

    const payload = Buffer.from([slotIdx]);
    return this.transport.send(CLA, INS.GET_SLOT, 0, 0, payload).then((response) => {
      console.log(response);

      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

      const pathStr = printBIP44Path(response.slice(8, 28));

      return {
        returnCode,
        errorMessage: errorCodeToString(returnCode),
        slotIdx: slotIdx,
        account: response.slice(0, 8).toString("hex"),
        path: pathStr
      };
    }, processErrorResponse);
  }

  async setSlot(slotIdx, account, path) {
    if (isNaN(slotIdx) || slotIdx < 0 || slotIdx > 63) {
      return {
        returnCode: 0,
        errorMessage: "slotIdx should be a number between 0 and 63 inclusively",
      };
    }

    const serializedSlotIdx = Buffer.from([slotIdx]);
    const serializedAccount = Buffer.from(account, "hex");
    const serializedPath = serializePathv1(path);

    if (serializedAccount.length !== 8) {
      return {
        returnCode: 0,
        errorMessage: "account is expected to be a hexstring 16 characters long",
      };
    }

    console.log(serializedSlotIdx)
    console.log(serializedAccount)
    console.log(serializedPath)

    const payload = Buffer.concat([ serializedSlotIdx, serializedAccount, serializedPath]);
    console.log(payload)

    return this.transport.send(CLA, INS.SET_SLOT, 0, 0, payload).then((response) => {
      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

      return {
        returnCode,
        errorMessage: errorCodeToString(returnCode),
      };
    }, processErrorResponse);
  }

}
