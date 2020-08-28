/*******************************************************************************
*   (c) 2019 Zondax GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "crypto.h"
#include "coin.h"
#include "zxmacros.h"

uint32_t hdPath[HDPATH_LEN_DEFAULT];

bool isTestnet() {
    return hdPath[0] == HDPATH_0_TESTNET &&
           hdPath[1] == HDPATH_1_TESTNET;
}

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#include "cx.h"

__Z_INLINE digest_type_e get_hash_type() {
    const uint8_t hash_type = (uint8_t) (hdPath[2] & 0xFF);
    switch(hash_type) {
        case 0x01:
            zemu_log_stack("path: sha2_256");
            return sha2_256;
        case 0x03:
            zemu_log_stack("path: sha3_256");
            return sha3_256;
        default:
            zemu_log_stack("path: unknown");
            return hash_unknown;
    }
}

__Z_INLINE cx_curve_t get_cx_curve() {
    const uint8_t curve_code = (uint8_t) ((hdPath[2] >> 8) & 0xFF);
    switch(curve_code) {
        case 0x02: {
            zemu_log_stack("curve: secp256k1");
            return CX_CURVE_SECP256K1;
        }
        case 0x03: {
            zemu_log_stack("curve: secp256r1");
            return CX_CURVE_SECP256R1;
        }
        default:
            return CX_CURVE_NONE;
    }
}

__Z_INLINE enum cx_md_e get_cx_hash_kind() {
    switch(get_hash_type()) {
        case sha2_256: {
            return CX_SHA256;
        }
        case sha3_256: {
            return CX_SHA3;
        }
        default:
            return CX_NONE;
    }
}

uint8_t crypto_extractPublicKey(const uint32_t path[HDPATH_LEN_DEFAULT], uint8_t *pubKey, uint16_t pubKeyLen) {
    MEMZERO(pubKey, pubKeyLen);
    cx_curve_t curve = get_cx_curve();
    if (curve==CX_CURVE_NONE) {
        return 0;
    }

    const uint32_t domainSize = 32;
    const uint32_t pkSize = 1 + 2 * domainSize;
    if (pubKeyLen < pkSize) {
        return 0;
    }

    cx_ecfp_public_key_t cx_publicKey;
    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[32];

    BEGIN_TRY
    {
        TRY {
            os_perso_derive_node_bip32(curve,
                                       path,
                                       HDPATH_LEN_DEFAULT,
                                       privateKeyData, NULL);

            cx_ecfp_init_private_key(curve, privateKeyData, 32, &cx_privateKey);
            cx_ecfp_init_public_key(curve, NULL, 0, &cx_publicKey);
            cx_ecfp_generate_pair(curve, &cx_publicKey, &cx_privateKey, 1);
        }
        FINALLY {
            MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
            MEMZERO(privateKeyData, domainSize);
        }
    }
    END_TRY;

    memcpy(pubKey, cx_publicKey.W, pkSize);
    return pkSize;
}

typedef struct {
    uint8_t r[32];
    uint8_t s[32];
    uint8_t v;

    // DER signature max size should be 73
    // https://bitcoin.stackexchange.com/questions/77191/what-is-the-maximum-size-of-a-der-encoded-ecdsa-signature#77192
    uint8_t der_signature[73];
} __attribute__((packed)) signature_t;

void sha256(const uint8_t *message, uint16_t messageLen, uint8_t message_digest[CX_SHA256_SIZE]) {
    cx_hash_sha256(message, messageLen, message_digest, CX_SHA256_SIZE);
}

uint16_t digest_message(uint8_t *digest, uint16_t digestMax, const uint8_t *message, uint16_t messageLen) {
    switch(get_hash_type()) {
        case sha2_256: {
            zemu_log_stack("sha2_256");
            if (digestMax < CX_SHA256_SIZE) {
                return 0;
            }
            sha256(message, messageLen, digest);
            return CX_SHA256_SIZE;
        }
        case sha3_256: {
            if (digestMax < 32) {
                return 0;
            }
            zemu_log_stack("sha3_256");
            cx_sha3_t sha3;
            cx_sha3_init(&sha3, 256);
            cx_hash((cx_hash_t*)&sha3, CX_LAST, message, messageLen, digest, 32);
            zemu_log_stack("sha3_256 ready");
            return 32;
        }
        default:
            return 0;
    }
}

uint16_t crypto_sign(uint8_t *buffer, uint16_t signatureMaxlen, const uint8_t *message, uint16_t messageLen) {
    cx_curve_t curve = get_cx_curve();
    if (curve==CX_CURVE_NONE) {
        return 0;
    }

    const uint32_t domainSize = 32;
    uint8_t messageDigest[128];

    const enum cx_md_e cxhash_kind = get_cx_hash_kind();
    const uint16_t messageDigestSize = digest_message(messageDigest, sizeof(messageDigest), message, messageLen );
    if (cxhash_kind == CX_NONE || messageDigestSize == 0) {
        return 0;
    }

    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[32];
    int signatureLength;
    unsigned int info = 0;

    signature_t *const signature = (signature_t *) buffer;

    BEGIN_TRY
    {
        TRY
        {
            // Generate keys
            zemu_log_stack("derive path");
            os_perso_derive_node_bip32(curve,
                                       hdPath,
                                       HDPATH_LEN_DEFAULT,
                                       privateKeyData, NULL);

            cx_ecfp_init_private_key(curve, privateKeyData, 32, &cx_privateKey);

            // Sign
            zemu_log_stack("cx_ecdsa_sign");
            signatureLength = cx_ecdsa_sign(&cx_privateKey,
                                            CX_RND_RFC6979 | CX_LAST,
                                            cxhash_kind,
                                            messageDigest,
                                            messageDigestSize,
                                            signature->der_signature,
                                            sizeof_field(signature_t, der_signature),
                                            &info);
            zemu_log_stack("signed");
        }
        FINALLY {
            MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
            MEMZERO(privateKeyData, domainSize);
        }
    }
    END_TRY;

    err_convert_e err = convertDERtoRSV(signature->der_signature, info,  signature->r, signature->s, &signature->v);
    if (err != no_error) {
        // Error while converting so return length 0
        return 0;
    }

    // return actual size using value from signatureLength
    return sizeof_field(signature_t, r) + sizeof_field(signature_t, s) + sizeof_field(signature_t, v) + signatureLength;
}

typedef struct {
    uint8_t publicKey[SECP256K1_PK_LEN];
    char addrStr[SECP256K1_PK_LEN*2];
    uint8_t padding[4];
} __attribute__((packed)) answer_t;

uint16_t crypto_fillAddress(uint8_t *buffer, uint16_t buffer_len) {
    MEMZERO(buffer, buffer_len);

    if (buffer_len < sizeof(answer_t)) {
        return 0;
    }

    answer_t *const answer = (answer_t *) buffer;

    if (crypto_extractPublicKey(hdPath, answer->publicKey, sizeof_field(answer_t, publicKey)) == 0 ) {
        return 0;
    }

    array_to_hexstr(answer->addrStr, sizeof_field(answer_t, addrStr) + 2, answer->publicKey, sizeof_field(answer_t, publicKey) );

    return sizeof(answer_t) - sizeof_field(answer_t, padding);
}

#endif
