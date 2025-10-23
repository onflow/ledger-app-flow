#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <iostream>

extern "C" {
#include "bech32.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4) {
        return 0;
    }

    // Extract parameters from fuzzer input
    const uint8_t hrp_len = data[0] % 16 + 1;                               // 1-16 characters for HRP
    const uint8_t pad = data[1] % 2;                                        // 0 or 1 for padding
    const bech32_encoding enc = static_cast<bech32_encoding>(data[2] % 2);  // BECH32 or BECH32M

    // Skip the first 3 bytes used for parameters
    const uint8_t *input_data = data + 3;
    size_t input_size = size - 3;

    // Limit input size to prevent overly large inputs
    if (input_size > MAX_INPUT_SIZE) {
        input_size = MAX_INPUT_SIZE;
    }

    if (input_size == 0) {
        return 0;
    }

    // Create a simple HRP from the first byte
    char hrp[17] = {0};
    for (int i = 0; i < hrp_len && i < 16; i++) {
        // Use printable ASCII characters for HRP (a-z)
        hrp[i] = static_cast<char>('a' + (data[1] + i) % 26);
    }
    hrp[hrp_len] = '\0';

    // Allocate output buffer - generous size to avoid buffer issues
    const size_t output_size = 256;
    char output[output_size];

    // Call the function under test
    const zxerr_t result = bech32EncodeFromBytes(output, output_size, hrp, input_data, input_size, pad, enc);

    // We don't need to do anything with the result for fuzzing
    // The fuzzer is looking for crashes, not correctness
    (void)result;

    return 0;
}