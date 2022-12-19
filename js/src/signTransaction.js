import { compareVersion, CHUNK_SIZE } from "./common";
import { serializePath } from "./serializePath";
import { merkleIndex, merkleTree } from "./txMerkleTree"

const PAYLOAD_TYPE = {
    INIT: 0x00,
    ADD: 0x01,
    LAST: 0x02,
    TX_METADATA: 0x03,
    MERKLE_TREE: 0x04,
    MERKLE_TREE_LAST: 0x05,
    MESSAGE_LAST: 0x10,
}

const P2_UNUSED = 0x00;

const PAYLOAD_TYPE_LAST_P2 = {
  ARBITRARY_MESSAGE_SIGNING: 0x01,
  NFT_SETUP_COLLECTION: 0x02,
  NFT_TRANSFER: 0x03,
}

export function signIsLastAPDU(type) {
    return (type === PAYLOAD_TYPE.LAST || type === PAYLOAD_TYPE.MERKLE_TREE_LAST || PAYLOAD_TYPE.MESSAGE_LAST)
}
  
/*
  Prepare chunks functions
*/
function prepareBasicChunks(serializedPathBuffer, message) {
    const chunks = [];

    // First chunk (only path)
    chunks.push({type: PAYLOAD_TYPE.INIT, p2: P2_UNUSED, buffer: serializedPathBuffer});

    const messageBuffer = Buffer.from(message);

    const buffer = Buffer.concat([messageBuffer]);
    for (let i = 0; i < buffer.length; i += CHUNK_SIZE) {
      let end = i + CHUNK_SIZE;
      if (i > buffer.length) {
        end = buffer.length;
      }
      chunks.push({type: PAYLOAD_TYPE.ADD, p2: P2_UNUSED, buffer:buffer.slice(i, end)});
    }

    return chunks;
}

function signGetChunksv1(path, options, getVersionResponse, message) {
  const serializedPath = serializePath(path, getVersionResponse, options);
  const chunks = prepareBasicChunks(serializedPath, message);
  chunks[chunks.length-1].type = PAYLOAD_TYPE.LAST
  return chunks;
}

//ExtraInfo is either
// - script hash from merkleIndex - initiates transaction signing with metadata
// - "Sign message" - initiates message signing
// - "nft1" - initiates Setup NFT collection signing
// - "nft2" - initiates Transfer NFT signing
// - anything else - initiates transaction sining without metadata
function signGetChunksv2(path, options, getVersionResponse, message, extraInfo) {
    const serializedPath = serializePath(path, getVersionResponse, options);
    const basicChunks = prepareBasicChunks(serializedPath, message)

    if (extraInfo == "Sign message") {
      basicChunks[basicChunks.length-1].type = PAYLOAD_TYPE.MESSAGE_LAST
      basicChunks[basicChunks.length-1].p2 = P2_UNUSED
      return basicChunks;
    }

    if (extraInfo == "nft1") {
      basicChunks[basicChunks.length-1].type = PAYLOAD_TYPE.LAST
      basicChunks[basicChunks.length-1].p2 = PAYLOAD_TYPE_LAST_P2.NFT_SETUP_COLLECTION
      return basicChunks;
    }

    if (extraInfo == "nft2") {
      basicChunks[basicChunks.length-1].type = PAYLOAD_TYPE.LAST
      basicChunks[basicChunks.length-1].p2 = PAYLOAD_TYPE_LAST_P2.NFT_TRANSFER
      return basicChunks;
    }

    // We try to find hash in the merkle tree. If it is not there, we send the tx without metadata (arbitrary tx signing in expert mode)
    const scriptHash = extraInfo
    const merkleI = merkleIndex[scriptHash.slice(0, 16)]
    if (merkleI === undefined) {
        basicChunks[basicChunks.length-1].type = PAYLOAD_TYPE.LAST
        basicChunks[basicChunks.length-1].p2 = PAYLOAD_TYPE_LAST_P2.ARBITRARY_MESSAGE_SIGNING
        return basicChunks;
    }
    // other chunks
    const metadata = merkleTree.children[merkleI[0]].children[merkleI[1]].children[merkleI[2]].children[merkleI[3]].children[0]
    const merkleTreeLevel1 = merkleTree.children[merkleI[0]].children[merkleI[1]].children[merkleI[2]].children.map((ch) => ch.hash).join('')
    const merkleTreeLevel2 = merkleTree.children[merkleI[0]].children[merkleI[1]].children.map((ch) => ch.hash).join('')
    const merkleTreeLevel3 = merkleTree.children[merkleI[0]].children.map((ch) => ch.hash).join('')
    const merkleTreeLevel4 = merkleTree.children.map((ch) => ch.hash).join('')

    return [
        ...basicChunks, 
        { type: PAYLOAD_TYPE.TX_METADATA, p2: P2_UNUSED, buffer: Buffer.from(metadata, "hex"), },
        { type: PAYLOAD_TYPE.MERKLE_TREE, p2: P2_UNUSED, buffer: Buffer.from(merkleTreeLevel1, "hex"), },
        { type: PAYLOAD_TYPE.MERKLE_TREE, p2: P2_UNUSED, buffer: Buffer.from(merkleTreeLevel2, "hex"), },
        { type: PAYLOAD_TYPE.MERKLE_TREE, p2: P2_UNUSED, buffer: Buffer.from(merkleTreeLevel3, "hex"), },
        { type: PAYLOAD_TYPE.MERKLE_TREE_LAST, p2: P2_UNUSED, buffer: Buffer.from(merkleTreeLevel4, "hex"), },
    ]
}

export function signGetChunks(path, options, getVersionResponse, message, scriptHash) {
    if (compareVersion(getVersionResponse, 0, 10, 3) <= 0) {
        return signGetChunksv1(path, options, getVersionResponse, message)
    }

    return signGetChunksv2(path, options, getVersionResponse, message, scriptHash);
}