/*******************************************************************************
 *   (c) 2018 - 2024 Zondax AG
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
#include <bech32.h>
#include <gmock/gmock.h>
#include <hexutils.h>
#include <zxformat.h>
#include <zxmacros.h>

#include <cstring>

namespace {
TEST(BECH32, hex_to_address) {
    char addr_out[100];
    const char *hrp = "zx";

    uint8_t data1[] = {1, 3, 5};
    uint8_t data2[] = {1, 3, 5, 7, 9, 11, 13};

    // These should fail with pad=0 due to invalid conversion per BIP173 spec
    auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data1, sizeof(data1), 0, BECH32_ENCODING_BECH32);
    ASSERT_EQ(err, zxerr_encoding_failed);

    err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data2, sizeof(data2), 0, BECH32_ENCODING_BECH32);
    ASSERT_EQ(err, zxerr_encoding_failed);

    ///
    err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data1, sizeof(data1), 1, BECH32_ENCODING_BECH32);
    ASSERT_EQ(err, zxerr_ok);
    std::cout << addr_out << std::endl;
    ASSERT_STREQ("zx1qyps2ucfnzd", addr_out);

    err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data2, sizeof(data2), 1, BECH32_ENCODING_BECH32);
    ASSERT_EQ(err, zxerr_ok);
    std::cout << addr_out << std::endl;
    ASSERT_STREQ("zx1qyps2pcfpvxshamanz", addr_out);
}

TEST(BECH32, huge_input) {
    char addr_out[200];
    const char *hrp = "zx";

    auto data = std::vector<uint8_t>(1000, 0x55);

    auto err =
        bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data.data(), data.size(), 0, BECH32_ENCODING_BECH32);
    ASSERT_EQ(err, zxerr_out_of_bounds);

    std::cout << addr_out << std::endl;
}

TEST(BECH32, small_output) {
    char addr_out[1000];
    const char *hrp = "zx";

    auto data = std::vector<uint8_t>(32, 0x55);

    MEMZERO(addr_out, sizeof(addr_out));

    // declare size to be smaller
    const size_t declared_size = 52;

    auto err = bech32EncodeFromBytes(addr_out, declared_size, hrp, data.data(), data.size(), 0, BECH32_ENCODING_BECH32);
    ASSERT_EQ(err, zxerr_buffer_too_small);

    for (size_t i = declared_size; i < sizeof(addr_out); i++) {
        ASSERT_EQ(addr_out[i], 0);
    }

    std::cout << addr_out << std::endl;
}

TEST(BECH32M, hex_to_address) {
    char addr_out[100];
    const char *hrp = "zx";

    uint8_t data1[] = {1, 3, 5};
    uint8_t data2[] = {1, 3, 5, 7, 9, 11, 13};

    // These should fail with pad=0 due to invalid conversion per BIP173 spec
    auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data1, sizeof(data1), 0, BECH32_ENCODING_BECH32M);
    ASSERT_EQ(err, zxerr_encoding_failed);

    err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data2, sizeof(data2), 0, BECH32_ENCODING_BECH32M);
    ASSERT_EQ(err, zxerr_encoding_failed);

    ///
    err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data1, sizeof(data1), 1, BECH32_ENCODING_BECH32M);
    ASSERT_EQ(err, zxerr_ok);
    std::cout << addr_out << std::endl;
    ASSERT_STREQ("zx1qyps2fyel80", addr_out);

    err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data2, sizeof(data2), 1, BECH32_ENCODING_BECH32M);
    ASSERT_EQ(err, zxerr_ok);
    std::cout << addr_out << std::endl;
    ASSERT_STREQ("zx1qyps2pcfpvxszpt3kq", addr_out);
}

TEST(BECH32M, huge_input) {
    char addr_out[200];
    const char *hrp = "zx";

    auto data = std::vector<uint8_t>(1000, 0x55);

    auto err =
        bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data.data(), data.size(), 0, BECH32_ENCODING_BECH32M);
    ASSERT_EQ(err, zxerr_out_of_bounds);

    std::cout << addr_out << std::endl;
}

TEST(BECH32M, small_output) {
    char addr_out[1000];
    const char *hrp = "zx";

    auto data = std::vector<uint8_t>(32, 0x55);

    MEMZERO(addr_out, sizeof(addr_out));

    // declare size to be smaller
    const size_t declared_size = 52;

    auto err =
        bech32EncodeFromBytes(addr_out, declared_size, hrp, data.data(), data.size(), 0, BECH32_ENCODING_BECH32M);
    ASSERT_EQ(err, zxerr_buffer_too_small);

    for (size_t i = declared_size; i < sizeof(addr_out); i++) {
        ASSERT_EQ(addr_out[i], 0);
    }

    std::cout << addr_out << std::endl;
}

// Comprehensive test cases with BIP173 test vectors and edge cases
TEST(BECH32, bip173_test_vectors) {
    char addr_out[256];

    // Test case 1: Raw bech32 encoding (not segwit address)
    // Note: bech32EncodeFromBytes does raw bech32 encoding, not segwit address encoding
    {
        uint8_t data[] = {0x00, 0x14, 0x75, 0x1e, 0x76, 0xe8};
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Verify it starts with correct HRP
        ASSERT_EQ(strncmp(addr_out, "bc1", 3), 0);
    }

    // Test case 2: Known test vectors from reference implementation
    {
        // Empty data with padding
        uint8_t data[] = {0};
        auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "a", data, 0, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ("a12uel5l", addr_out);
    }

    // Test case 3: Basic encoding test
    {
        uint8_t data[] = {0x00};
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "a", data, sizeof(data), 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Just verify successful encoding
        ASSERT_GT(strlen(addr_out), 0);
    }

    // Test case 4: Verify HRP and separator
    {
        uint8_t data[] = {0x11, 0x22, 0x33};
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "test", data, sizeof(data), 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Verify HRP and separator
        ASSERT_EQ(strncmp(addr_out, "test1", 5), 0);
    }
}

TEST(BECH32, edge_cases_buffer_boundaries) {
    char addr_out[256];

    // Test case 1: Minimum valid data (1 byte)
    {
        uint8_t data[] = {0x00};
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Verify encoding succeeded
        ASSERT_GT(strlen(addr_out), 0);
        ASSERT_EQ(strncmp(addr_out, "bc1", 3), 0);
    }

    // Test case 2: 40 bytes data
    {
        uint8_t data[40];
        memset(data, 0xAB, sizeof(data));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Verify the output starts with expected HRP
        ASSERT_EQ(strncmp(addr_out, "bc1", 3), 0);
    }

    // Test case 3: Exactly fitting output buffer
    {
        uint8_t data[] = {0x00, 0x14, 0x75, 0x1e};
        char small_out[16];  // Deliberately small
        auto err =
            bech32EncodeFromBytes(small_out, sizeof(small_out), "bc", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        // This should fail due to insufficient buffer
        ASSERT_EQ(err, zxerr_buffer_too_small);
    }
}

TEST(BECH32, edge_cases_empty_null) {
    char addr_out[256];

    // Test case 1: Empty data array with valid parameters
    {
        uint8_t empty_data[1] = {0};  // Can't pass nullptr, use zero-length instead
        auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", empty_data, 0, 1, BECH32_ENCODING_BECH32);
        // Empty data should still encode successfully with just checksum
        ASSERT_EQ(err, zxerr_ok);
    }

    // Test case 2: Empty HRP (should succeed per bech32 spec)
    {
        uint8_t data[] = {0x00, 0x14};
        auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        // Empty HRP is actually valid in bech32 spec
        ASSERT_EQ(err, zxerr_ok);
    }

    // Test case 3: Zero output buffer size
    {
        uint8_t data[] = {0x00, 0x14};
        auto err = bech32EncodeFromBytes(addr_out, 0, "bc", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_buffer_too_small);
    }
}

TEST(BECH32, different_hrp_lengths) {
    char addr_out[256];
    uint8_t data[] = {0x00, 0x14, 0x75, 0x1e, 0x76, 0xe8};

    // Test case 1: Single character HRP
    {
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "a", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_EQ(strncmp(addr_out, "a1", 2), 0);
    }

    // Test case 2: Maximum reasonable HRP (83 chars is the limit)
    {
        const char *long_hrp = "an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio";
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), long_hrp, data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        // This should fail due to total length exceeding 90 characters
        ASSERT_NE(err, zxerr_ok);
    }

    // Test case 3: Common cryptocurrency HRPs
    {
        const char *hrps[] = {"bc", "tb", "bcrt", "ltc", "tltc"};
        for (const auto &hrp : hrps) {
            auto err =
                bech32EncodeFromBytes(addr_out, sizeof(addr_out), hrp, data, sizeof(data), 0, BECH32_ENCODING_BECH32);
            ASSERT_EQ(err, zxerr_ok);
            // Verify HRP is in output
            ASSERT_EQ(strncmp(addr_out, hrp, strlen(hrp)), 0);
        }
    }
}

TEST(BECH32, maximum_valid_sizes) {
    char addr_out[256];

    // Test case 1: 65 bytes (exceeds MAX_INPUT_SIZE of 64)
    {
        uint8_t data[65];
        memset(data, 0x55, sizeof(data));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        // Should fail due to MAX_INPUT_SIZE limit
        ASSERT_EQ(err, zxerr_out_of_bounds);
    }

    // Test case 2: Exactly at MAX_INPUT_SIZE boundary (64 bytes)
    {
        uint8_t data[64];  // MAX_INPUT_SIZE is 64
        memset(data, 0xAA, sizeof(data));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        // Should fail due to bech32 90 character limit
        // 64 bytes -> ~102 base32 chars + hrp + checksum > 90
        ASSERT_EQ(err, zxerr_encoding_failed);
    }

    // Test case 3: Test with safe size that won't exceed 90 char limit
    {
        // With "bc" HRP (2 chars) + separator (1) + checksum (6) = 9 overhead
        // So we can have max 81 base32 chars, which is ~50 bytes of data
        uint8_t data[40];
        memset(data, 0xBB, sizeof(data));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        // Should succeed with 40 bytes
        ASSERT_EQ(err, zxerr_ok);
    }
}

TEST(BECH32, bech32_vs_bech32m_comparison) {
    char addr_out_bech32[256];
    char addr_out_bech32m[256];

    // Test with same data but different encoding
    uint8_t data[] = {0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94,
                      0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6};

    // Encode with BECH32
    auto err1 = bech32EncodeFromBytes(addr_out_bech32, sizeof(addr_out_bech32), "bc", data, sizeof(data), 0,
                                      BECH32_ENCODING_BECH32);
    ASSERT_EQ(err1, zxerr_ok);

    // Encode with BECH32M
    auto err2 = bech32EncodeFromBytes(addr_out_bech32m, sizeof(addr_out_bech32m), "bc", data, sizeof(data), 0,
                                      BECH32_ENCODING_BECH32M);
    ASSERT_EQ(err2, zxerr_ok);

    // Outputs should be different (different checksum algorithm)
    ASSERT_STRNE(addr_out_bech32, addr_out_bech32m);

    // But they should have the same prefix (HRP + separator + data)
    size_t prefix_len = strlen("bc1") + (sizeof(data) * 8 + 4) / 5;  // data encoded in base32
    ASSERT_EQ(strncmp(addr_out_bech32, addr_out_bech32m, prefix_len), 0);
}

TEST(BECH32, special_patterns) {
    char addr_out[256];

    // Test case 1: All zeros
    {
        uint8_t data[20];
        memset(data, 0x00, sizeof(data));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Just verify successful encoding - exact output depends on implementation
        ASSERT_GT(strlen(addr_out), 0);
        ASSERT_EQ(strncmp(addr_out, "bc1", 3), 0);
    }

    // Test case 2: All ones (0xFF)
    {
        uint8_t data[20];
        memset(data, 0xFF, sizeof(data));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Just verify successful encoding
        ASSERT_GT(strlen(addr_out), 0);
        ASSERT_EQ(strncmp(addr_out, "bc1", 3), 0);
    }

    // Test case 3: Alternating pattern
    {
        uint8_t data[20];
        for (size_t i = 0; i < sizeof(data); i++) {
            data[i] = (i % 2) ? 0xFF : 0x00;
        }
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data, sizeof(data), 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        // Just verify it encodes successfully
        ASSERT_GT(strlen(addr_out), 0);
    }

    // Test case 4: Known pattern that we verified in existing tests
    {
        // This should fail with pad=0 due to invalid conversion per BIP173 spec
        uint8_t data[] = {1, 3, 5};
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "zx", data, sizeof(data), 0, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_encoding_failed);
    }
}

// Real test vectors using hex strings converted to byte arrays
// Generated using reference implementation from sipa/bech32 with pad=True
TEST(BECH32, verified_hex_test_vectors) {
    char addr_out[256];
    uint8_t data_buffer[65];

    // 4-byte hex with bc HRP, raw data (pad=1)
    {
        const char *hex_input = "deadbeef";
        const char *expected = "bc1m6kmamcr0f7ys";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // 1-byte hex with test HRP, raw data (pad=1)
    {
        const char *hex_input = "ff";
        const char *expected = "test1lu0zy72x";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "test", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Zero byte with minimal HRP, raw data (pad=1)
    {
        const char *hex_input = "00";
        const char *expected = "a1qqqd87cq";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "a", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // 8-byte pattern with witness version 1 (pad=1)
    {
        const char *hex_input = "a1b2c3d4e5f67890";
        const char *expected = "test15xev84897eufqjdk5ap";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "test", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }
}

// Real test vectors for bech32m encoding
TEST(BECH32M, verified_hex_test_vectors) {
    char addr_out[256];
    uint8_t data_buffer[65];

    // 4-byte hex with bc HRP, raw data (pad=1, bech32m)
    {
        const char *hex_input = "deadbeef";
        const char *expected = "bc1m6kmamcknejpj";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32M);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // 1-byte hex with test HRP, raw data (pad=1, bech32m)
    {
        const char *hex_input = "ff";
        const char *expected = "test1lu675j0y";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "test", data_buffer, data_len, 1,
                                         BECH32_ENCODING_BECH32M);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // 32-byte hash, raw data (pad=1, bech32m)
    {
        const char *hex_input = "1863143c14c5166804bd19203356da136c985678cd4d27a1b8c6329604903262";
        const char *expected = "bc1rp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3qmp8a48";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32M);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }
}

// Test cases with pad=0 to verify different padding behavior
TEST(BECH32, pad_zero_test_vectors) {
    char addr_out[256];
    uint8_t data_buffer[65];

    // 8-byte pattern with pad=0 - should produce different result than pad=1
    {
        const char *hex_input = "a1b2c3d4e5f67890";
        const char *expected = "test15xev84897eufqjdk5u";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "test", data_buffer, data_len, 0, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // 5-byte pattern with pad=0 (5*8=40 bits, divisible by 5)
    {
        const char *hex_input = "0102030405";
        const char *expected = "bc1qypqxpq9f3wf05";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 0, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }
}

// === ADDITIONAL COMPREHENSIVE TEST CASES ===
// Generated using reference implementation from sipa/bech32
// Covers additional edge cases not already tested above
TEST(BECH32, additional_comprehensive_cases) {
    char addr_out[256];
    uint8_t data_buffer[65];

    // Test 1: Sequential patterns
    {
        const char *hex_input = "01";
        const char *expected = "bc1qylhukqn";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 2: 2-byte sequential
    {
        const char *hex_input = "0102";
        const char *expected = "bc1qypqxe39ep";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 3: All zeros pattern
    {
        const char *hex_input = "0000";
        const char *expected = "bc1qqqqpvn3du";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 4: All ones pattern
    {
        const char *hex_input = "ffff";
        const char *expected = "bc1lllspe864m";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 5: Different HRP (testnet)
    {
        const char *hex_input = "abcd1234";
        const char *expected = "tb140x3ydqljuq3a";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "tb", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 6: Medium-sized pattern (16 bytes)
    {
        const char *hex_input = "0123456789abcdef0123456789abcdef";
        const char *expected = "bc1qy352euf40x77qfrg4ncn27daup8f5qu";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 7: Bech32m vs Bech32 comparison (same input, different encoding)
    {
        const char *hex_input = "fedcba9876543210";
        const char *expected_bech32 = "bc1lmwt4xrk2sepqqfalwf";
        const char *expected_bech32m = "bc1lmwt4xrk2sepq44dntt";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));

        // Test bech32
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected_bech32, addr_out);

        // Test bech32m - should produce different checksum
        err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32M);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected_bech32m, addr_out);
    }

    // Test 8: fuzzing error
    {
        const char *hex_input = "000000000000000000ffffffe21de3e2e200ff45";
        const char *expected_bech32 = "coston1qqqqqqqqqqqqqq8llll7y80rut3qpl69vzuqg3";
        const char *expected_bech32m = "coston1qqqqqqqqqqqqqq8llll7y80rut3qpl69e7vvdn";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));

        // Test bech32
        auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "coston", data_buffer, data_len, 1,
                                         BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected_bech32, addr_out);

        // Test bech32m - should produce different checksum
        err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "coston", data_buffer, data_len, 1,
                                    BECH32_ENCODING_BECH32M);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected_bech32m, addr_out);
    }

    // Test 9: 9-byte sequential pattern
    {
        const char *hex_input = "010203040506070809";
        const char *expected = "bc1qypqxpq9qcrsszgg5n5cx";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 10: Boundary value 0x7f (127 in decimal)
    {
        const char *hex_input = "7f";
        const char *expected = "bc10unaheek";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 11: Boundary value 0x80 (128 in decimal, signed byte boundary)
    {
        const char *hex_input = "80";
        const char *expected = "bc1sqkgcwmj";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 12: 16-byte pattern with pad=0 (good for exact bit conversion)
    {
        const char *hex_input = "0102030405060708090a0b0c0d0e0f10";
        const char *expected = "bc1qypqxpq9qcrsszg2pvxq6rs0znehv8q";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 0, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 13: Alternating 0xaa pattern (20 bytes) with single character HRP
    {
        const char *hex_input = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        const char *expected = "a142424242424242424242424242424242a5wyat";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "a", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 14: Pi digits pattern with cosmos HRP and bech32m encoding
    {
        const char *hex_input = "31415926535897932384";
        const char *expected = "cosmos1x9q4jfjntztexguy735m72";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "cosmos", data_buffer, data_len, 1,
                                         BECH32_ENCODING_BECH32M);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 15: Litecoin HRP with 0x55 pattern (6 bytes)
    {
        const char *hex_input = "555555555555";
        const char *expected = "ltc12424242425c8takx";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "ltc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 16: Mixed hex pattern with test HRP and bech32m
    {
        const char *hex_input = "0f1e2d3c4b5a69780f1e2d3c4b5a6978";
        const char *expected = "test1pu0z60zttf5hsrc7957ykknf0qmcjp8z";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err = bech32EncodeFromBytes(addr_out, sizeof(addr_out), "test", data_buffer, data_len, 1,
                                         BECH32_ENCODING_BECH32M);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }

    // Test 17: Boundary value 0xfe (254, near max byte value)
    {
        const char *hex_input = "fe";
        const char *expected = "bc1lc6znpzh";
        uint32_t data_len = hexstr_to_array(data_buffer, sizeof(data_buffer), hex_input, strlen(hex_input));
        auto err =
            bech32EncodeFromBytes(addr_out, sizeof(addr_out), "bc", data_buffer, data_len, 1, BECH32_ENCODING_BECH32);
        ASSERT_EQ(err, zxerr_ok);
        ASSERT_STREQ(expected, addr_out);
    }
}
}  // namespace
