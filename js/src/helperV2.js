import {CLA, errorCodeToString, INS, PAYLOAD_TYPE, processErrorResponse} from "./common";

export async function signSendChunkv2(app, chunkIdx, chunkNum, chunk) {
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
  
  return app.transport
    .send(CLA, INS.SIGN, payloadType, 0, chunk, [0x9000, 0x6984, 0x6a80])
    .then((response) => {
      const errorCodeData = response.slice(-2);
      const returnCode = errorCodeData[0] * 256 + errorCodeData[1];
      let errorMessage = errorCodeToString(returnCode);

      if (returnCode === 0x6a80 || returnCode === 0x6984) {
        errorMessage = `${errorMessage} : ${response.slice(0, response.length - 2).toString("ascii")}`;
      }

      let signatureCompact = null;
      let signatureDER = null;
      if (response.length > 2) {
        signatureCompact = response.slice(0, 65);
        signatureDER = response.slice(65, response.length - 2);
      }

      return {
        signatureCompact,
        signatureDER,
        returnCode: returnCode,
        errorMessage: errorMessage,
      };
    }, processErrorResponse);
}
