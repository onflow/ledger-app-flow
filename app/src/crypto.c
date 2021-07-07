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

extern uint32_t hdPath[HDPATH_LEN_DEFAULT];


#include "cx.h"


zxerr_t crypto_extractPublicKey(const uint32_t path[HDPATH_LEN_DEFAULT], uint8_t *pubKey, uint16_t pubKeyLen) {
    zemu_log_stack("crypto_extractPublicKey");
    MEMZERO(pubKey, pubKeyLen);

    cx_curve_t curve = CX_CURVE_SECP256K1;

    const uint32_t domainSize = 32;
    const uint32_t pkSize = 1 + 2 * domainSize;
    if (pubKeyLen < pkSize) {
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
                                       path,
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
            MEMZERO(privateKeyData, domainSize);
        }
    }
    END_TRY;

    memcpy(pubKey, cx_publicKey.W, pkSize);
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

uint16_t digest_message(uint8_t *digest, uint16_t digestMax, const uint8_t *message, uint16_t messageLen) {
    zemu_log_stack("sha2_256");
    if (digestMax < CX_SHA256_SIZE) {
        zemu_log_stack("digest_message: zxerr_buffer_too_small");
        return zxerr_buffer_too_small;
    }
    sha256(message, messageLen, digest);
    return CX_SHA256_SIZE;
}

zxerr_t crypto_sign(uint8_t *buffer, uint16_t signatureMaxlen, const uint8_t *message, uint16_t messageLen, uint16_t *sigSize) {
    zemu_log_stack("crypto_sign");

    cx_curve_t curve = CX_CURVE_SECP256K1;

    const uint32_t domainSize = 32;
    uint8_t messageDigest[32];

    const enum cx_md_e cxhash_kind = CX_SHA256;
    const uint16_t messageDigestSize = digest_message(messageDigest, sizeof(messageDigest), message, messageLen );
    if (messageDigestSize != 32) {
        zemu_log_stack("crypto_sign: zxerr_out_of_bounds");
        return zxerr_out_of_bounds;
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

typedef struct {
    uint8_t publicKey[SECP256K1_PK_LEN];
    char addrStr[SECP256K1_PK_LEN*2];
    uint8_t padding[4];
} __attribute__((packed)) answer_t;

zxerr_t crypto_fillAddress(uint8_t *buffer, uint16_t buffer_len, uint16_t *addrLen) {
    MEMZERO(buffer, buffer_len);

    if (buffer_len < sizeof(answer_t)) {
        zemu_log_stack("crypto_fillAddress: zxerr_buffer_too_small");
        return zxerr_buffer_too_small;
    }

    answer_t *const answer = (answer_t *) buffer;

    zxerr_t err = crypto_extractPublicKey(hdPath, answer->publicKey, sizeof_field(answer_t, publicKey));
    if ( err != zxerr_ok ) {
        return err;
    }

    array_to_hexstr(answer->addrStr, sizeof_field(answer_t, addrStr) + 2, answer->publicKey, sizeof_field(answer_t, publicKey) );

    *addrLen = sizeof(answer_t) - sizeof_field(answer_t, padding);
    return zxerr_ok;
}

