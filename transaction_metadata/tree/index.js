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
const ARGUMENT_TYPE_STRING = 4;
const ARGUMENT_TYPE_HASH_ALGO = 5;
const ARGUMENT_TYPE_SIGNATURE_ALGO = 6;
const ARGUMENT_TYPE_NODE_ROLE = 7;
const JSMN_STRING = 3;

const uint8_to_buff = (n) => {
  const buff = Buffer.allocUnsafe(1);
  buff.writeUint8(n);
  return buff;
}

//This is to implement custom rules to make the labels shorter
const legerifyTxName = (name) => {
  const txNameTransforms = {
    "Transfer Fungible Token with Address":"Transfer Fungible Token with Addr"
  }
  return txNameTransforms[name]?txNameTransforms[name]:name
}

const legerifyArgLabel = (name) => {
  const txArgTransforms = {
    "Networking Address":"Netw. Address",
    "Networking Key":"Netw. Key",
    "Staking Key PoP":"St. Key PoP",
    "Public Keys":"Pub. Key",
    "Machine Account Public Key":"MA Pub. Key",
    "Raw Value for Machine Account Hash Algorithm Enum":"MA Hash Alg.",
    "Raw Value for Machine Account Signature Algorithm Enum":"MA Sign. Alg.",
    "Raw Value for Signature Algorithm Enum":"Signature Alg.",
    "Raw Value for Hash Algorithm Enum":"Hash Alg.",
    "FT Contract Address":"FT Contract Addr.",
    "Sender's Collection Path Identifier": "Sen. Coll Path Id",
    "Recipient's Receiver Path Identifier": "Rec. Coll Path Id",
    "NFT Contract Address": "NFT Contract Addr",
    "NFT ID to Transfer": "NFT ID to Transf",
    "NFT Contract Address":"NFT Contract Addr"
  }
  return txArgTransforms[name]?txArgTransforms[name]:name
}

const enumToType = (name) => {
  const argTransforms = {
    "Raw Value for Machine Account Hash Algorithm Enum":ARGUMENT_TYPE_HASH_ALGO,
    "Raw Value for Machine Account Signature Algorithm Enum":ARGUMENT_TYPE_SIGNATURE_ALGO,
    "Raw Value for Signature Algorithm Enum":ARGUMENT_TYPE_SIGNATURE_ALGO,
    "Raw Value for Hash Algorithm Enum":ARGUMENT_TYPE_HASH_ALGO,
    "Node Role": ARGUMENT_TYPE_NODE_ROLE
  }
  return argTransforms[name]?argTransforms[name]:ARGUMENT_TYPE_NORMAL
}

const readManifest = (testnetFile, mainnetFile) => {
  const sortFun = (template1, template2) => template1.id > template2.id ? 1 : (template1.id < template2.id ? -1 : 0);
  const testnetTemplates = [...JSON.parse(fs.readFileSync(testnetFile)).templates].sort(sortFun);
  const mainnetTemplates = [...JSON.parse(fs.readFileSync(mainnetFile)).templates].sort(sortFun);
  
  //validate that the manifest files match
  console.assert(testnetTemplates.length == mainnetTemplates.length);
  for(let i=0; i<testnetTemplates.length; i++) {
    console.assert(mainnetTemplates[i].id === testnetTemplates[i].id);
    console.assert(mainnetTemplates[i].arguments.length === testnetTemplates[i].arguments.length);
  }

  const templatesToMetadata = (templateTestnet, templateMainnet) => {
    const processArg = (arg, idx) => {
      if (arg.type == "String") {
        return Buffer.concat([
          uint8_to_buff(ARGUMENT_TYPE_STRING),                            //argument type
          Buffer.from(legerifyArgLabel(arg.label)),                       //argument label
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(idx),                                             //order in which should arguments display
        ])  
      }
      if (arg.type == "UInt8" && enumToType(arg.label) != ARGUMENT_TYPE_NORMAL) {
        return Buffer.concat([
          uint8_to_buff(enumToType(arg.label)),                           //argument type
          Buffer.from(legerifyArgLabel(arg.label)),                       //argument label
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(idx),                                             //order in which should arguments display
        ])  
      }
      if (arg.type[0] !== '[' && arg.type[arg.type.length-1] !== '?') {
        return Buffer.concat([
          uint8_to_buff(ARGUMENT_TYPE_NORMAL),                            //argument type
          Buffer.from(legerifyArgLabel(arg.label)),                       //argument label
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
          Buffer.from(legerifyArgLabel(arg.label)),                       //argument label
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
          Buffer.from(legerifyArgLabel(arg.label)),                       //argument label
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
          Buffer.from(legerifyArgLabel(arg.label)),                       //argument label
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(idx),                                             //order in which should arguments display
          Buffer.from(arg.type.slice(1, -2)),                             //argument type
          Buffer.from("00", "hex"),                                       //trailing 0
          uint8_to_buff(JSMN_STRING),                                     //Argument encoding
        ])  
      }
    }

    const result = Buffer.concat([
      Buffer.from("02", "hex"),                           // number of hashes
      Buffer.from(templateTestnet.hash, "hex"),           // hash testnet
      Buffer.from(templateMainnet.hash, "hex"),           // hash mainnet
      Buffer.from(legerifyTxName(templateMainnet.name)),  // transaction name
      Buffer.from("00", "hex"),                           // trailing 0 after name
      uint8_to_buff(templateMainnet.arguments.length),      // number of arguments
      Buffer.concat(templateMainnet.arguments.map((arg, idx) => processArg(arg, idx))),
    ])
    
    if (result.length > 255) {
      console.log(result.toString('hex'))
      throw new Error("Metadata too long!");
    }

    return result
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

const metadataManifest = readManifest("../manifest.testnet.json", "../manifest.mainnet.json");
const txMetadata = metadataManifest

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

// cut empty top level branches
const emptyTopLevelBranchSHA = "94a4bf5f458f2def50f807bf419501bfd5e77a084c30592aa3803a522a3c272e"
for(let branch=0; branch<7; branch++) {
  if (merkleTree.children[branch].hash === emptyTopLevelBranchSHA) {
    for(let subbranch=0; subbranch<7; subbranch++) {
      merkleTree.children[branch].children[subbranch] = "Empty branch";
    }
  }
}

const data = "" +
    "export const merkleTree = " + JSON.stringify(merkleTree, null, 2) + "\n\n" +
    "export const merkleIndex = " + JSON.stringify(merkleIndex, null, 2) + "\n\n";

fs.writeFileSync("../txMerkleTree.js", data);
fs.writeFileSync("../txMerkleTree.mjs", data);

const data2 = "# pylint: skip-file\n" +
    "merkleTree = " + JSON.stringify(merkleTree, null, 2) + "\n\n" +
    "merkleIndex = " + JSON.stringify(merkleIndex, null, 2) + "\n\n";

fs.writeFileSync("../txMerkleTree.py", data2);

