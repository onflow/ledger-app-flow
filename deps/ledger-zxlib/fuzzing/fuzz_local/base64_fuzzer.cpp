#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <iostream>

extern "C" {
#include "base64.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) {
        return 0;
    }

    // Limit input size to prevent overly large allocations
    const size_t max_input_size = 1024;
    const size_t input_size = size > max_input_size ? max_input_size : size;

    // Allocate output buffer for base64 encoding
    // Base64 encoding requires approximately 4/3 the size of input + null terminator
    const size_t output_size = ((input_size + 2) / 3) * 4 + 1;
    char *output = new char[output_size];

    // Test base64_encode
    const uint16_t result =
        base64_encode(output, static_cast<uint16_t>(output_size), data, static_cast<uint16_t>(input_size));
    (void)result;

    delete[] output;

    return 0;
}