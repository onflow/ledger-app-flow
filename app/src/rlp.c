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
#include "uint256.h"

parser_error_t rlp_decode(
        const uint8_t *data,
        rlp_kind_e *fieldKind,
        uint16_t *fieldLen,
        uint16_t *fieldStartOffset,
        uint16_t *bytesConsumed) {

    *fieldKind = kind_unknown;
    *fieldStartOffset = 0;
    *bytesConsumed = 0;

    uint8_t p = *data;
    if (p >= 0 && p <= 0x7F) {
        *fieldKind = kind_byte;
        *fieldLen = 0;
        *fieldStartOffset = 0;
        *bytesConsumed = 1; // 1 byte to consume from the stream
        return parser_ok;
    }

    if (p >= 0x80 && p <= 0xb7) {
        *fieldKind = kind_string;
        *fieldLen = p - 0x80;
        *fieldStartOffset = 1;
        *bytesConsumed = 1 + *fieldLen;
        return parser_ok;
    }

    if (p >= 0xb8 && p <= 0xbf) {
        *fieldKind = kind_string;
        uint8_t len_len = p - 0xb7;
        *fieldLen = 0;
        for (uint8_t i = 0; i < len_len; i++) {
            *fieldLen <<= 8u;
            *fieldLen += *(data + 1 + i);
        }
        *fieldStartOffset = 1 + len_len;
        *bytesConsumed = 1 + len_len + *fieldLen;
        return parser_ok;
    }

    if (p >= 0xc0 && p <= 0xf7) {
        *fieldKind = kind_list;
        *fieldLen = p - 0xc0;
        *fieldStartOffset = 1;
        *bytesConsumed = 1 + *fieldLen;
        return parser_ok;
    }

    if (p >= 0xf8 && p <= 0xff) {
        *fieldKind = kind_list;
        uint8_t len_len = p - 0xf7;
        *fieldLen = p - 0xf7;
        *fieldLen = 0;
        for (uint8_t i = 0; i < len_len; i++) {
            *fieldLen <<= 8u;
            *fieldLen += *(data + 1 + i);
        }
        *fieldStartOffset = 1 + len_len;
        *bytesConsumed = 1 + len_len + *fieldLen;
        return parser_ok;
    }

    return parser_unexpected_error;
}

parser_error_t rlp_parseStream(const uint8_t *data,
                               uint16_t dataOffset,
                               uint64_t dataLen,
                               rlp_field_t *fields,
                               uint8_t maxFieldCount,
                               uint16_t *fieldCount) {
    uint64_t offset = dataOffset;
    *fieldCount = 0;

    while (offset < dataLen && *fieldCount < maxFieldCount) {
        int16_t bytesConsumed;

        rlp_decode(
                data + offset,
                &fields[*fieldCount].kind,
                &fields[*fieldCount].valueLen,
                &fields[*fieldCount].valueOffset,
                &bytesConsumed);
        fields[*fieldCount].fieldOffset = offset;

        if (bytesConsumed < 0) {
            return bytesConsumed;   // as error
        }

        offset += bytesConsumed;
        (*fieldCount)++;
    }

    return parser_ok;
}

parser_error_t rlp_readByte(const uint8_t *data, const rlp_field_t *field, uint8_t *value) {
    if (field->kind != kind_byte)
        return parser_rlp_error_invalid_kind;

    if (field->valueLen != 0)
        return parser_rlp_error_invalid_value_len;

    if (field->valueOffset != 0)
        return parser_rlp_error_invalid_field_offset;

    *value = *(data + field->fieldOffset + field->valueOffset);

    return parser_ok;
}

parser_error_t rlp_readStringPaging(const uint8_t *data, const rlp_field_t *field,
                                    char *value, uint16_t maxLen,
                                    uint16_t *valueLen,
                                    uint8_t pageIdx, uint8_t *pageCount) {
    if (field->kind != kind_string)
        return parser_rlp_error_invalid_kind;

    MEMSET(value, 0, maxLen);
    maxLen--;   // need 1 bytes for string termination

    *pageCount = field->valueLen / maxLen;
    if (field->valueLen % maxLen > 0) {
        *pageCount = *pageCount + 1;
    }

    uint16_t pageOffset = pageIdx * maxLen;
    if (pageOffset > field->valueLen) {
        return parser_rlp_error_invalid_page;
    }

    *valueLen = maxLen;
    const uint16_t bytesLeft = field->valueLen - pageOffset;
    if (*valueLen > bytesLeft) {
        *valueLen = bytesLeft;
    }

    MEMCPY(value,
           data + field->fieldOffset + field->valueOffset + pageOffset,
           *valueLen);

    return parser_ok;
}

parser_error_t rlp_readString(const uint8_t *data, const rlp_field_t *field, char *value, uint16_t maxLen) {
    if (field->kind != kind_string)
        return parser_rlp_error_invalid_kind;

    if (field->valueLen > maxLen)
        return parser_rlp_error_buffer_too_small;

    uint8_t dummy;
    uint16_t dummy2;
    return rlp_readStringPaging(data, field, value, maxLen, &dummy2, 0, &dummy);
}

parser_error_t rlp_readList(const uint8_t *data,
                            const rlp_field_t *field,
                            rlp_field_t *listFields,
                            uint8_t maxListFieldCount,
                            uint16_t *listFieldCount) {
    if (field->kind != kind_list)
        return parser_rlp_error_invalid_kind;

    return rlp_parseStream(data,
                           field->fieldOffset + field->valueOffset,
                           field->fieldOffset + field->valueOffset + field->valueLen,
                           listFields,
                           maxListFieldCount,
                           listFieldCount);
}

parser_error_t rlp_readUInt256(const uint8_t *data,
                               const rlp_field_t *field,
                               uint256_t *value) {
    if (field->kind == kind_string) {
        uint8_t tmpBuffer[32];

        MEMSET(tmpBuffer, 0, 32);
        MEMMOVE(tmpBuffer - field->valueLen + 32,
                data + field->valueOffset + field->fieldOffset,
                field->valueLen);

        readu256BE(tmpBuffer, value);

        return parser_ok;
    }

    if (field->kind == kind_byte) {
        uint8_t tmpBuffer[32];

        MEMSET(tmpBuffer, 0, 32);
        rlp_readByte(data, field, tmpBuffer + 31);
        readu256BE(tmpBuffer, value);

        return parser_ok;
    }

    return parser_rlp_error_invalid_kind;
}
