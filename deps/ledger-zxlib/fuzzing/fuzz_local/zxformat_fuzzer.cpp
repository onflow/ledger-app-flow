/*******************************************************************************
 *   (c) 2025 Zondax AG
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif

#include "zxformat.h"

#ifdef __cplusplus
}
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4) {
        return 0;
    }

    char buffer[256];

    // Test int32_to_str with various inputs
    if (size >= 4) {
        int32_t value;
        memcpy(&value, data, sizeof(int32_t));
        int32_to_str(buffer, sizeof(buffer), value);

        // Test with smaller buffer sizes
        int32_to_str(buffer, 10, value);
        int32_to_str(buffer, 5, value);
        int32_to_str(buffer, 2, value);
        int32_to_str(buffer, 1, value);
    }

    // Test uint32_to_str with various inputs
    if (size >= 4) {
        uint32_t value;
        memcpy(&value, data, sizeof(uint32_t));
        uint32_to_str(buffer, sizeof(buffer), value);

        // Test with smaller buffer sizes
        uint32_to_str(buffer, 10, value);
        uint32_to_str(buffer, 5, value);
        uint32_to_str(buffer, 2, value);
        uint32_to_str(buffer, 1, value);
    }

    // Test fpstr_to_str (fixed point string conversion)
    if (size >= 16) {
        char inBuffer[128];
        const size_t copy_len = (size > 127) ? 127 : size;
        memcpy(inBuffer, data, copy_len);
        inBuffer[copy_len] = '\0';

        // Test with various decimal places
        const uint8_t decimals = data[0] % 20;
        fpstr_to_str(buffer, sizeof(buffer), inBuffer, decimals);
    }

    // Test pageString function (with correct parameters)
    if (size >= 32) {
        char outValue[128];
        const uint16_t outValueLen = sizeof(outValue);

        char inValue[64];
        const size_t copy_len = (size > 63) ? 63 : size;
        memcpy(inValue, data, copy_len);
        inValue[copy_len] = '\0';

        const uint8_t pageIdx = data[0] % 10;
        uint8_t pageCount = 0;

        pageString(outValue, outValueLen, inValue, pageIdx, &pageCount);
    }

    // Test fpuint64_to_str
    if (size >= 8) {
        uint64_t value;
        memcpy(&value, data, sizeof(uint64_t));
        const uint8_t decimals = data[0] % 10;
        fpuint64_to_str(buffer, sizeof(buffer), value, decimals);
    }

    // Test str_to_int64 with correct parameters
    if (size > 2) {
        char str[64];
        const size_t str_len = (size > 63) ? 63 : size;
        memcpy(str, data, str_len);
        str[str_len] = '\0';

        // Find a reasonable end pointer
        const char *end = str + str_len;
        char error = 0;
        str_to_int64(str, end, &error);
    }

    // Test array to hex functions
    if (size >= 8) {
        array_to_hexstr(buffer, sizeof(buffer), data, (size > 64) ? 64 : size);
        array_to_hexstr_uppercase(buffer, sizeof(buffer), data, (size > 64) ? 64 : size);
    }

    // Test bip32_to_str
    if (size >= 20) {
        char bip32Buffer[256];
        uint32_t path[6];
        const uint8_t pathLen = (data[0] % 5) + 1;

        // Initialize all path elements to 0
        memset(path, 0, sizeof(path));

        // Only fill the number of elements we actually have data for
        for (uint8_t i = 0; i < pathLen && static_cast<size_t>(i) * 4 + 4 <= size - 1; i++) {
            memcpy(&path[i], data + 1 + static_cast<size_t>(i) * 4, sizeof(uint32_t));
        }

        bip32_to_str(bip32Buffer, sizeof(bip32Buffer), path, pathLen);
        bip32_to_str(bip32Buffer, 10, path, pathLen);
        bip32_to_str(bip32Buffer, 5, path, pathLen);
        bip32_to_str(bip32Buffer, sizeof(bip32Buffer), path, 0);
        bip32_to_str(bip32Buffer, sizeof(bip32Buffer), path, 6);
    }

    // Test str_to_int8
    if (size > 2) {
        char str[16];
        const size_t str_len = (size > 15) ? 15 : size;
        memcpy(str, data, str_len);
        str[str_len] = '\0';

        const char *end = str + str_len;
        char error = 0;
        str_to_int8(str, end, &error);
        str_to_int8(str, end, NULL);
    }

    // Test number_inplace_trimming
    if (size > 8) {
        char numberStr[128];
        const size_t copy_len = (size > 127) ? 127 : size;
        memcpy(numberStr, data, copy_len);
        numberStr[copy_len] = '\0';

        const uint8_t non_trimmed = data[0] % 10;
        number_inplace_trimming(numberStr, non_trimmed);

        // Test with decimal point
        strcpy(numberStr, "123.456000");
        number_inplace_trimming(numberStr, 2);

        strcpy(numberStr, "0.000000");
        number_inplace_trimming(numberStr, 0);
    }

    // Test hexstr_to_array
    if (size >= 4) {
        uint8_t outputArray[128];
        char hexStr[256];

        // Create a valid hex string from data
        const size_t hexLen = (size > 127) ? 127 : size;
        array_to_hexstr(hexStr, sizeof(hexStr), data, hexLen);

        hexstr_to_array(outputArray, sizeof(outputArray), hexStr, strlen(hexStr));
        hexstr_to_array(outputArray, 10, hexStr, strlen(hexStr));
        hexstr_to_array(outputArray, sizeof(outputArray), hexStr, 3);
    }

    // Test to_uppercase and to_lowercase
    if (size >= 1) {
        uint8_t letter = data[0];
        to_uppercase(&letter);

        letter = data[0];
        to_lowercase(&letter);

        to_uppercase(NULL);
        to_lowercase(NULL);
    }

    // Test array_to_uppercase and array_to_lowercase
    if (size >= 4) {
        uint8_t upperArray[64];
        uint8_t lowerArray[64];
        const size_t array_len = (size > 64) ? 64 : size;

        memcpy(upperArray, data, array_len);
        memcpy(lowerArray, data, array_len);

        array_to_uppercase(upperArray, array_len);
        array_to_lowercase(lowerArray, array_len);

        array_to_uppercase(NULL, array_len);
        array_to_lowercase(NULL, array_len);
    }

    // Test pageStringHex
    if (size >= 16) {
        char hexOutput[128];
        const uint16_t hexOutputLen = sizeof(hexOutput);

        const uint16_t inValueLen = (size > 256) ? 256 : size;
        const uint8_t pageIdx = data[0] % 10;
        uint8_t pageCount = 0;

        pageStringHex(hexOutput, hexOutputLen, (const char *)data, inValueLen, pageIdx, &pageCount);
        pageStringHex(hexOutput, 10, (const char *)data, inValueLen, pageIdx, &pageCount);
        pageStringHex(hexOutput, 1, (const char *)data, inValueLen, pageIdx, &pageCount);
        pageStringHex(hexOutput, hexOutputLen, (const char *)data, 0, pageIdx, &pageCount);
    }

    // Test formatBufferData
    if (size >= 8) {
        char formattedOutput[256];
        const uint16_t outputLen = sizeof(formattedOutput);

        const uint64_t dataLen = (size > 500) ? 500 : size;
        const uint8_t pageIdx = data[0] % 10;
        uint8_t pageCount = 0;

        // Copy data to a bounded local buffer to avoid potential out-of-bounds access
        uint8_t localBuffer[500];
        const size_t copyLen = (size > 500) ? 500 : size;
        memcpy(localBuffer, data, copyLen);

        formatBufferData(localBuffer, dataLen, formattedOutput, outputLen, pageIdx, &pageCount);
        formatBufferData(localBuffer, dataLen, formattedOutput, 10, pageIdx, &pageCount);
        // Test with valid length instead of 501 which exceeds our local buffer
        formatBufferData(localBuffer, copyLen, formattedOutput, outputLen, pageIdx, &pageCount);

        // Test with non-ASCII data
        uint8_t nonAsciiData[64];
        memset(nonAsciiData, 0x80, sizeof(nonAsciiData));
        const size_t naCopy = size < sizeof(nonAsciiData) ? size : sizeof(nonAsciiData);
        for (size_t i = 0; i < naCopy; i++) {
            nonAsciiData[i] = data[i] | 0x80;
        }
        formatBufferData(nonAsciiData, naCopy, formattedOutput, outputLen, pageIdx, &pageCount);
    }

    // Test uint64_from_BEarray
    if (size >= 8) {
        uint8_t beArray[8];
        memcpy(beArray, data, 8);

        uint64_t result = uint64_from_BEarray(beArray);

        // Test with different patterns
        if (size >= 16) {
            memcpy(beArray, data + 8, 8);
            const uint64_t result2 = uint64_from_BEarray(beArray);
            // Use results to avoid dead store warning
            if (result > result2) {
                result = result2;
            }
        }

        // Test with all zeros
        memset(beArray, 0, 8);
        const uint64_t zeroResult = uint64_from_BEarray(beArray);
        if (zeroResult != 0) {
            result = zeroResult;
        }

        // Test with all ones
        memset(beArray, 0xFF, 8);
        const uint64_t onesResult = uint64_from_BEarray(beArray);
        if (onesResult != 0xFFFFFFFFFFFFFFFF) {
            result = onesResult;
        }

        // Force use of result
        if (result == 0) {
            beArray[0] = 0;
        }
    }

    return 0;
}