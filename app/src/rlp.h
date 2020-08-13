/*******************************************************************************
*   (c) 2020 Zondax GmbH
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

#include <zxmacros.h>
#include "uint256.h"
#include "parser_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kind_unknown = 0,
    kind_byte,
    kind_string,
    kind_list,
} rlp_kind_e;

typedef struct {
    rlp_kind_e kind;
    uint16_t fieldOffset;
    uint16_t valueOffset;
    uint16_t valueLen;
} rlp_field_t;

parser_error_t rlp_decode(const parser_context_t *input,
                          parser_context_t *outputPayload,
                          rlp_kind_e *outputKind,
                          uint32_t *bytesConsumed);

// reads a byte from the field
parser_error_t rlp_readByte(const parser_context_t *ctx, rlp_kind_e kind, uint8_t *value);

// reads a variable uint256
parser_error_t rlp_readUInt256(const parser_context_t *ctx, rlp_kind_e kind, uint256_t *value);

#ifdef __cplusplus
}
#endif
