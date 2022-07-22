const fs = require("fs");
const jsSHA = require("jssha");
const { type } = require("os");

const MERKLE_TREE_DEPTH = 4

//These data are not present in the manifest file, for now, we use these values as they match all current transactions
const MIN_ARRAY_LENGTH = 0
const MAX_ARRAY_LENGTH = 3

const ARGUMENT_TYPE_NORMAL = 1;
const ARGUMENT_TYPE_OPTIONAL = 2;
const ARGUMENT_TYPE_ARRAY = 3;
const ARGUMENT_TYPE_OPTIONALARRAY = 4;
const JSMN_STRING = 3;

const uint8_to_buff = (n) => {
  const buff = Buffer.allocUnsafe(1);
  buff.writeUint8(n);
  return buff;
}

const readManifest = (testnetFile, mainnetFile) => {
  const sortFun = (template1, template2) => template1.id > template2.id ? 1 : (template1.id < template2.id ? -1 : 0);
  const testnetTemplates = [...JSON.parse(fs.readFileSync(testnetFile)).templates].sort(sortFun);
  const mainnetTemplates = [...JSON.parse(fs.readFileSync(mainnetFile)).templates].sort(sortFun);
  
  //validate that the manifest files match
  console.assert(testnetTemplates.length == mainnetTemplates.length);
  for(let i=0; i<testnetTemplates.length; i++) {
    console.assert(testnetTemplates[i].id === testnetTemplates[i].id);
    console.assert(testnetTemplates[i].arguments.length === testnetTemplates[i].arguments.length);
  }

  const templatesToMetadata = (templateTestnet, templateMainnet) => {
    const processArg = (arg, idx) => {
      if (arg.type[0] !== '[' && arg.type[arg.type.length-1] !== '?') {
        return Buffer.concat([
          uint8_to_buff(ARGUMENT_TYPE_NORMAL),                            //argument type
          Buffer.from(arg.label),                                         //argument label
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(idx),                                             //order in which should arguments display
          Buffer.from(arg.type),                                          //argument type
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(JSMN_STRING),                                     //Argument encoding
        ])  
      }
      if (arg.type[0] !== '[' && arg.type[arg.type.length-1] === '?') {
        return Buffer.concat([
          uint8_to_buff(ARGUMENT_TYPE_OPTIONAL),                          //argument type
          Buffer.from(arg.label),                                         //argument label
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(idx),                                             //order in which should arguments display
          Buffer.from(arg.type.slice(0, -1)),                             //argument type
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(JSMN_STRING),                                     //Argument encoding
        ])  
      }
      if (arg.type[0] === '[' && arg.type[arg.type.length-1] !== '?') {
        return Buffer.concat([
          uint8_to_buff(ARGUMENT_TYPE_ARRAY),                             //argument type
          uint8_to_buff(MIN_ARRAY_LENGTH),                                //min array length
          uint8_to_buff(MAX_ARRAY_LENGTH),                                //max array length
          Buffer.from(arg.label),                                         //argument label
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(idx),                                             //order in which should arguments display
          Buffer.from(arg.type.slice(1, -1)),                             //argument type
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(JSMN_STRING),                                     //Argument encoding
        ])  
      }
      if (arg.type[0] === '[' && arg.type[arg.type.length-1] === '?') {
        return Buffer.concat([
          uint8_to_buff(ARGUMENT_TYPE_OPTIONALARRAY),                     //argument type
          uint8_to_buff(MIN_ARRAY_LENGTH),                                //min array length
          uint8_to_buff(MAX_ARRAY_LENGTH),                                //max array length
          Buffer.from(arg.label),                                         //argument label
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(idx),                                             //order in which should arguments display
          Buffer.from(arg.type.slice(1, -2)),                             //argument type
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(JSMN_STRING),                                     //Argument encoding
        ])  
      }
    }

    return Buffer.concat([
      Buffer.from("02", "hex"),                           // number of hashes
      Buffer.from(templateTestnet.hash, "hex"),           // hash testnet
      Buffer.from(templateMainnet.hash, "hex"),           // hash mainnet
      Buffer.from(templateMainnet.name),                  // transaction name
      Buffer.from("00", "hex"),                           // trailing 0 after name
      uint8_to_buff(templateMainnet.arguments.length),      // number of arguments
      Buffer.concat(templateMainnet.arguments.map((arg, idx) => processArg(arg, idx))),
    ])
  }

  return [...Array(testnetTemplates.length).keys()].map((i) => templatesToMetadata(testnetTemplates[i], mainnetTemplates[i]));
}

const getMetadataFromCMetadata = (data) => Buffer.concat(
  data.map((el) => {
    if (typeof el === "string") {
      return Buffer.from(el)
    }
    if (typeof el === "number") {
      return uint8_to_buff(el)
    }
    console.assert(false)
  })
)

const TX_METADATA_CREATE_ACCOUNT = getMetadataFromCMetadata([
  1, 
  0xee, 0xf2, 0xd0, 0x49, 0x44, 0x48, 0x55, 0x41, 0x77, 0x61, 0x2e, 0x63, 0x02, 0x62, 0x56, 0x25, 0x83, 0x39, 0x23, 0x0c, 0xbc, 0x69, 0x31, 0xde, 0xd7, 0x8d, 0x61, 0x49, 0x44, 0x3c, 0x61, 0x73,
  'C', 'r', 'e', 'a', 't', 'e', ' ', 'A', 'c', 'c', 'o', 'u', 'n', 't', 0,  //tx name (to display)
  1,
  ARGUMENT_TYPE_ARRAY, 1, 5, 
  'P', 'u', 'b', ' ', 'k', 'e', 'y', 0, //arg name (to display)
  0, //argument index
  'S', 't', 'r', 'i', 'n', 'g',  0, //expected value type
  JSMN_STRING, //expected value json token type
]);

const TX_METADATA_ADD_NEW_KEY = getMetadataFromCMetadata([
  1, //number of hashes + hashes
  0x59, 0x5c, 0x86, 0x56, 0x14, 0x41, 0xb3, 0x2b, 0x2b, 0x91, 0xee, 0x03, 0xf9, 0xe1, 0x0c, 0xa6, 0xef, 0xa7, 0xb4, 0x1b, 0xcc, 0x99, 0x4f, 0x51, 0x31, 0x7e, 0xc0, 0xaa, 0x9d, 0x8f, 0x8a, 0x42,
  'A', 'd', 'd', ' ', 'N', 'e', 'w', ' ', 'K', 'e', 'y', 0,  //tx name (to display)
  1,  //number of arguments

  //Argument 1
  ARGUMENT_TYPE_NORMAL,
  'P', 'u', 'b', ' ', 'k', 'e', 'y', 0, //arg name (to display)
  0, //argument index
  'S', 't', 'r', 'i', 'n', 'g',  0, //expected value type
  JSMN_STRING, //expected value json token type
]);

const TX_METADATA_TOKEN_TRANSFER = getMetadataFromCMetadata([
  3, //number of hashes + hashes
  0xca, 0x80, 0xb6, 0x28, 0xd9, 0x85, 0xb3, 0x58, 0xae, 0x1c, 0xb1, 0x36, 0xbc, 0xd9, 0x76, 0x99, 0x7c, 0x94, 0x2f, 0xa1, 0x0d, 0xba, 0xbf, 0xea, 0xfb, 0x4e, 0x20, 0xfa, 0x66, 0xa5, 0xa5, 0xe2,
  0xd5, 0x6f, 0x4e, 0x1d, 0x23, 0x55, 0xcd, 0xcf, 0xac, 0xfd, 0x01, 0xe4, 0x71, 0x45, 0x9c, 0x6e, 0xf1, 0x68, 0xbf, 0xdf, 0x84, 0x37, 0x1a, 0x68, 0x5c, 0xcf, 0x31, 0xcf, 0x3c, 0xde, 0xdc, 0x2d,
  0x47, 0x85, 0x15, 0x86, 0xd9, 0x62, 0x33, 0x5e, 0x3f, 0x7d, 0x9e, 0x5d, 0x11, 0xa4, 0xc5, 0x27, 0xee, 0x4b, 0x5f, 0xd1, 0xc3, 0x89, 0x5e, 0x3c, 0xe1, 0xb9, 0xc2, 0x82, 0x1f, 0x60, 0xb1, 0x66,
  'T', 'o', 'k', 'e', 'n', ' ', 'T', 'r', 'a', 'n', 's', 'f', 'e', 'r', 0,  //tx name (to display)
  2,  //number of arguments

  //Argument 1
  ARGUMENT_TYPE_NORMAL,
  'A', 'm', 'o', 'u', 'n', 't', 0, //arg name (to display)
  0, //argument index
  'U', 'F', 'i', 'x', '6', '4',  0, //expected value type
  JSMN_STRING, //expected value json token type

  //Argument 2
  ARGUMENT_TYPE_NORMAL,
  'D', 'e', 's', 't', 'i', 'n', 'a', 't', 'i', 'o', 'n', 0, //arg name (to display)
  1, //argument index
  'A', 'd', 'd', 'r', 'e', 's', 's', 0, //expected value type
  JSMN_STRING, //expected value json token type
]);


const metadataManifest = readManifest("manifest.testnet.json", "manifest.mainnet.json");
const txMetadata = [TX_METADATA_CREATE_ACCOUNT, TX_METADATA_ADD_NEW_KEY, TX_METADATA_TOKEN_TRANSFER, ...metadataManifest]

//We add empty metadata strings so we have 7^MERKLE_TREE_DEPTH elementes in the field
const desiredLength = 7**MERKLE_TREE_DEPTH;
const txMetadataFullHex = [...txMetadata.map((b) => b.toString("hex")), ...Array(desiredLength-txMetadata.length).fill("")]

const processMerkleTreeLevel = (children) => {
  const hasher = new jsSHA("SHA-256", "HEX");
  for(child of children) {
    if (typeof child === "string") {
      hasher.update(child)
    }
    else {
      hasher.update(child.hash);
    }
  }
  return {
    hash:  hasher.getHash("HEX"),
    children: children,
  }
}

//Helper to split array into 7-tuples
const splitTo7Tuples = (array) => {
  return array.reduce((result, value, index, array) => {
    if (index % 7 == 0) {
      result.push(array.slice(index, index + 7));
    }
    return result;
  }, [])
}

//Now we prepare the merkle tree
let merkleTree = txMetadataFullHex.map((txMetadata) => processMerkleTreeLevel([txMetadata])) //Level leaves
for(let i=0; i<MERKLE_TREE_DEPTH; i++) {
  merkleTree = splitTo7Tuples(merkleTree).map((seventuple) => processMerkleTreeLevel(seventuple))
}
merkleTree = merkleTree[0]

//We create an index: first 8 bytes of script hash (16 hex digits) => array of indices
const INDEX_HASH_LEN = 8
let merkleIndex = {}
//to keep it simple...
console.assert(MERKLE_TREE_DEPTH == 4);
for(let idx1=0; idx1<7; idx1++) {
  for(let idx2=0; idx2<7; idx2++) {
    for(let idx3=0; idx3<7; idx3++) {
      for(let idx4=0; idx4<7; idx4++) {
        const template = merkleTree.children[idx1].children[idx2].children[idx3].children[idx4].children[0]
        const numberOfHashes = parseInt(template.slice(0,2), 16)
        for (let hashNo=0; hashNo<numberOfHashes; hashNo++) {
          const hashIndex = template.slice(2 + 64*hashNo, 2 + 64*hashNo + 2*INDEX_HASH_LEN) //64 = 2*sha256 size (2 is for hex representation)
          merkleIndex[hashIndex] = [idx1, idx2, idx3, idx4] 
        }
      }
    }
  }
}

const data = "" +
    "export const merkleTree = " + JSON.stringify(merkleTree, null, 2) + "\n\n" +
    "export const merkleIndex = " + JSON.stringify(merkleIndex, null, 2) + "\n\n";

fs.writeFileSync("txMerkleTree.js", data);

