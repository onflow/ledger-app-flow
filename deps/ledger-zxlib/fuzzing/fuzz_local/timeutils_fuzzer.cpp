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

#include "timeutils.h"

#ifdef __cplusplus
}
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 8) {
        return 0;
    }

    char buffer[256];

    // Test printTime with various timestamps
    uint64_t timestamp;
    memcpy(&timestamp, data, sizeof(uint64_t));

    // Test printTime function
    printTime(buffer, sizeof(buffer), timestamp);

    // Test with smaller buffer sizes
    printTime(buffer, 50, timestamp);
    printTime(buffer, 20, timestamp);
    printTime(buffer, 10, timestamp);
    printTime(buffer, 1, timestamp);

    // Test printTimeSpecialFormat if available
    printTimeSpecialFormat(buffer, sizeof(buffer), timestamp);
    printTimeSpecialFormat(buffer, 50, timestamp);

    // Test month functions
    if (size > 0) {
        const uint8_t month = data[0] % 13;  // 0-12
        const char *monthStr = getMonth(month);
        (void)monthStr;  // Use to avoid warning
    }

    // Test decodeTime function
    if (size >= 8) {
        timedata_t timedata;
        decodeTime(&timedata, timestamp);

        // Also test extractTime
        extractTime(timestamp, &timedata);
    }

    return 0;
}