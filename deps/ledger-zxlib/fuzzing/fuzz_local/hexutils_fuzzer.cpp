#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <iostream>

extern "C" {
#include "hexutils.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) {
        return 0;
    }

    // Create null-terminated string for parseHexString
    char *input = new char[size + 1];
    memcpy(input, data, size);
    input[size] = '\0';

    // Allocate output buffer
    const uint16_t output_size = 256;
    uint8_t output[output_size];

    // Test parseHexString
    const size_t result = parseHexString(output, output_size, input);
    (void)result;

    delete[] input;

    return 0;
}