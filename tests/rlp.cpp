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

#include "gmock/gmock.h"
#include <iostream>
#include <hexutils.h>
#include "rlp.h"

using ::testing::TestWithParam;
using ::testing::Values;

struct RLPValueTestCase {
    const char *data;
    uint8_t expectedKind;
    uint64_t expectedLen;
    uint64_t expectedDataOffset;
    uint64_t expectedConsumed;
};

class RLPDecodeTest : public testing::TestWithParam<RLPValueTestCase> {
public:
    void SetUp() override {}

    void TearDown() override {}
};

// Test cases adapted from official Ethereum RLP tests: https://github.com/ethereum/tests/blob/develop/RLPTests/rlptest.json
INSTANTIATE_TEST_SUITE_P(
        InstantiationName,
        RLPDecodeTest,
        Values(
                RLPValueTestCase{"00", RLP_KIND_STRING, 1, 0, 1}, // Byte string (00)
                RLPValueTestCase{"01", RLP_KIND_STRING, 1, 0, 1}, // Byte string (01)
                RLPValueTestCase{"7F", RLP_KIND_STRING, 1, 0, 1}, // Byte string (7F)

                RLPValueTestCase{"80", RLP_KIND_STRING, 0, 1, 1},       // Empty string ("")
                RLPValueTestCase{"83646F67", RLP_KIND_STRING, 3, 1, 4}, // Short string ("dog")

                RLPValueTestCase{"B7"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000", RLP_KIND_STRING, 55, 1,
                                 56},
                RLPValueTestCase{"B90400"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000", RLP_KIND_STRING, 1024,
                                 3, 1027},
                RLPValueTestCase{"C0", RLP_KIND_LIST, 0, 1, 1},
                RLPValueTestCase{"C80000000000000000", RLP_KIND_LIST, 8, 1, 9},
                RLPValueTestCase{"F7"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000", RLP_KIND_LIST, 55, 1,
                                 56},
                RLPValueTestCase{"F90400"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000"
                                 "0000000000000000000000000000000000000000000000000000000000000000", RLP_KIND_LIST, 1024, 3,
                                 1027}
        )
);

TEST_P(RLPDecodeTest, decodeElement) {
    auto params = GetParam();

    uint8_t data[2000];
    const size_t dataSize = parseHexString(data, sizeof(data), params.data);

    parser_context_t ctx_in;
    parser_context_t ctx_out;

    ctx_in.buffer = data;
    ctx_in.bufferLen = dataSize;
    ctx_in.offset = 0;

    rlp_kind_e kind;
    uint32_t bytesConsumed;

    parser_error_t err = rlp_decode(&ctx_in, &ctx_out, &kind, &bytesConsumed);

    EXPECT_THAT(err, PARSER_OK);
    EXPECT_THAT(kind, testing::Eq(params.expectedKind));
    EXPECT_THAT(ctx_out.bufferLen, testing::Eq(params.expectedLen));
    EXPECT_THAT(ctx_out.buffer - ctx_in.buffer, testing::Eq(params.expectedDataOffset));
    EXPECT_THAT(bytesConsumed, testing::Eq(params.expectedConsumed));
}