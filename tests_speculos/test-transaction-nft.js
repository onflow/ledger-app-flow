'use strict';

import merge from "deepmerge";
import { testStart, testEnd, testStep, compareInAPDU, compareOutAPDU, noMoreAPDUs, compareGetVersionAPDUs, getScriptName, getSpeculosDefaultConf, humanTime } from "./speculos-common.js";
import { getSpyTransport } from "./speculos-transport.js";
import { getButtonsAndSnapshots } from "./speculos-buttons-and-snapshots.js";
import { default as OnflowLedgerMod } from "@onflow/ledger";
import { fileURLToPath, pathToFileURL } from 'url';
import assert from 'assert/strict';
import { encodeTransactionPayload, encodeTransactionEnvelope } from "@onflow/encode";
import pkg from 'elliptic';
const {ec: EC} = pkg;
import jsSHA from "jssha";

const scriptName = getScriptName(fileURLToPath(import.meta.url));
testStart(scriptName);

const speculosConf = getSpeculosDefaultConf();
const transport = await getSpyTransport(speculosConf);
const FlowApp = OnflowLedgerMod.default;
const app = new FlowApp(transport);
const device = getButtonsAndSnapshots(scriptName, speculosConf);

const ECDSA_SECP256K1 = { name: "secp256k1", code: FlowApp.Signature.SECP256K1, pathCode: 0x200 };
const ECDSA_P256 = { name: "p256", code: FlowApp.Signature.P256, pathCode: 0x300};

const SHA2_256 = { name: "SHA-256", code: FlowApp.Hash.SHA2_256, pathCode: 0x01};
const SHA3_256 = { name: "SHA3-256", code: FlowApp.Hash.SHA3_256, pathCode: 0x03};

const path = `m/44'/539'/0'/0/0`;
const options = ECDSA_P256.code | SHA3_256.code

const EMULATOR = "Emulator";
const TESTNET = "Testnet";
const MAINNET = "Mainnet";

const ADDRESS_EMULATOR = "ed2d4f9eb8bcd4ac";
const ADDRESS_TESTNET = "99a8ac2c71d4f6bd";
const ADDRESS_MAINNET = "f19c161bc24cf4b4";

const ADDRESSES = {
    [EMULATOR]: ADDRESS_EMULATOR,
    [TESTNET]: ADDRESS_TESTNET,
    [MAINNET]: ADDRESS_MAINNET,
};

const basePayloadTx = (network) => {
	const address = ADDRESSES[network];

	return {
		script: "",
		arguments: [],
		refBlock: "f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b",
		gasLimit: 42,
		proposalKey: {
		address: address,
		keyId: 4,
		sequenceNum: 10,
		},
		payer: address,
		authorizers: [address],
	};
};

const combineMerge = (target, source, options) => {
	// empty list always overwrites target
	if (source.length == 0) return source
  
	const destination = target.slice()
  
	source.forEach((item, index) => {
	  if (typeof destination[index] === "undefined") {
		destination[index] = options.cloneUnlessOtherwiseSpecified(item, options)
	  } else if (options.isMergeableObject(item)) {
		destination[index] = merge(target[index], item, options)
	  } else if (target.indexOf(item) === -1) {
		destination.push(item)
	  }
	})
  
	return destination
};
  
const buildPayloadTx = (network, partialTx) =>
    merge(basePayloadTx(network), partialTx, {arrayMerge: combineMerge});


const getTxEnvelope = (script, args) => {
	const tx = buildPayloadTx(MAINNET, {
		script: script,
		arguments: args,
	  })
	return encodeTransactionEnvelope({...tx, payloadSigs: []});	
}

//--------------------------TESTS-----------------------------

await device.makeStartingScreenshot();

// We get pubkey so we can verify signature
testStep(" - - -", "await app.getAddressAndPubKey() // path=" + path);
const getPubkeyResponse = await app.getAddressAndPubKey(path, options);
assert.equal(getPubkeyResponse.returnCode, 0x9000);
assert.equal(getPubkeyResponse.errorMessage, "No errors");
const pubkeyHex = getPubkeyResponse.publicKey.toString("hex")


{
	const contractName = "contractName"
	const contractAddress = "contractAddress"
	const storagePath = "storagePath"
	const publicCollectionContractName = "publicCollectionContractName"
	const publicCollectionName = "publicCollectionName"
	const publicPath = "publicPath"
	const script =
`import NonFungibleToken from 0xNONFUNGIBLETOKEN
import MetadataViews from 0xMETADATAVIEWS
import ${contractName} from ${contractAddress}
transaction {
  prepare(acct: AuthAccount) {
    let collectionType = acct.type(at: /storage/${storagePath})
    // if there already is a collection stored, return
    if (collectionType != nil) {
      return
    }
    // create empty collection
    let collection <- ${contractName}.createEmptyCollection()
    // put the new Collection in storage
    acct.save(<-collection, to: /storage/${storagePath})
    // create a public capability for the collection
    acct.link<&{NonFungibleToken.CollectionPublic, NonFungibleToken.Receiver, ${publicCollectionContractName}.${publicCollectionName}, MetadataViews.ResolverCollection}>(
      /public/${publicPath},
      target: /storage/${storagePath}
    )
  }
}
`
	const args = []
	const txBlob = Buffer.from(getTxEnvelope(script, args), "hex")
	console.log("Script")
	console.log(script)
	console.log("Tx built")
	console.log(txBlob.toString("hex"))

	testStep(" - - -", "NFT 1 correct");
    const signPromise =  app.sign(path, txBlob, options, "nft1");
    await device.review("Review transaction");
	const signResponse = await signPromise;
	console.log(signResponse)
	assert.equal(signResponse.returnCode, 0x9000);
	assert.equal(signResponse.errorMessage, "No errors");
	
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-transaction");
	const hasher = new jsSHA(SHA3_256.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(txBlob);
	const digestHex = hasher.getHash("HEX");
	const ec = new EC(ECDSA_P256.name);
	assert.ok(ec.verify(digestHex, signResponse.signatureDER.toString("hex"), pubkeyHex, 'hex'));
}

{
	const contractName = "contractName"
	const contractAddress = "contractAddress"
	const storagePath = "storagePath"
	const publicPath = "publicPath"
	const script =
`import NonFungibleToken from 0xNONFUNGIBLETOKEN
import ${contractName} from ${contractAddress}
transaction(recipient: Address, withdrawID: UInt64) {
  // local variable for storing the transferred nft
  let transferToken: @NonFungibleToken.NFT
  prepare(owner: AuthAccount) {
      // check if collection exists
      if (owner.type(at: /storage/${storagePath}) != Type<@${contractName}.Collection>()) {
        panic("Could not borrow a reference to the stored collection")
      }
      // borrow a reference to the collection
      let collectionRef = owner
        .borrow<&${contractName}.Collection>(from: /storage/${storagePath})!
      // withdraw the NFT
      self.transferToken <- collectionRef.withdraw(withdrawID: withdrawID)
  }
  execute {
      // get the recipient's public account object
      let recipient = getAccount(recipient)
      // get receivers capability
      let nonFungibleTokenCapability = recipient
        .getCapability<&{NonFungibleToken.CollectionPublic}>(/public/${publicPath})
      // check the recipient has a NonFungibleToken public capability
      if (!nonFungibleTokenCapability.check()) {
        panic("Could not borrow a reference to the receiver's collection")
      }
      // deposit nft to recipients collection
      nonFungibleTokenCapability
        .borrow()!
        .deposit(token: <-self.transferToken)
  }
}
`
	const args = [
		{
		  	"type": "Address",
		  	"value": "Ny nice address"
		},
		{
  			"type": "UInt64",
		  	"value": "123456"
		},  
	]
	const txBlob = Buffer.from(getTxEnvelope(script, args), "hex")
	console.log("Script")
	console.log(script)
	console.log("Tx built")
	console.log(txBlob.toString("hex"))

	testStep(" - - -", "NFT 2 correct");
    const signPromise =  app.sign(path, txBlob, options, "nft2");
    await device.review("Review transaction");
	const signResponse = await signPromise;
	console.log(signResponse)
	assert.equal(signResponse.returnCode, 0x9000);
	assert.equal(signResponse.errorMessage, "No errors");
	
	let tag = Buffer.alloc(32);
	tag.write("FLOW-V0.0-transaction");
	const hasher = new jsSHA(SHA3_256.name, "UINT8ARRAY");
	hasher.update(tag);
	hasher.update(txBlob);
	const digestHex = hasher.getHash("HEX");
	const ec = new EC(ECDSA_P256.name);
	assert.ok(ec.verify(digestHex, signResponse.signatureDER.toString("hex"), pubkeyHex, 'hex'));
}


await transport.close()
testEnd(scriptName);
process.stdin.pause()
