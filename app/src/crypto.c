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

flow_path_t hdPath;

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#include "cx.h"

#define CHECK_ZXERR(__EXPR) { \
    zxerr_t __err = __EXPR;  \
    CHECK_APP_CANARY();  \
    if (__err != zxerr_ok) return __err; \
}

__Z_INLINE digest_type_e get_hash_type(const uint32_t path[HDPATH_LEN_DEFAULT]) {
    _Static_assert(HDPATH_LEN_DEFAULT >= 3, "Invalid HDPATH_LEN_DEFAULT");
    const uint8_t hash_type = (uint8_t) (path[2] & 0xFF);
    switch(hash_type) {
        case 0x01:
            zemu_log_stack("path: sha2_256");
            return HASH_SHA2_256;
        case 0x03:
            zemu_log_stack("path: sha3_256");
            return HASH_SHA3_256;
        default:
            zemu_log_stack("path: unknown");
            return HASH_UNKNOWN;
    }
}

__Z_INLINE cx_curve_t get_cx_curve(const uint32_t path[HDPATH_LEN_DEFAULT]) {
    _Static_assert(HDPATH_LEN_DEFAULT >= 3, "Invalid HDPATH_LEN_DEFAULT");
    const uint8_t curve_code = (uint8_t) ((path[2] >> 8) & 0xFF);
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

zxerr_t crypto_extractPublicKey(const flow_path_t *path, uint8_t *pubKey, uint16_t pubKeyLen) {
    zemu_log_stack("crypto_extractPublicKey");
    MEMZERO(pubKey, pubKeyLen);

    cx_curve_t curve = get_cx_curve(path->data);
    if (curve!=CX_CURVE_SECP256K1 && curve!=CX_CURVE_SECP256R1 ) {
        zemu_log_stack("extractPublicKey: invalid_crypto_settings");
        return zxerr_invalid_crypto_settings;
    }

    if (pubKeyLen < PUBLIC_KEY_LEN) {
        zemu_log_stack("extractPublicKey: zxerr_buffer_too_small");
        return zxerr_buffer_too_small;
    }


    cx_ecfp_public_key_t cx_publicKey;
    cx_ecfp_private_key_t cx_privateKey;
    uint8_t privateKeyData[32];

    BEGIN_TRY
    {
        TRY {
            zemu_log_stack("extractPublicKey: derive_node_bip32");
            os_perso_derive_node_bip32(curve,
                                       path->data,
                                       HDPATH_LEN_DEFAULT,
                                       privateKeyData, NULL);

            zemu_log_stack("extractPublicKey: cx_ecfp_init_private_key");
            cx_ecfp_init_private_key(curve, privateKeyData, 32, &cx_privateKey);
            cx_ecfp_init_public_key(curve, NULL, 0, &cx_publicKey);

            zemu_log_stack("extractPublicKey: cx_ecfp_generate_pair");
            cx_ecfp_generate_pair(curve, &cx_publicKey, &cx_privateKey, 1);
        }
        FINALLY {
            MEMZERO(&cx_privateKey, sizeof(cx_privateKey));
            MEMZERO(privateKeyData, sizeof(privateKeyData));
        }
    }
    END_TRY;

    memcpy(pubKey, cx_publicKey.W, PUBLIC_KEY_LEN);
    return zxerr_ok;
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

zxerr_t digest_message(const uint8_t *message, uint16_t messageLen, digest_type_e hash_kind, uint8_t *digest, uint16_t digestMax,  uint16_t* digest_size) {
    switch(hash_kind) {
        case HASH_SHA2_256: {
            zemu_log_stack("sha2_256");
            if (digestMax < CX_SHA256_SIZE) {
                zemu_log_stack("digest_message: zxerr_buffer_too_small");
                return zxerr_buffer_too_small;
            }
            sha256(message, messageLen, digest);
            *digest_size = CX_SHA256_SIZE;
            return zxerr_ok;
        }
        case HASH_SHA3_256: {
            if (digestMax < 32) {
                return zxerr_buffer_too_small;
            }
            zemu_log_stack("sha3_256");
            cx_sha3_t sha3;
            cx_sha3_init(&sha3, 256);
            cx_hash((cx_hash_t*)&sha3, CX_LAST, message, messageLen, digest, 32);
            zemu_log_stack("sha3_256 ready");
            *digest_size = 32;
            return zxerr_ok;
        }
        default: {
            zemu_log_stack("digest_message: zxerr_invalid_crypto_settings");
            return zxerr_invalid_crypto_settings;
        }
    }
}

zxerr_t crypto_sign(const uint32_t path[HDPATH_LEN_DEFAULT], const uint8_t *message, uint16_t messageLen, uint8_t *buffer, uint16_t signatureMaxlen,  uint16_t *sigSize) {
    zemu_log_stack("crypto_sign");

    cx_curve_t curve = get_cx_curve(path);
    if (curve!=CX_CURVE_SECP256K1 && curve!=CX_CURVE_SECP256R1 ) {
        zemu_log_stack("crypto_sign: invalid_crypto_settings");
        return zxerr_invalid_crypto_settings;
    }

    const enum cx_md_e cx_hash_kind = get_hash_type(path);
    
    uint8_t messageDigest[32];
    uint16_t messageDigestSize = 0;

    CHECK_ZXERR(digest_message(message, messageLen, cx_hash_kind, messageDigest, sizeof(messageDigest), &messageDigestSize));
    
    if (messageDigestSize != 32) {
        zemu_log_stack("crypto_sign: zxerr_out_of_bounds");
        return zxerr_out_of_bounds;
    }

    cx_ecfp_private_key_t cx_privateKey;
    const uint32_t domainSize = 32;
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
                                       path,
                                       HDPATH_LEN_DEFAULT,
                                       privateKeyData, NULL);

            cx_ecfp_init_private_key(curve, privateKeyData, 32, &cx_privateKey);

            // Sign
            zemu_log_stack("cx_ecdsa_sign");
            signatureLength = cx_ecdsa_sign(&cx_privateKey,
                                            CX_RND_RFC6979 | CX_LAST,
                                            CX_SHA256,
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
        return zxerr_invalid_crypto_settings;
    }

    // return actual size using value from signatureLength
    *sigSize = sizeof_field(signature_t, r) + sizeof_field(signature_t, s) + sizeof_field(signature_t, v) + signatureLength;
    return zxerr_ok;
}

#endif
