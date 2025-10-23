#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern "C" {
#include "segwit_addr.h"
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < 4) {
        return 0;
    }

    // Extract parameters from fuzzer input
    const uint8_t hrp_len = (data[0] % 16) + 1;                                   // 1-16 characters for HRP
    const int ver = data[1] % 17;                                                 // Version 0-16
    const bech32_encoding enc = static_cast<bech32_encoding>((data[2] % 2) + 1);  // BECH32 or BECH32M

    // Skip the first 3 bytes used for parameters
    const uint8_t *prog_data = data + 3;
    size_t prog_len = size - 3;

    // Limit program length (2-40 bytes as per spec)
    if (prog_len < 2) {
        return 0;
    }
    if (prog_len > 40) {
        prog_len = 40;
    }

    // Create HRP
    char hrp[17] = {0};
    for (int i = 0; i < hrp_len && i < 16; i++) {
        hrp[i] = static_cast<char>('a' + (data[0] + i) % 26);
    }
    hrp[hrp_len] = '\0';

    // Test segwit_addr_encode
    char encode_output[256];
    const int encode_result = segwit_addr_encode(encode_output, hrp, ver, prog_data, prog_len);
    (void)encode_result;

    // Test segwit_addr_decode
    if (encode_result) {
        int decode_ver;
        uint8_t decode_prog[40];
        size_t decode_prog_len;

        const int decode_result = segwit_addr_decode(&decode_ver, decode_prog, &decode_prog_len, hrp, encode_output);
        (void)decode_result;
    }

    // Test bech32_encode with 5-bit data
    uint8_t data_5bit[64];
    const size_t data_5bit_len = (prog_len < 64) ? prog_len : 64;
    for (size_t i = 0; i < data_5bit_len; i++) {
        data_5bit[i] = prog_data[i] & 0x1F;  // Convert to 5-bit values
    }

    char bech32_output[256];
    const int bech32_result = bech32_encode(bech32_output, hrp, data_5bit, data_5bit_len, enc);
    (void)bech32_result;

    // Test bech32_decode
    if (bech32_result) {
        char decode_hrp[256];
        uint8_t decode_data[256];
        size_t decode_data_len;

        const bech32_encoding decode_enc = bech32_decode(decode_hrp, decode_data, &decode_data_len, bech32_output);
        (void)decode_enc;
    }

    return 0;
}