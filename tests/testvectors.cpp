#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err58-cpp"
/*******************************************************************************
*   (c) 2019 Zondax GmbH
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

#include <gmock/gmock.h>
#include "utils/testcases.h"

#include <iostream>
#include <memory>
#include <app_mode.h>
#include "parser.h"
#include "utils/common.h"
#include "hdpath.h"

using ::testing::TestWithParam;

void check_testcase(const testcase_t &testcase) {
    auto tc = ReadTestCaseData(testcase.testcases, testcase.index);

    parser_context_t ctx;
    parser_error_t err;

    // Define mainnet or testnet through derivation path
    hdPath.data[0] = HDPATH_0_DEFAULT;
    hdPath.data[1] = HDPATH_1_DEFAULT;

    if (tc.chainID == "Testnet" || tc.chainID == "Emulator") {
        hdPath.data[0] = HDPATH_0_TESTNET;
        hdPath.data[1] = HDPATH_1_TESTNET;
    }

    app_mode_set_expert(tc.expert);

    show_address = SHOW_ADDRESS_EMPTY_SLOT;

    uint8_t scriptHash[32];
    sha256((const uint8_t *) tc.script.c_str(), tc.script.length(), scriptHash);

    parsed_tx_metadata_t m;
    _parseTxMetadata(scriptHash, tc.metadata.data(), tc.metadata.size(), &m);


    err = parser_parse(&ctx, tc.blob.data(), tc.blob.size());
    if (tc.valid) {
        ASSERT_EQ(err, PARSER_OK) << parser_getErrorDescription(err);
    } else {
        if (err != PARSER_OK)
            return;
    }

    err = parser_validate(&ctx);
    if (tc.valid) {
        EXPECT_EQ(err, PARSER_OK) << parser_getErrorDescription(err);
    } else {
        EXPECT_NE(err, PARSER_OK) << parser_getErrorDescription(err);
        return;
    }

    auto output = dumpUI(&ctx, 40, 40);

    std::cout << std::endl;
    for (const auto &i : output) {
        std::cout << i << std::endl;
    }

    std::cout << " EXPECTED ============" << std::endl;
    for (const auto &i : tc.expected_ui_output) {
        std::cout << i << std::endl;
    }

    EXPECT_EQ(output.size(), tc.expected_ui_output.size());
    for (size_t i = 0; i < tc.expected_ui_output.size(); i++) {
        if (i < output.size()) {
            EXPECT_THAT(output[i], testing::Eq(tc.expected_ui_output[i]));
        }
    }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

class VerifyTestVectors : public ::testing::TestWithParam<testcase_t> {
public:
    struct PrintToStringParamName {
        template<class ParamType>
        std::string operator()(const testing::TestParamInfo<ParamType> &info) const {
            auto p = static_cast<testcase_t>(info.param);
            std::stringstream ss;
            ss << std::setfill('0') << std::setw(5) << p.index << "_" << p.description;
            return ss.str();
        }
    };
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VerifyTestVectors);

INSTANTIATE_TEST_SUITE_P(
        ValidPayloadCases,
        VerifyTestVectors,
        ::testing::ValuesIn(GetJsonTestCases("testvectors/validPayloadCases.json")), VerifyTestVectors::PrintToStringParamName()
);

INSTANTIATE_TEST_SUITE_P(
        InvalidPayloadCases,
        VerifyTestVectors,
        ::testing::ValuesIn(GetJsonTestCases("testvectors/invalidPayloadCases.json")), VerifyTestVectors::PrintToStringParamName()
);


INSTANTIATE_TEST_SUITE_P(
        ValidEnvelopeCases,
        VerifyTestVectors,
        ::testing::ValuesIn(GetJsonTestCases("testvectors/validEnvelopeCases.json")), VerifyTestVectors::PrintToStringParamName()
);

INSTANTIATE_TEST_SUITE_P(
        InvalidEnvelopeCases,
        VerifyTestVectors,
        ::testing::ValuesIn(GetJsonTestCases("testvectors/invalidEnvelopeCases.json")), VerifyTestVectors::PrintToStringParamName()
);

INSTANTIATE_TEST_SUITE_P(
        ManifestEnvelopeCases,
        VerifyTestVectors,
        ::testing::ValuesIn(GetJsonTestCases("testvectors/manifestEnvelopeCases.json")), VerifyTestVectors::PrintToStringParamName()
);

INSTANTIATE_TEST_SUITE_P(
        ManifestPayloadCases,
        VerifyTestVectors,
        ::testing::ValuesIn(GetJsonTestCases("testvectors/manifestPayloadCases.json")), VerifyTestVectors::PrintToStringParamName()
);


TEST_P(VerifyTestVectors, CheckUIOutput_Manual) { check_testcase(GetParam()); }

#pragma clang diagnostic pop
