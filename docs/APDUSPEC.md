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

| Field | Type     | Content                | Expected  |
| ----- | -------- | ---------------------- | --------- |
| CLA   | byte (1) | Application Identifier | 0x33      |
| INS   | byte (1) | Instruction ID         | 0x02      |
| P1    | byte (1) | Payload desc           | 0 = init  |
|       |          |                        | 1 = add   |
|       |          |                        | 2 = last  |
| P2    | byte (1) | ----                   | not used  |
| L     | byte (1) | Bytes in payload       | (depends) |

The first packet/chunk includes only the derivation path

All other packets/chunks contain data chunks that are described below

##### First Packet

| Field   | Type     | Content              | Expected |
| ------- | -------- | -------------------- | -------- |
| Path[0] | byte (4) | Derivation Path Data | 44'      |
| Path[1] | byte (4) | Derivation Path Data | 539'     |
| Path[2] | byte (4) | Derivation Path Data | ?        |
| Path[3] | byte (4) | Derivation Path Data | ?        |
| Path[4] | byte (4) | Derivation Path Data | ?        |
| Options | byte (2) | Crypto options (LE)  | ?        |

##### Other Chunks/Packets

| Field | Type     | Content | Expected |
| ----- | -------- | ------- | -------- |
| Data  | bytes... | Message |          |

Data is defined as:

| Field   | Type    | Content          | Expected |
| ------- | ------- | ---------------- | -------- |
| Message | bytes.. | RLP data to sign |          |

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
