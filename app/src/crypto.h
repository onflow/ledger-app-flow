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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <zxmacros.h>
#include "coin.h"
#include <stdbool.h>
#include <sigutils.h>
#include <zxerror.h>

typedef enum {
    HASH_UNKNOWN,
    HASH_SHA2_256,
    HASH_SHA3_256
} digest_type_e;

typedef enum {
    CURVE_UNKNOWN,
    CURVE_SECP256K1,
    CURVE_SECP256R1
} curve_e;

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#else
#define CX_SHA256_SIZE 32
#endif

void sha256(const uint8_t *message, uint16_t messageLen, uint8_t message_digest[CX_SHA256_SIZE]);

zxerr_t crypto_extractPublicKey(const hd_path_t path, const uint16_t options, uint8_t *pubKey, uint16_t pubKeyLen);

zxerr_t crypto_sign(
    const hd_path_t path,
    const uint16_t options,
    const uint8_t *message,
    uint16_t messageLen,
    uint8_t *signature,
    uint16_t signatureMaxlen,
    uint16_t *sigSize
);

#ifdef __cplusplus
}
#endif
