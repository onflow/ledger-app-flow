import { compareVersion } from "./common";

const HARDENED = 0x80000000

//version 0
//we serialize just 20 bytes of hdpath
//version 1
//we serialize two additional bytes from curveHashOption
export function serializePath(path, getVersionResponse, curveHashOption) {
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

    const version = (compareVersion(getVersionResponse, 0, 10, 0) >= 0) ? 1 : 0
  
    const buf = (version === 0) ? Buffer.alloc(20): Buffer.alloc(22);
    if (version > 0) {
      buf.writeUInt16LE(curveHashOption, 20);
    }
  
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
  
  export function validateCryptoOptions(cryptoOptions) {
    if (typeof cryptoOptions !== "number" || !Number.isInteger(cryptoOptions) || cryptoOptions<0 || cryptoOptions>=65536) {
      throw new Error("CryptoOptions should be an integer that fits into 16bits.");
    }
  }