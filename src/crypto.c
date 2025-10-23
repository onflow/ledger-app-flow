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
#include "addr.h"
#include "zxmacros.h"
#include "zxformat.h"
#include "zxerror.h"

#if defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2) || \
    defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX_P)
#include "lib_standard_app/crypto_helpers.h"
#include "cx.h"

__Z_INLINE digest_type_e get_hash_type(const uint16_t options) {
    const uint8_t hash_type = (uint8_t)(options & 0xFF);
    switch (hash_type) {
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

__Z_INLINE cx_curve_t get_cx_curve(const uint16_t options) {
    const uint8_t curve_code = (uint8_t)((options >> 8) & 0xFF);
    switch (curve_code) {
        case 0x02: {
            zemu_log_stack("curve: secp256r1");
            return CX_CURVE_SECP256R1;
        }
        case 0x03: {
            zemu_log_stack("curve: secp256k1");
            return CX_CURVE_SECP256K1;
        }
        default:
            return CX_CURVE_NONE;
    }
}

zxerr_t crypto_extractPublicKey(const hd_path_t path,
                                const uint16_t options,
                                uint8_t *pubKey,
                                uint16_t pubKeyLen) {
    zemu_log_stack("crypto_extractPublicKey");
    MEMZERO(pubKey, pubKeyLen);

    cx_curve_t curve = get_cx_curve(options);
    if (curve != CX_CURVE_SECP256K1 && curve != CX_CURVE_SECP256R1) {
        zemu_log_stack("extractPublicKey: invalid_crypto_settings");
        return zxerr_invalid_crypto_settings;
    }

    if (pubKeyLen < 65) {
        zemu_log_stack("extractPublicKey: zxerr_buffer_too_small");
        return zxerr_buffer_too_small;
    }

    cx_err_t cx_err = zxerr_unknown;

    zemu_log_stack("extractPublicKey: derive_node_bip32");
    cx_err =
        bip32_derive_get_pubkey_256(curve, path.data, HDPATH_LEN_DEFAULT, pubKey, NULL, CX_SHA256);
    if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;

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

zxerr_t sha256(const uint8_t *message,
               uint16_t messageLen,
               uint8_t message_digest[CX_SHA256_SIZE]) {
    size_t digest_len = cx_hash_sha256(message, messageLen, message_digest, CX_SHA256_SIZE);
    if (digest_len != CX_SHA256_SIZE) {
        return zxerr_invalid_crypto_settings;
    }
    return zxerr_ok;
}

zxerr_t digest_message(const uint8_t *message,
                       uint16_t messageLen,
                       const uint8_t domainTag[DOMAIN_TAG_LENGTH],
                       digest_type_e hash_kind,
                       uint8_t *digest,
                       uint16_t digestMax,
                       uint16_t *digest_size) {
    cx_err_t cx_err = zxerr_unknown;

    switch (hash_kind) {
        case HASH_SHA2_256: {
            zemu_log_stack("sha2_256");
            if (digestMax < CX_SHA256_SIZE) {
                zemu_log_stack("digest_message: zxerr_buffer_too_small");
                return zxerr_buffer_too_small;
            }
            cx_sha256_t sha2;
            cx_err = cx_sha256_init_no_throw(&sha2);
            if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;
            cx_err =
                cx_hash_no_throw((cx_hash_t *) &sha2, 0, domainTag, DOMAIN_TAG_LENGTH, NULL, 0);
            if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;
            cx_err = cx_hash_no_throw((cx_hash_t *) &sha2,
                                      CX_LAST,
                                      message,
                                      messageLen,
                                      digest,
                                      CX_SHA256_SIZE);
            if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;
            *digest_size = cx_hash_get_size((cx_hash_t *) &sha2);
            ;
            return zxerr_ok;
        }
        case HASH_SHA3_256: {
            if (digestMax < CX_SHA3_256_SIZE) {
                return zxerr_buffer_too_small;
            }
            zemu_log_stack("sha3_256");
            cx_sha3_t sha3;
            cx_err = cx_sha3_init_no_throw(&sha3, 256);
            if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;
            cx_err =
                cx_hash_no_throw((cx_hash_t *) &sha3, 0, domainTag, DOMAIN_TAG_LENGTH, NULL, 0);
            if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;
            cx_err =
                cx_hash_no_throw((cx_hash_t *) &sha3, CX_LAST, message, messageLen, digest, 32);
            if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;
            *digest_size = cx_hash_get_size((cx_hash_t *) &sha3);
            return zxerr_ok;
        }
        default: {
            zemu_log_stack("digest_message: zxerr_invalid_crypto_settings");
            return zxerr_invalid_crypto_settings;
        }
    }
}

zxerr_t crypto_sign(const hd_path_t path,
                    const uint16_t options,
                    const uint8_t *message,
                    uint16_t messageLen,
                    const uint8_t domainTag[DOMAIN_TAG_LENGTH],
                    uint8_t *buffer,
                    uint16_t bufferSize,
                    uint16_t *sigSize) {
    cx_err_t cx_err = zxerr_unknown;
    zemu_log_stack("crypto_sign");

    cx_curve_t curve = get_cx_curve(options);
    if (curve != CX_CURVE_SECP256K1 && curve != CX_CURVE_SECP256R1) {
        zemu_log_stack("crypto_sign: invalid_crypto_settings");
        return zxerr_invalid_crypto_settings;
    }

    const digest_type_e cx_hash_kind = get_hash_type(options);

    uint8_t messageDigest[32];
    uint16_t messageDigestSize = 0;

    CHECK_ZXERR(digest_message(message,
                               messageLen,
                               domainTag,
                               cx_hash_kind,
                               messageDigest,
                               sizeof(messageDigest),
                               &messageDigestSize));

    if (cx_hash_kind != HASH_SHA2_256 && cx_hash_kind != HASH_SHA3_256) {
        zemu_log_stack("crypto_sign: zxerr_out_of_bounds");
        return zxerr_out_of_bounds;
    }

    if (cx_hash_kind == HASH_SHA2_256 && messageDigestSize != CX_SHA256_SIZE) {
        zemu_log_stack("crypto_sign: zxerr_out_of_bounds");
        return zxerr_out_of_bounds;
    }

    if (cx_hash_kind == HASH_SHA3_256 && messageDigestSize != CX_SHA3_256_SIZE) {
        zemu_log_stack("crypto_sign: zxerr_out_of_bounds");
        return zxerr_out_of_bounds;
    }

    size_t signatureLength;
    uint32_t info = 0;

    if (bufferSize < sizeof(signature_t)) {
        zemu_log_stack("crypto_sign: zxerr_buffer_too_small");
        return zxerr_buffer_too_small;
    }
    signature_t *const signature = (signature_t *) buffer;

    signatureLength = sizeof_field(signature_t, der_signature);
    zemu_log_stack("cx_ecdsa_sign");
    cx_err = bip32_derive_ecdsa_sign_hash_256(curve,
                                              path.data,
                                              HDPATH_LEN_DEFAULT,
                                              CX_RND_RFC6979 | CX_LAST,
                                              CX_SHA256,
                                              messageDigest,
                                              messageDigestSize,
                                              signature->der_signature,
                                              &signatureLength,
                                              &info);
    if (cx_err != CX_OK) return zxerr_invalid_crypto_settings;
    zemu_log_stack("signed");

    err_convert_e err =
        convertDERtoRSV(signature->der_signature, info, signature->r, signature->s, &signature->v);
    if (err != no_error) {
        // Error while converting so return length 0
        return zxerr_invalid_crypto_settings;
    }

    // return actual size using value from signatureLength
    *sigSize = sizeof_field(signature_t, r) + sizeof_field(signature_t, s) +
               sizeof_field(signature_t, v) + signatureLength;
    return zxerr_ok;
}

#endif
