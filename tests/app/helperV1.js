import {CLA, errorCodeToString, INS, PAYLOAD_TYPE, processErrorResponse} from "./common";

const HARDENED = 0x80000000;

export function serializePathv1(path) {
  if (typeof path !== "string") {
    throw new Error("Path should be a string (e.g \"m/44'/1'/5'/0/3\")");
  }

  if (!path.startsWith("m")) {
    throw new Error('Path should start with "m" (e.g "m/44\'/461\'/5\'/0/3")');
  }

  const pathArray = path.split("/");

  if (pathArray.length !== 6) {
    throw new Error("Invalid path. (e.g \"m/44'/1'/5'/0/3\")");
  }

  const buf = Buffer.alloc(20);

  for (let i = 1; i < pathArray.length; i += 1) {
    let value = 0;
    let child = pathArray[i];
    if (child.endsWith("'")) {
      value += HARDENED;
      child = child.slice(0, -1);
    }

    const childNumber = Number(child);

    if (Number.isNaN(childNumber)) {
      throw new Error(`Invalid path : ${child} is not a number. (e.g "m/44'/1'/5'/0/3")`);
    }

    if (childNumber >= HARDENED) {
      throw new Error("Incorrect child value (bigger or equal to 0x80000000)");
    }

    value += childNumber;

    buf.writeUInt32LE(value, 4 * (i - 1));
  }

  return buf;
}

function printBIP44Item(v) {
  let hardened = v >= 0x8000000;
  return `${v & 0x7FFFFFFF}${hardened ? "'" : ""}`;
}

export function printBIP44Path(pathBytes) {
  if (pathBytes.length !== 20) {
    throw new Error("Invalid bip44 path");
  }

  let pathValues = [0, 0, 0, 0, 0];
  for (let i = 0; i < 5; i += 1) {
    pathValues[i] = pathBytes.readUInt32LE(4 * i);
    console.log(pathValues[i]);
  }

  return `m/${
    printBIP44Item(pathValues[0])
  }/${
    printBIP44Item(pathValues[1])
  }/${
    printBIP44Item(pathValues[2])
  }/${
    printBIP44Item(pathValues[3])
  }/${
    printBIP44Item(pathValues[4])
  }`;
}

export async function signSendChunkv1(app, chunkIdx, chunkNum, chunk) {
  let payloadType = PAYLOAD_TYPE.ADD;
  if (chunkIdx === 1) {
    payloadType = PAYLOAD_TYPE.INIT;
  }
  if (chunkIdx === chunkNum) {
    payloadType = PAYLOAD_TYPE.LAST;
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
