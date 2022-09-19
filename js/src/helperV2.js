import {CLA, errorCodeToString, INS, PAYLOAD_TYPE, processErrorResponse} from "./common";

export async function signSendChunkv2(app, chunkIdx, chunkNum, chunk) {
  if (chunkNum <= 5) {
    return processErrorResponse("Invalid chunks")
  }
  let payloadType = PAYLOAD_TYPE.ADD;
  if (chunkIdx === 1) {
    payloadType = PAYLOAD_TYPE.INIT;
  }
  if (chunkIdx === chunkNum - 4) {
    payloadType = PAYLOAD_TYPE.TEMPLATE;
  }
  if (chunkIdx === chunkNum - 3) {
    payloadType = PAYLOAD_TYPE.MERKLE_TREE;
  }
  if (chunkIdx === chunkNum - 2) {
    payloadType = PAYLOAD_TYPE.MERKLE_TREE;
  }
  if (chunkIdx === chunkNum - 1) {
    payloadType = PAYLOAD_TYPE.MERKLE_TREE;
  }
  if (chunkIdx === chunkNum ) {
    payloadType = PAYLOAD_TYPE.MERKLE_TREE_LAST;
  }
 
  // transport should not throw on 
  // RETURN_CODE_OK - response contains signatures 
  // DATA_INVALID, BAD_KEY_HANDLE - response contains more specific error message
  const RETURN_CODES_WITH_ERROR_MESSAGE = [0x6984, 0x6a80];
  const RETURN_CODE_OK = 0x9000
  const RETURN_CODES_WITH_VALUE = RETURN_CODES_WITH_ERROR_MESSAGE + RETURN_CODE_OK 

  return app.transport
    .send(CLA, INS.SIGN, payloadType, 0, chunk, RETURN_CODES_WITH_VALUE)
    .then((response) => {
      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];
      let errorMessage = errorCodeToString(returnCode);

      if (RETURN_CODES_WITH_ERROR_MESSAGE.includes(returnCode)) {
        errorMessage = `${errorMessage} : ${response.slice(0, response.length - 2).toString("ascii")}`;
      }

      let signatureCompact = null;
      let signatureDER = null;
      if (returnCode === RETURN_CODE_OK) {
        signatureCompact = response.slice(0, 65);
        signatureDER = response.slice(65, response.length - 2);
      }

      return {
        signatureCompact,
        signatureDER,
        returnCode: returnCode,
        errorMessage: errorMessage,
      };
    }, processErrorResponse); //other return codes do not contain specific information, transport throws and rejected promise is handled here
}
