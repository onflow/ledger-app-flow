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

import { serializePath, validateCryptoOptions, printBIP44Path} from "./serializePath";
import { signGetChunks, signIsLastAPDU } from "./signTransaction";
import { CLA, ERROR_CODE, errorCodeToString, getVersion, INS, P1_VALUES, PKLEN, processErrorResponse } from "./common";

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
      P256: 0x0200,
      SECP256K1: 0x0300,
    }
  }

  constructor(transport) {
    if (!transport) {
      throw new Error("Transport has not been defined");
    }
    this.transport = transport;
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

  async getAddressAndPubKey(path, cryptoOptions) {
    validateCryptoOptions(cryptoOptions);
    const getVersionResponse = await this.getVersion();
    const serializedPath = serializePath(path, getVersionResponse, cryptoOptions);

    return this.transport
      .send(CLA, INS.GET_PUBKEY, P1_VALUES.ONLY_RETRIEVE, 0, serializedPath, [0x9000])
      .then(processGetAddrResponse, processErrorResponse);
  }

  async showAddressAndPubKey(path, cryptoOptions) {
    validateCryptoOptions(cryptoOptions);
    const getVersionResponse = await this.getVersion();
    const serializedPath = serializePath(path, getVersionResponse, cryptoOptions);

    return this.transport
      .send(CLA, INS.GET_PUBKEY, P1_VALUES.SHOW_ADDRESS_IN_DEVICE, 0, serializedPath, [0x9000])
      .then(processGetAddrResponse, processErrorResponse);
  }

  async sign(path, message, cryptoOptions, scriptHash) {
    validateCryptoOptions(cryptoOptions);
    const getVersionResponse = await this.getVersion();
    const chunks = signGetChunks(path, cryptoOptions, getVersionResponse, message, scriptHash)

    if (typeof chunks === "string") {
      return {
        signatureCompact: null,
        signatureDER: null,
        returnCode: 0xffff,
        errorMessage: chunks,
      }
    }

    let result = {
      signatureCompact: null,
      signatureDER: null,
      returnCode: 0xffff,
      errorMessage: "Result not initialized",
    }

    for (let i = 0; i < chunks.length; i += 1) {
      const payloadType = chunks[i].type
      const chunk = chunks[i].buffer

      // eslint-disable-next-line no-await-in-loop
      result = await this.transport
        .send(CLA, INS.SIGN, payloadType, 0, chunk, [0x9000, 0x6984, 0x6a80])
        .then((response) => {
          const errorCodeData = response.slice(-2);
          const returnCode = errorCodeData[0] * 256 + errorCodeData[1];
          const errorMessage = errorCodeToString(returnCode);

          // these error codes contain detailed error description in the response
          if (returnCode === 0x6a80 || returnCode === 0x6984) {
            const detailedErrorMessage = `${errorMessage} : ${response.slice(0, response.length - 2).toString("ascii")}`;
            return {
              signatureCompact: null,
              signatureDER: null,
              returnCode: returnCode,
              errorMessage: detailedErrorMessage,
            };
          }

          // last APDU, everything OK
          if (returnCode == 0x9000 && signIsLastAPDU(payloadType)) {
            const signatureCompact = response.slice(0, 65);
            const signatureDER = response.slice(65, response.length - 2);
            return {
              signatureCompact,
              signatureDER,
              returnCode: returnCode,
              errorMessage: errorMessage,
            };  
          }    

          // Here return code should be 0x9000 as for error codes other than [0x9000, 0x6984, 0x6a80] send throws
          return {
            signatureCompact: null,
            signatureDER: null,
            returnCode: returnCode,
            errorMessage: errorMessage,
          };
        }, processErrorResponse);
        
      // If any error accours we do not send further APDUs
      if (result.returnCode !== ERROR_CODE.NoError) {
        break;
      }
    }

    return {
      returnCode: result.returnCode,
      errorMessage: result.errorMessage,
      signatureCompact: result.signatureCompact,
      signatureDER: result.signatureDER,
    };
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

      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];

      const pathStr = printBIP44Path(response.slice(8, 28));

      return {
        returnCode,
        errorMessage: errorCodeToString(returnCode),
        slotIdx: slotIdx,
        account: response.slice(0, 8).toString("hex"),
        path: pathStr,
        options: response.length >= 32 ? response.readInt16LE(28) : null, //ledger app versions <0.9.12 do not return options
      };
    }, processErrorResponse);
  }

  async setSlot(slotIdx, account, path, cryptoOptions) {
    if (isNaN(slotIdx) || slotIdx < 0 || slotIdx > 63) {
      return {
        returnCode: 0,
        errorMessage: "slotIdx should be a number between 0 and 63 inclusively",
      };
    }

    validateCryptoOptions(cryptoOptions);

    const serializedSlotIdx = Buffer.from([slotIdx]);
    const serializedAccount = Buffer.from(account, "hex");
  
    const getVersionResponse = await this.getVersion();
    const serializedPath = serializePath(path, getVersionResponse, cryptoOptions);

    if (serializedAccount.length !== 8) {
      return {
        returnCode: 0,
        errorMessage: "account is expected to be a hexstring 16 characters long",
      };
    }

    const payload = Buffer.concat([ serializedSlotIdx, serializedAccount, serializedPath]);

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
