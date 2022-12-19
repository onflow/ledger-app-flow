# Flow App

## General structure

The general structure of commands and responses is as follows:

### Commands

| Field   | Type     | Content                | Note |
| :------ | :------- | :--------------------- | ---- |
| CLA     | byte (1) | Application Identifier | 0x33 |
| INS     | byte (1) | Instruction ID         |      |
| P1      | byte (1) | Parameter 1            |      |
| P2      | byte (1) | Parameter 2            |      |
| L       | byte (1) | Bytes in payload       |      |
| PAYLOAD | byte (L) | Payload                |      |

### Response

| Field   | Type     | Content     | Note                     |
| ------- | -------- | ----------- | ------------------------ |
| ANSWER  | byte (?) | Answer      | depends on the command   |
| SW1-SW2 | byte (2) | Return code | see list of return codes |

### Return codes

| Return code | Description             |
| ----------- | ----------------------- |
| 0x6400      | Execution Error         |
| 0x6982      | Empty buffer            |
| 0x6983      | Output buffer too small |
| 0x6986      | Command not allowed     |
| 0x6D00      | INS not supported       |
| 0x6E00      | CLA not supported       |
| 0x6F00      | Unknown                 |
| 0x9000      | Success                 |

---

## Derivation Paths

Flow supports a range of signature schemes and hash algorithms.

In order to keep these public keys separated, the second items in the derivation path is used to indicate the signature scheme to use.

| Field   | Type     | Content              | Expected    |
| ------- | -------- | -------------------- | ----------- |
| Path[0] | byte (4) | Derivation Path Data | 44'         |
| Path[1] | byte (4) | Derivation Path Data | 539'        |
| Path[2] | byte (4) | Derivation Path Data | ?           |
| Path[3] | byte (4) | Derivation Path Data | ?           |
| Path[4] | byte (4) | Derivation Path Data | ?           |

Path hardening in item3 and item4 is optional. In case the device contains app version <= 0.9.12 two least significant bytes of Path[2] contain cryptooptions, but the codes of P-256 and secp256k1 curves are reversed.

## Crypto options

Crypto options are stored as 16-bit integer, more significant byte stores curve, less significant byte stores hash.

### Signatures

| Algorithm | Curve     | ID              | Code |
| --------- | --------- | --------------- | ---- |
| ECDSA     | P-256     | ECDSA_P256      | 2    |
| ECDSA     | secp256k1 | ECDSA_secp256k1 | 3    |

### Hashes

| Algorithm | Output Size | ID       | Code |
| --------- | ----------- | -------- | ---- |
| SHA-2     | 256         | SHA2_256 | 1    |
| SHA-3     | 256         | SHA3_256 | 3    |

---

## Account Slots

Account information can be temporarily stored in the device slots.
Each account slot contains an account identifier and the corresponding derivation path.
Only a 1:1 account/path relation can be stored per slot.

Each slot has the following structure

| Field   | Type    |                       |
| ------- | ------- | --------------------- |
| Account | byte(8) | Account Identifier    |
| Path    | u32 (5) | Derivation Path       |
| Options | byte(2) | Crypto options (LE)   |



---

## Command definition

### GET_VERSION

#### Command

| Field | Type     | Content                | Expected |
| ----- | -------- | ---------------------- | -------- |
| CLA   | byte (1) | Application Identifier | 0x33     |
| INS   | byte (1) | Instruction ID         | 0x00     |
| P1    | byte (1) | Parameter 1            | ignored  |
| P2    | byte (1) | Parameter 2            | ignored  |
| L     | byte (1) | Bytes in payload       | 0        |

#### Response

| Field   | Type     | Content          | Note                            |
| ------- | -------- | ---------------- | ------------------------------- |
| TEST    | byte (1) | Test Mode        | 0xFF means test mode is enabled |
| MAJOR   | byte (1) | Version Major    |                                 |
| MINOR   | byte (1) | Version Minor    |                                 |
| PATCH   | byte (1) | Version Patch    |                                 |
| LOCKED  | byte (1) | Device is locked |                                 |
| SW1-SW2 | byte (2) | Return code      | see list of return codes        |

---

### INS_GET_PUBKEY

#### Command

| Field   | Type     | Content                   | Expected           |
| ------- | -------- | ------------------------- | ------------------ |
| CLA     | byte (1) | Application Identifier    | 0x33               |
| INS     | byte (1) | Instruction ID            | 0x01               |
| P1      | byte (1) | Request User confirmation | No = 0             |
| P2      | byte (1) | Parameter 2               | ignored            |
| L       | byte (1) | Bytes in payload          | (depends)          |
| Path[0] | byte (4) | Derivation Path Data      | 0x80000000 \| 44   |
| Path[1] | byte (4) | Derivation Path Data      | 0x80000000 \| 539' |
| Path[2] | byte (4) | Derivation Path Data      | ?                  |
| Path[3] | byte (4) | Derivation Path Data      | ?                  |
| Path[4] | byte (4) | Derivation Path Data      | ?                  |
| Options | byte (2) | CryptoOptions (LE)        | ?                  |

#### Response

| Field      | Type      | Content           | Note                     |
| ---------- | --------- | ----------------- | ------------------------ |
| PK         | byte (65) | Public Key        |                          |
| ADDR_B_LEN | byte (1)  | ADDR_B Length     |                          |
| ADDR_B     | byte (??) | Address as Bytes  |                          |
| ADDR_S_LEN | byte (1)  | ADDR_S Len        |                          |
| ADDR_S     | byte (??) | Address as String |                          |
| SW1-SW2    | byte (2)  | Return code       | see list of return codes |

### INS_SIGN

#### Command

| Field | Type     | Content                | Expected           |
| ----- | -------- | ---------------------- | ------------------ |
| CLA   | byte (1) | Application Identifier | 0x33               |
| INS   | byte (1) | Instruction ID         | 0x02               |
| P1    | byte (1) | Payload desc           | 0 = init           |
|       |          |                        | 1 = add            |
|       |          |                        | 2 = final          |
|       |          |                        | 3 = metadata       |
|       |          |                        | 4 = MT proof       |
|       |          |                        | 5 = MT proof final |
|       |          |                        | 10 = message final |
| P2    | byte (1) | ----                   | (depends)          |
| L     | byte (1) | Bytes in payload       | (depends)          |

The first packet/chunk includes only the derivation path

All other packets/chunks contain data chunks that are described below. There are following workflows as of now (typical sequences here, the app allows other combination of commands, too):

Merkle tree workflow - Init packet, several add packets, metadata packet, four Merkle tree packets (3x 0x04 and finaly 0x05).
Arbitrary transaction signing - Init packet, several add packets, final packet.
NFT workflow - Init packet, several add packets, final packet.
Message signing workflow - Init packet, several add packets, final message packet (P1=0x10).

##### Init Packet P1 = 0x00

| Field   | Type     | Content              | Expected |
| ------- | -------- | -------------------- | -------- |
| P2      | byte (1) |                      | not used |
| ------- | -------- | -------------------- | -------- |
| Path[0] | byte (4) | Derivation Path Data | 44'      |
| Path[1] | byte (4) | Derivation Path Data | 539'     |
| Path[2] | byte (4) | Derivation Path Data | ?        |
| Path[3] | byte (4) | Derivation Path Data | ?        |
| Path[4] | byte (4) | Derivation Path Data | ?        |
| Options | byte (2) | Crypto options (LE)  | ?        |

This clears data and sets detivation path and crypto options variable

##### Add Packet P1 = 0x01

| Field | Type     | Content    | Expected |
| ----- | -------- | ---------- | -------- |
| P2    | byte (1) |            | not used |
| ----- | -------- | ---------- | -------- |
| Data  | bytes... | see bellow |          |

Data is defined as:

| Field   | Type    | Content                  | Expected |
| ------- | ------- | ------------------------ | -------- |
| Message | bytes.. | RLP data/message to sign |          |

Appends to data (transaction or message)

##### Final Packet P1 = 0x02

| Field | Type     | Content    | Expected   |
| ----- | -------- | ---------- | ---------- |
| P2    | byte (1) | workflow   | 1, 2, or 3 |
| ----- | -------- | ---------- | ---------- |
| Data  | bytes... | see bellow |            |

Workflow is defined as: 1 - Arbitrary message signing, 2 - Setup NFT collection, 3 - Transfer NFT
Arbitrary message signing requires expert mode and is able to handle any transaction. Setup NFT collection and Transfer NFT require the transaction to contain script (and arguments) conforming to NFT script templates (see script_parser.c).

Data is defined as:

| Field   | Type    | Content          | Expected |
| ------- | ------- | ---------------- | -------- |
| Message | bytes.. | RLP data to sign |          |

Appends to transaction data and initiates transaction signing without metadata (requires expert mode).

##### Metadata Packet P1 = 0x03

| Field | Type     | Content  | Expected |
| ----- | -------- | -------- | -------- |
| P2    | byte (1) |          | not used |
| ----- | -------- | -------- | -------- |
| Data  | bytes... | Metadata |          |

Metadata is defined as:

| Field          | Type              | Content          | Expected |
| -------------- | ----------------- | ---------------- | -------- |
| Num. of hashes | byte (1)          | number of hashes |          |
| Script hash 1  | byte (32)         | script hash      |          |
| Script hash 2  | byte (32)         | script hash      |          |
| ...            |                   |                  |          |
| Script hash n  | byte (32)         | script hash      |          |
| Tx name        | null term. string | name of tx       |          |
| Num. of args   | byte (1)          | num. of tx args  |          |
| Argument 1     | bytes             | argument 1       |          |
| Argument 2     | bytes             | argument 2       |          |
| ...            |                   |                  |          |
| Argument m     | bytes             | argument m       |          |

and argument is either normal argument

| Field          | Type              | Content                       | Expected |
| -------------- | ----------------- | ----------------------------- | -------- |
| Argument type  | byte (1)          | 1 - normal                    |          |
|                |                   | 2 - optional                  |          |
| Arg. name      | null term. string |                               |          |
| Arg. index     | byte (1)          | Order in which args are shown |          |
| Value type     | null term. string | Expected JSON value type      |          |
| JSON type      | byte (1)          |                               | 3-string |

or array argument

| Field          | Type              | Content                       | Expected |
| -------------- | ----------------- | ----------------------------- | -------- |
| Argument type  | byte (1)          | 3 - normal array              |          |
|                |                   | 4 - optional array            |          |
| Arr. min. len. | byte (1)          | Array min. length             |          |
| Arr. min. len. | byte (1)          | Array max. length             |          |
| Arg. name      | null term. string |                               |          |
| Arg. index     | byte (1)          | Order in which args are shown |          |
| Value type     | null term. string | Expected JSON value type      |          |
| JSON type      | byte (1)          |                               | 3-string |

Loads metadata, clears merkle tree counter.

##### Merkle tree Packet P1 = 0x04 and 0x05

Four APDUs for four levels of internal merkle tree nodes. Each internal nerkle tree node has 7 children as 7 hashes fit into one APDU. APDU with P1=0x03 calculates metadata hash which corresponds to Merkle tree leaf value. Three subsequent P1=0x04 calls have to contain hashes from previous calls (either P1=0x03 or P1=0x04). After three calls there is call with P1=0x05, which works the same as P1=0x04 call, but it initiates transaction signing.

| Field               | Type         | Content          | Expected |
| ------------------- | ------------ | ---------------- | -------- |
| P2                  | byte (1)     |                  | not used |
| ------------------- | ------------ | ---------------- | -------- |
| Merkle tree hash 1  | byte (32)    | Merkle tree hash |          |
| Merkle tree hash 2  | byte (32)    | Merkle tree hash |          |
| ...                 |              |                  |          |
| Merkle tree hash 7  | byte (32)    | Merkle tree hash |          |

Validates merkle tree node. Validates that previous hash (metadata hash or merkle tree node hash) is in the list of hashes. Computes new hash and increments merkle tree counter. Call with P1 = 0x05 starts the signing process with metadata. This requires that we are at the root of the merkle tree and that the hash value matches the one stored in the app.

##### Final message signing Packet P1 = 0x10

| Field | Type     | Content | Expected |
| ----- | -------- | ------- | -------- |
| P2    | byte (1) |         | not used |
| ----- | -------- | ------- | -------- |
| Data  | bytes... | Message |          |

Data is defined as:

| Field   | Type    | Content             | Expected |
| ------- | ------- | ------------------- | -------- |
| Message | bytes.. | Mesage data to sign |          |

Appends to data to message and initiates message signing.

#### Response

| Field       | Type           | Content     | Note                     |
| ----------- | -------------- | ----------- | ------------------------ |
| secp256k1 R | byte (32)      | Signature   |                          |
| secp256k1 S | byte (32)      | Signature   |                          |
| secp256k1 V | byte (1)       | Signature   |                          |
| SIG         | byte (varible) | Signature   | DER format               |
| SW1-SW2     | byte (2)       | Return code | see list of return codes |

---

### INS_GET_SLOTS_STATUS

#### Command

| Field | Type     | Content                   | Expected |
| ----- | -------- | ------------------------- | -------- |
| CLA   | byte (1) | Application Identifier    | 0x33     |
| INS   | byte (1) | Instruction ID            | 0x11     |
| P1    | byte (1) | Request User confirmation | No = 0   |
| P2    | byte (1) | Parameter 2               | ignored  |
| L     | byte (1) | Bytes in payload          | 0        |

#### Response

| Field | Type      | Content      | Note            |
| ----- | --------- | ------------ | --------------- |
| USED  | byte (64) | Slot is used | No = 0, Yes = 1 |

### INS_GET_SLOT

#### Command

| Field | Type     | Content                   | Expected |
| ----- | -------- | ------------------------- | -------- |
| CLA   | byte (1) | Application Identifier    | 0x33     |
| INS   | byte (1) | Instruction ID            | 0x11     |
| P1    | byte (1) | Request User confirmation | No = 0   |
| P2    | byte (1) | Parameter 2               | ignored  |
| L     | byte (1) | Bytes in payload          | 1        |
| Slot  | byte (1) | Slot Index                | 0..63    |

#### Response

| Field   | Type     | Content              | Note               |
| ------- | -------- | -------------------- | ------------------ |
| ADDR    | byte (8) | Address              |                    |
| Path[0] | byte (4) | Derivation Path Data | 0x80000000 \| 44   |
| Path[1] | byte (4) | Derivation Path Data | 0x80000000 \| 539' |
| Path[2] | byte (4) | Derivation Path Data | ?                  |
| Path[3] | byte (4) | Derivation Path Data | ?                  |
| Path[4] | byte (4) | Derivation Path Data | ?                  |
| Options | byte (2) | Crypto options (LE)  | ?                  |

Note:
Setting the slot to all zeros, will remove the data, otherwise,
the slot needs to have a valid derivation path

### INS_SET_SLOT

#### Command

| Field   | Type     | Content                | Expected |
| ------- | -------- | ---------------------- | -------- |
| CLA     | byte (1) | Application Identifier | 0x33     |
| INS     | byte (1) | Instruction ID         | 0x12     |
| P1      | byte (1) | Parameter 1            | ignored  |
| P2      | byte (1) | Parameter 2            | ignored  |
| L       | byte (1) | Bytes in payload       | 29       |
| Slot    | byte (1) | Slot Index             | 0..63    |
| ADDR    | byte (8) | Address                |          |
| Path[0] | byte (4) | Derivation Path Data   | ?        |
| Path[1] | byte (4) | Derivation Path Data   | ?        |
| Path[2] | byte (4) | Derivation Path Data   | ?        |
| Path[3] | byte (4) | Derivation Path Data   | ?        |
| Path[4] | byte (4) | Derivation Path Data   | ?        |
| Options | byte (2) | Crypto options (LE)    | ?        |
