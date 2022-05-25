/*******************************************************************************
*  (c) 2019-2020 Zondax GmbH
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

#include <stdint.h>
#include <stddef.h>

#define CHECK_PARSER_ERR(__CALL) { \
    parser_error_t __err = __CALL;  \
    CHECK_APP_CANARY()  \
    if (__err != PARSER_OK) return __err;}

#define CTX_CHECK_AND_ADVANCE(CTX, SIZE) \
    CTX_CHECK_AVAIL((CTX), (SIZE))   \
    (CTX)->offset += (SIZE);

#define CTX_CHECK_AVAIL(CTX, SIZE) \
    if ( (CTX) == NULL || ((CTX)->offset + SIZE) > (CTX)->bufferLen) { return PARSER_UNEXPECTED_BUFFER_END; }

typedef enum {
    // Generic errors
    PARSER_OK = 0,
    PARSER_NO_DATA,
    PARSER_INIT_CONTEXT_EMPTY,
    PARSER_DISPLAY_IDX_OUT_OF_RANGE,
    PARSER_DISPLAY_PAGE_OUT_OF_RANGE,
    PARSER_UNEXPECTED_ERROR,
    // Coin specific
    PARSER_RLP_ERROR_INVALID_KIND,
    PARSER_RLP_ERROR_INVALID_VALUE_LEN,
    PARSER_RLP_ERROR_INVALID_FIELD_OFFSET,
    PARSER_RLP_ERROR_BUFFER_TOO_SMALL,
    PARSER_RLP_ERROR_INVALID_PAGE,
    PARSER_JSON_INVALID,
    PARSER_JSON_INVALID_TOKEN_IDX,
    PARSER_JSON_TOO_MANY_TOKENS,
    PARSER_JSON_INCOMPLETE_JSON,
    PARSER_JSON_UNEXPECTED_ERROR,
    PARSER_JSON_ZERO_TOKENS,
    ///
    PARSER_UNEXPECTED_TX_VERSION,
    PARSER_UNEXPECTED_TYPE,
    PARSER_UNEXPECTED_SCRIPT,
    PARSER_UNEXPECTED_METHOD,
    PARSER_UNEXPECTED_BUFFER_END,
    PARSER_UNEXPECTED_VALUE,
    PARSER_UNEXPECTED_NUMBER_ITEMS,
    PARSER_UNEXPECTED_CHARACTERS,
    PARSER_UNEXPECTED_FIELD,
    PARSER_VALUE_OUT_OF_RANGE,
    PARSER_INVALID_ADDRESS,
    // Context related errors
    PARSER_CONTEXT_MISMATCH,
    PARSER_CONTEXT_UNEXPECTED_SIZE,
    PARSER_CONTEXT_INVALID_CHARS,
    PARSER_CONTEXT_UNKNOWN_PREFIX,
    // Required fields
    PARSER_REQUIRED_NONCE,
    PARSER_REQUIRED_METHOD,
    //Template errors
    PARSER_TEMPLATE_TOO_MANY_HASHES,
    PARSER_TEMPLATE_ERROR,
    PARSER_TEMPLATE_TOO_MANY_ARGUMENTS

} parser_error_t;

typedef struct {
    const uint8_t *buffer;
    uint16_t bufferLen;
    uint16_t offset;
} parser_context_t;

#ifdef __cplusplus
}
#endif
