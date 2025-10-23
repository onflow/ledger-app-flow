#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <vector>

extern "C" {
#include "base58.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) {
        return 0;
    }

    // Test decode_base58
    unsigned char decode_output[256];
    size_t decode_outlen = sizeof(decode_output);

    // Create null-terminated string for decode test
    std::vector<char> decode_input(size + 1);
    memcpy(decode_input.data(), data, size);
    decode_input[size] = '\0';

    const int decode_result = decode_base58(decode_input.data(), size, decode_output, &decode_outlen);
    (void)decode_result;

    // Test encode_base58
    unsigned char encode_output[512];
    size_t encode_outlen = sizeof(encode_output);

    const int encode_result = encode_base58(data, size, encode_output, &encode_outlen);
    (void)encode_result;

    // Test encode_base58_clip
    if (size > 0) {
        const char clip_result = encode_base58_clip(data[0]);
        (void)clip_result;
    }

    return 0;
}