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

#include "rlp.h"

#define CHECK_AVAILABLE(ctx, size) \
    if (ctx->offset > ctx->bufferLen) return PARSER_UNEXPECTED_BUFFER_END; \
    if (ctx->bufferLen - ctx->offset < size ) return PARSER_UNEXPECTED_BUFFER_END;

#define CHECK_LEN_LEN_MAX(len_len, size) \
    if (len_len > size) return PARSER_RLP_ERROR_INVALID_VALUE_LEN;

parser_error_t rlp_decode(
        const parser_context_t *input,
        parser_context_t *outputPayload,
        rlp_kind_e *outputKind,
        uint32_t *bytesConsumed) {

    outputPayload->buffer = input->buffer + input->offset;
    outputPayload->bufferLen = 0;
    outputPayload->offset = 0;
    *outputKind = RLP_KIND_UNKNOWN;
    *bytesConsumed = 0;

    CHECK_AVAILABLE(input, 1)
    uint8_t p = *outputPayload->buffer;

    if (p >= 0 && p <= 0x7F) {
        *outputKind = RLP_KIND_STRING;
        outputPayload->bufferLen = 1;
        outputPayload->buffer += 0;
        *bytesConsumed = 1; // 1 byte to consume from the stream
        CHECK_AVAILABLE(input, *bytesConsumed)
        return PARSER_OK;
    }

    if (p >= 0x80 && p <= 0xb7) {
        *outputKind = RLP_KIND_STRING;
        outputPayload->bufferLen = p - 0x80;
        outputPayload->buffer += 1;
        *bytesConsumed = 1 + outputPayload->bufferLen;
        CHECK_AVAILABLE(input, *bytesConsumed)
        return PARSER_OK;
    }

    if (p >= 0xb8 && p <= 0xbf) {
        *outputKind = RLP_KIND_STRING;
        const uint8_t len_len = p - 0xb7;
        CHECK_AVAILABLE(input, 1 + len_len)
        CHECK_LEN_LEN_MAX(len_len, sizeof(outputPayload->bufferLen));

        outputPayload->bufferLen = 0;
        for (uint8_t i = 0; i < len_len; i++) {
            outputPayload->bufferLen <<= 8u;
            outputPayload->bufferLen += *(outputPayload->buffer + 1 + i);
        }
        outputPayload->buffer += 1 + len_len;
        *bytesConsumed = 1 + len_len + outputPayload->bufferLen;
        CHECK_AVAILABLE(input, *bytesConsumed)
        return PARSER_OK;
    }

    if (p >= 0xc0 && p <= 0xf7) {
        *outputKind = RLP_KIND_LIST;
        outputPayload->bufferLen = p - 0xc0;
        outputPayload->buffer += 1;
        *bytesConsumed = 1 + outputPayload->bufferLen;
        CHECK_AVAILABLE(input, *bytesConsumed)
        return PARSER_OK;
    }

    if (p >= 0xf8 && p <= 0xff) {
        *outputKind = RLP_KIND_LIST;
        const uint8_t len_len = p - 0xf7;
        CHECK_AVAILABLE(input, 1 + len_len)
        CHECK_LEN_LEN_MAX(len_len, sizeof(outputPayload->bufferLen));

        outputPayload->bufferLen = 0;
        for (uint8_t i = 0; i < len_len; i++) {
            outputPayload->bufferLen <<= 8u;
            outputPayload->bufferLen += *(outputPayload->buffer + 1 + i);
        }

        outputPayload->buffer += 1 + len_len;
        *bytesConsumed = 1 + len_len + outputPayload->bufferLen;
        CHECK_AVAILABLE(input, *bytesConsumed)
        return PARSER_OK;
    }

    return PARSER_UNEXPECTED_ERROR;
}

parser_error_t rlp_readByte(const parser_context_t *ctx, rlp_kind_e kind, uint8_t *value) {
    if (kind != RLP_KIND_STRING) {
        return PARSER_RLP_ERROR_INVALID_KIND;
    }

    if (ctx->bufferLen != 1) {
        return PARSER_RLP_ERROR_INVALID_VALUE_LEN;
    }

    if (ctx->offset != 0) {
        return PARSER_RLP_ERROR_INVALID_FIELD_OFFSET;
    }
    *value = *ctx->buffer;

    return PARSER_OK;
}

parser_error_t rlp_readUInt64(const parser_context_t *ctx,
                              rlp_kind_e kind,
                              uint64_t *value) {
    if (kind != RLP_KIND_STRING) {
        return PARSER_RLP_ERROR_INVALID_KIND;
    }

    // handle case when string is a single byte
    if (ctx->bufferLen == 1) {
        uint8_t tmp = 0;
        CHECK_PARSER_ERR(rlp_readByte(ctx, kind, &tmp))
        *value = tmp;
        return PARSER_OK;
    }

    // max size of uint64_t is 8 bytes
    if (ctx->bufferLen > 8) {
        return PARSER_RLP_ERROR_INVALID_VALUE_LEN;
    }

    *value = 0;

    for (uint8_t i = 0; i < ctx->bufferLen; i++) {
        *value <<= 8u;
        *value += *(ctx->buffer + ctx->offset + i);
    }

    return PARSER_OK;
}
