const merge = require("deepmerge");
const rlp = require("rlp");
const fs = require("fs");
const path = require("path");
const {
  encodeTransactionPayload, 
  encodeTransactionEnvelope,
} = require("@onflow/encode");

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

const TX_HELLO_WORLD = `transaction(msg: String) { execute { log(msg) } }`;

const TX_ADD_NEW_KEY = 
`transaction(publicKey: String) {
prepare(signer: AuthAccount) {
signer.addPublicKey(publicKey.decodeHex())
}
}`;

const TX_CREATE_ACCOUNT = 
`transaction(publicKeys: [String]) {
prepare(signer: AuthAccount) {
let acct = AuthAccount(payer: signer)
for key in publicKeys {
acct.addPublicKey(key.decodeHex())
}
}
}`;

const TX_TRANSFER_TOKENS_EMULATOR = 
`import FungibleToken from 0xee82856bf20e2aa6
transaction(amount: UFix64, to: Address) {
let vault: @FungibleToken.Vault
prepare(signer: AuthAccount) {
self.vault <- signer
.borrow<&{FungibleToken.Provider}>(from: /storage/flowTokenVault)!
.withdraw(amount: amount)
}
execute {
getAccount(to)
.getCapability(/public/flowTokenReceiver)!
.borrow<&{FungibleToken.Receiver}>()!
.deposit(from: <-self.vault)
}
}`;

const TX_TRANSFER_TOKENS_TESTNET = 
`import FungibleToken from 0x9a0766d93b6608b7
transaction(amount: UFix64, to: Address) {
let vault: @FungibleToken.Vault
prepare(signer: AuthAccount) {
self.vault <- signer
.borrow<&{FungibleToken.Provider}>(from: /storage/flowTokenVault)!
.withdraw(amount: amount)
}
execute {
getAccount(to)
.getCapability(/public/flowTokenReceiver)!
.borrow<&{FungibleToken.Receiver}>()!
.deposit(from: <-self.vault)
}
}`;

const TX_TRANSFER_TOKENS_MAINNET = 
`import FungibleToken from 0xf233dcee88fe0abe
transaction(amount: UFix64, to: Address) {
let vault: @FungibleToken.Vault
prepare(signer: AuthAccount) {
self.vault <- signer
.borrow<&{FungibleToken.Provider}>(from: /storage/flowTokenVault)!
.withdraw(amount: amount)
}
execute {
getAccount(to)
.getCapability(/public/flowTokenReceiver)!
.borrow<&{FungibleToken.Receiver}>()!
.deposit(from: <-self.vault)
}
}`;

const TXS_TRANSFER_TOKENS = {
  [EMULATOR]: TX_TRANSFER_TOKENS_EMULATOR,
  [TESTNET]: TX_TRANSFER_TOKENS_TESTNET,
  [MAINNET]: TX_TRANSFER_TOKENS_MAINNET,
};

const encodeAccountKey = (publicKey, sigAlgo, hashAlgo, weight)  =>
  rlp
    .encode([
      Buffer.from(publicKey, "hex"),
      sigAlgo,
      hashAlgo,
      weight,
    ])
    .toString("hex")

const range = (start, end) => Array.from({length: end - start}, (v,k) => start + k);

const PUBLIC_KEY = "94488a795a07700c6fb83e066cf57dfd87f92ce70cbc81cb3bd3fea2df7b67073b70e36b44f3578b43d64d3faa2e8e415ef6c2b5fe4390d5a78e238581c6e4bc";

const SIG_ALGO_UNKNOWN = 0;
const SIG_ALGO_ECDSA_P256 = 2;
const SIG_ALGO_ECDSA_SECP256K1 = 3;
const SIG_ALGO_MAX = 255;

const SIG_ALGOS = [
  SIG_ALGO_UNKNOWN,
  SIG_ALGO_ECDSA_P256,
  SIG_ALGO_ECDSA_SECP256K1,
  SIG_ALGO_MAX,
];

const HASH_ALGO_UNKNOWN = 0;
const HASH_ALGO_SHA2_256 = 1;
const HASH_ALGO_SHA3_256 = 3;
const HASH_ALGO_MAX = 255;

const HASH_ALGOS = [
  HASH_ALGO_UNKNOWN,
  HASH_ALGO_SHA2_256,
  HASH_ALGO_SHA3_256,
  HASH_ALGO_MAX,
];

const WEIGHT_MIN = 0;
const WEIGHT_MID = 500;
const WEIGHT_MAX = 1000;

const WEIGHTS = [WEIGHT_MIN, WEIGHT_MID, WEIGHT_MAX];

const ACCOUNT_KEYS = (() => {
  const accountKeys = [];

  for (const sigAlgo of SIG_ALGOS) {
    for (const hashAlgo of HASH_ALGOS) {
      for (const weight of WEIGHTS) {
        accountKeys.push(encodeAccountKey(PUBLIC_KEY, sigAlgo, hashAlgo, weight));
      }
    }
  }

  return accountKeys;
})();

const DEFAULT_ACCOUNT_KEY = encodeAccountKey(PUBLIC_KEY, SIG_ALGO_ECDSA_P256, HASH_ALGO_SHA3_256, WEIGHT_MAX);

const FLOW_AMOUNT_MIN = "0.0";
const FLOW_AMOUNT_MAX = "184467440737.9551615";

const FLOW_AMOUNTS = [
  FLOW_AMOUNT_MIN,
  FLOW_AMOUNT_MAX,
];

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

const buildEnvelopeTx = (network, partialTx) =>
  merge(baseEnvelopeTx(network), partialTx, {arrayMerge: combineMerge});

const basePayloadTx = (network) => {
  const address = ADDRESSES[network];

  return {
    script: TX_ADD_NEW_KEY,
    arguments: [{ type: "String", value: DEFAULT_ACCOUNT_KEY }],
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

const baseEnvelopeTx = (network) => {
  const address = ADDRESSES[network];

  return {
    ...basePayloadTx(network),
    payloadSigs: [
      {
        address: address,
        keyId: 4,
        sig: "f7225388c1d69d57e6251c9fda50cbbf9e05131e5adb81e5aa0422402f048162",
      },
    ],
  };
};

const createPayloadTestcase = (valid) => {
  return (x) => ({
    title: x[0],	
    valid: valid,	
    chainID: x[2],	
    payloadMessage: x[1],	
    envelopeMessage: { ...x[1], payloadSigs: [] },	
    encodedTransactionPayloadHex: encodeTransactionPayload(x[1]),	
    encodedTransactionEnvelopeHex: encodeTransactionEnvelope({ ...x[1], payloadSigs: [] }),
  });
};

const createEnvelopeTestcase = (valid) => {
  return (x) => ({	
    title: x[0],	
    valid: valid,	
    chainID: x[2],	
    payloadMessage: x[1],	
    envelopeMessage: { ...x[1], payloadSigs: [] },	
    encodedTransactionPayloadHex: encodeTransactionPayload(x[1]),	
    encodedTransactionEnvelopeHex: encodeTransactionEnvelope({ ...x[1], payloadSigs: [] }),
  });
};

const sampleArguments = (arguments) => {
  return arguments.map(({ type, sampleValue }) => {
    return {
      type: type,
      value: sampleValue,
    };
  });
};

const testnetTemplates = JSON.parse(fs.readFileSync('manifest.testnet.json')).templates;
const mainnetTemplates = JSON.parse(fs.readFileSync('manifest.mainnet.json')).templates;

const manifestTestnetPayloadCases = testnetTemplates.map((template) => {
  return [
    `${template.id} - ${template.name}`,
    buildPayloadTx(TESTNET, {
      script: template.source,
      arguments: sampleArguments(template.arguments || [], TESTNET),
    }),
    TESTNET,
  ]
});

const manifestMainnetPayloadCases = mainnetTemplates.map((template) => {
  return [
    `${template.id} - ${template.name}`,
    buildPayloadTx(MAINNET, {
      script: template.source,
      arguments: sampleArguments(template.arguments || [], MAINNET),
    }),
    MAINNET,
  ]
});

const manifestTestnetEnvelopeCases = testnetTemplates.map((template) => {
  return [
    `${template.id} - ${template.name}`,
    buildEnvelopeTx(TESTNET, {
      script: template.source,
      arguments: sampleArguments(template.arguments || [], TESTNET),
    }),
    TESTNET,
  ]
});

const manifestMainnetEnvelopeCases = mainnetTemplates.map((template) => {
  return [
    `${template.id} - ${template.name}`,
    buildEnvelopeTx(MAINNET, {
      script: template.source,
      arguments: sampleArguments(template.arguments || [], MAINNET),
    }),
    MAINNET,
  ]
});

const manifestPayloadCases = [
  ...manifestTestnetPayloadCases,
  ...manifestMainnetPayloadCases,
];

const manifestEnvelopeCases = [
  ...manifestTestnetEnvelopeCases,
  ...manifestMainnetEnvelopeCases,
];

const invalidPayloadCases = [
  [
    "Example Transaction - Invalid Payload - Unapproved Script",
    buildPayloadTx(MAINNET, {script: TX_HELLO_WORLD}), // TX_HELLO_WORLD is not an approved transaction template
    MAINNET,
  ],
  [
    "Example Transaction - Invalid Payload - Empty Script",
    buildPayloadTx(MAINNET, {script: ""}),
    MAINNET,
  ],
  [
    "Example Transaction - Invalid Payload - Too Many Authorizers",
    buildPayloadTx(MAINNET, {
      // Maximum authorizer count is 16
      authorizers: [
        "a3a627d47f628e9b",
        "b5f242e7e66684fa",
        "5195fb3af79c8425",
        "4254874588d9dcde",

        "a6333e989923dc01",
        "b0675bab0027d660",
        "5400e27611ddd6bf",
        "bb4d6932cca2a849",

        "5f2ad0efdd58a896",
        "497eb5dc445ca2f7",
        "ad190c0155a6a228",
        "1ff233dcee8f96d6",

        "fb958a01ff759609",
        "edc1ef3266719c68",
        "09a656ef778b9cb7",
        "e6ebddabaaf4e241",

        "028c6476bb0ee29e",
      ],
    }),
    MAINNET,
  ],
].map(createPayloadTestcase(false));

const validPayloadTransferCases = 
  Object.entries(TXS_TRANSFER_TOKENS).
  reduce((list, [network, script]) => [
    ...list,
    ...(FLOW_AMOUNTS.map((amount) => 
      [
        `Send Flow Token Transaction (${network}) - Valid Payload - Valid Amount ${amount}`,
        buildPayloadTx(network, {
          script: script,
          arguments: [
            {
              type: "UFix64",
              value: amount,
            },
            {
              type: "Address",
              value: `0x${ADDRESSES[network]}`,
            }
          ]
        }),
        network,
      ]
    )),
  ], []);
  
const validPayloadCases = [
  [
    "Example Transaction - Valid Payload - Zero Gas Limit",
    buildPayloadTx(MAINNET, {gasLimit: 0}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Payload - Zero proposerKey.keyId",
    buildPayloadTx(MAINNET, {proposalKey: {keyId: 0}}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Payload - Zero proposalKey.sequenceNum",
    buildPayloadTx(MAINNET, {proposalKey: {sequenceNum: 0}}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Payload - Empty Authorizers",
    buildPayloadTx(MAINNET, {authorizers: []}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Maximum Authorizers",
    buildPayloadTx(MAINNET, {
      // Maximum authorizer count is 16
      authorizers: [
        // Lists are merged in buildPayloadTx, so first authorizer is already added
        "a3a627d47f628e9b",
        "b5f242e7e66684fa",
        "5195fb3af79c8425",

        "4254874588d9dcde",
        "a6333e989923dc01",
        "b0675bab0027d660",
        "5400e27611ddd6bf",

        "bb4d6932cca2a849",
        "5f2ad0efdd58a896",
        "497eb5dc445ca2f7",
        "ad190c0155a6a228",

        "1ff233dcee8f96d6",
        "fb958a01ff759609",
        "edc1ef3266719c68",
        "09a656ef778b9cb7",
      ],
    }),
    MAINNET,
  ],
  ...validPayloadTransferCases,
  ...(ACCOUNT_KEYS.map((accountKey, i) => 
    [
      `Create Account Transaction - Valid Payload - Single Account Key #${i}`,
      buildPayloadTx(MAINNET, {
        script: TX_CREATE_ACCOUNT,
        arguments: [
          {
            type: "Array",
            value: [
              {
                type: "String",
                value: accountKey,
              }
            ]
          }
        ]
      }),
      MAINNET,
    ]
  )),
  ...(range(1, 5).map((i) => 
    [
      `Create Account Transaction - Valid Payload - Multiple Account Keys #${i}`,
      buildPayloadTx(MAINNET, {
        script: TX_CREATE_ACCOUNT,
        arguments: [
          {
            type: "Array",
            value: range(0, i).map((j) => (
              {
                type: "String",
                value: ACCOUNT_KEYS[j],
              }
            ))
          }
        ]
      }),
      MAINNET,
    ]
  )),
  ...(ACCOUNT_KEYS.map((accountKey, i) => 
    [
      `Add New Key Transaction - Valid Envelope - Valid Account Key ${i}`,
      buildEnvelopeTx(MAINNET, {
        script: TX_ADD_NEW_KEY,
        arguments: [
          {
            type: "String",
            value: accountKey,
          }
        ]
      }),
      MAINNET,
    ]
  )),
].map(createPayloadTestcase(false));

const invalidEnvelopeCases = [
  [
    "Example Transaction - Invalid Envelope - Unapproved Script",
    buildEnvelopeTx(MAINNET, {script: TX_HELLO_WORLD}), // TX_HELLO_WORLD is not an approved transaction template
    MAINNET,
  ],
  [
    "Example Transaction - Invalid Envelope - Empty Script",
    buildEnvelopeTx(MAINNET, {script: ""}),
    MAINNET,
  ],
  [
    "Example Transaction - Invalid Envelope - Too Many Authorizers",
    buildEnvelopeTx(MAINNET, {
      // Maximum authorizer count is 16
      authorizers: [
        "a3a627d47f628e9b",
        "b5f242e7e66684fa",
        "5195fb3af79c8425",
        "4254874588d9dcde",

        "a6333e989923dc01",
        "b0675bab0027d660",
        "5400e27611ddd6bf",
        "bb4d6932cca2a849",

        "5f2ad0efdd58a896",
        "497eb5dc445ca2f7",
        "ad190c0155a6a228",
        "1ff233dcee8f96d6",

        "fb958a01ff759609",
        "edc1ef3266719c68",
        "09a656ef778b9cb7",
        "e6ebddabaaf4e241",
        
        "028c6476bb0ee29e",
      ],
    }),
    MAINNET,
  ],
].map(createEnvelopeTestcase(true));

const validEnvelopeTransferCases = 
  Object.entries(TXS_TRANSFER_TOKENS).
  reduce((list, [network, script]) => [
    ...list,
    ...(FLOW_AMOUNTS.map((amount) => 
      [
        `Send Flow Token Transaction (${network}) - Valid Envelope - Valid Amount ${amount}`,
        buildEnvelopeTx(network, {
          script: script,
          arguments: [
            {
              type: "UFix64",
              value: amount,
            },
            {
              type: "Address",
              value: `0x${ADDRESSES[network]}`
            }
          ]
        }),
        network,
      ]
    )),
  ], []);

const validEnvelopeCases = [
  [
    "Example Transaction - Valid Envelope - Zero Gas Limit",
    buildEnvelopeTx(MAINNET, {gasLimit: 0}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Zero proposerKey.keyId",
    buildEnvelopeTx(MAINNET, {proposalKey: {keyId: 0}}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Zero proposalKey.sequenceNum",
    buildEnvelopeTx(MAINNET, {proposalKey: {sequenceNum: 0}}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Empty Authorizers",
    buildEnvelopeTx(MAINNET, {authorizers: []}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Maximum Authorizers",
    buildEnvelopeTx(MAINNET, {
      // Maximum authorizer count is 16
      authorizers: [
        // Lists are merged in buildEnvelopeTx, so first authorizer is already added
        "a3a627d47f628e9b",
        "b5f242e7e66684fa",
        "5195fb3af79c8425",

        "4254874588d9dcde",
        "a6333e989923dc01",
        "b0675bab0027d660",
        "5400e27611ddd6bf",

        "bb4d6932cca2a849",
        "5f2ad0efdd58a896",
        "497eb5dc445ca2f7",
        "ad190c0155a6a228",

        "1ff233dcee8f96d6",
        "fb958a01ff759609",
        "edc1ef3266719c68",
        "09a656ef778b9cb7",
      ],
    }),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Empty payloadSigs",
    buildEnvelopeTx(MAINNET, {payloadSigs: []}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Zero payloadSigs.0.key",
    buildEnvelopeTx(MAINNET, {payloadSigs: [{keyId: 0}]}),
    MAINNET,
  ],
  [
    "Example Transaction - Valid Envelope - Out-of-order payloadSigs -- By keyId",
    buildEnvelopeTx(MAINNET, {
      authorizers: [ADDRESS_MAINNET],
      payloadSigs: [
        {address: ADDRESS_MAINNET, keyId: 2, sig: "c"},
        {address: ADDRESS_MAINNET, keyId: 0, sig: "a"},
        {address: ADDRESS_MAINNET, keyId: 1, sig: "b"},
      ],
    }),
    MAINNET,
  ],
  ...validEnvelopeTransferCases,
  ...(ACCOUNT_KEYS.map((accountKey, i) => 
    [
      `Create Account Transaction - Valid Envelope - Single Account Key #${i}`,
      buildEnvelopeTx(MAINNET, {
        script: TX_CREATE_ACCOUNT,
        arguments: [
          {
            type: "Array",
            value: [
              {
                type: "String",
                value: accountKey,
              }
            ]
          }
        ]
      }),
      MAINNET,
    ]
  )),
  ...(range(1, 5).map((i) => 
    [
      `Create Account Transaction - Valid Envelope - Multiple Account Keys #${i}`,
      buildEnvelopeTx(MAINNET, {
        script: TX_CREATE_ACCOUNT,
        arguments: [
          {
            type: "Array",
            value: range(0, i).map((j) => (
              {
                type: "String",
                value: ACCOUNT_KEYS[j],
              }
            ))
          }
        ]
      }),
      MAINNET,
    ]
  )),
  ...(ACCOUNT_KEYS.map((accountKey, i) => 
    [
      `Add New Key Transaction - Valid Envelope - Valid Account Key ${i}`,
      buildEnvelopeTx(MAINNET, {
        script: TX_ADD_NEW_KEY,
        arguments: [
          {
            type: "String",
            value: accountKey,
          }
        ]
      }),
      MAINNET,
    ]
  )),
].map(createEnvelopeTestcase(true));


const args = process.argv.slice(2);
const outDir = args[0];

fs.writeFileSync(path.join(outDir, "validPayloadCases.json"), JSON.stringify(validPayloadCases, null, 2));
fs.writeFileSync(path.join(outDir, "invalidPayloadCases.json"), JSON.stringify(invalidPayloadCases, null, 2));
fs.writeFileSync(path.join(outDir, "validEnvelopeCases.json"), JSON.stringify(validEnvelopeCases, null, 2));
fs.writeFileSync(path.join(outDir, "invalidEnvelopeCases.json"), JSON.stringify(invalidEnvelopeCases, null, 2));

fs.writeFileSync(path.join(outDir, "manifestEnvelopeCases.json"), JSON.stringify(manifestEnvelopeCases, null, 2));
fs.writeFileSync(path.join(outDir, "manifestPayloadCases.json"), JSON.stringify(manifestPayloadCases, null, 2));
