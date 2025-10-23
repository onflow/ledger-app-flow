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

#include "bignum.h"

#ifdef __cplusplus
}
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 2) {
        return 0;
    }

    // Test bignumLittleEndian_to_bcd and bignumLittleEndian_bcdprint
    uint8_t bcd_buffer[128];
    char print_buffer[256];

    // Limit input size to avoid too large allocations
    const size_t input_size = (size > 64) ? 64 : size;

    // Test little endian BCD conversion
    memset(bcd_buffer, 0, sizeof(bcd_buffer));
    bignumLittleEndian_to_bcd(bcd_buffer, sizeof(bcd_buffer), data, input_size);

    // Test BCD to string printing
    bignumLittleEndian_bcdprint(print_buffer, sizeof(print_buffer), bcd_buffer, sizeof(bcd_buffer));

    // Test big endian BCD conversion
    memset(bcd_buffer, 0, sizeof(bcd_buffer));
    bignumBigEndian_to_bcd(bcd_buffer, sizeof(bcd_buffer), data, input_size);

    // Test BCD to string printing for big endian
    bignumBigEndian_bcdprint(print_buffer, sizeof(print_buffer), bcd_buffer, sizeof(bcd_buffer));

    // Test with various buffer sizes
    if (size > 4) {
        const uint16_t bcd_len = (data[0] % 64) + 1;
        const uint16_t out_len = (data[1] % 128) + 1;

        if (bcd_len <= sizeof(bcd_buffer) && out_len <= sizeof(print_buffer)) {
            memset(bcd_buffer, 0, bcd_len);
            bignumLittleEndian_to_bcd(bcd_buffer, bcd_len, data + 2, input_size - 2);
            bignumLittleEndian_bcdprint(print_buffer, out_len, bcd_buffer, bcd_len);

            memset(bcd_buffer, 0, bcd_len);
            bignumBigEndian_to_bcd(bcd_buffer, bcd_len, data + 2, input_size - 2);
            bignumBigEndian_bcdprint(print_buffer, out_len, bcd_buffer, bcd_len);
        }
    }

    return 0;
}