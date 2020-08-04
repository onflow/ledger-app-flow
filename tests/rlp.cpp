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

INSTANTIATE_TEST_SUITE_P(
        InstantiationName,
        RLPDecodeTest,
        Values(
                RLPValueTestCase{"00", kind_byte, 0, 0, 1},
                RLPValueTestCase{"01", kind_byte, 0, 0, 1},
                RLPValueTestCase{"7F", kind_byte, 0, 0, 1},

                RLPValueTestCase{"80", kind_string, 0, 1, 1},
                RLPValueTestCase{"B7", kind_string, 55, 1, 56},
                RLPValueTestCase{"B90400", kind_string, 1024, 3, 1027},

                RLPValueTestCase{"C0", kind_list, 0, 1, 1},
                RLPValueTestCase{"C8", kind_list, 8, 1, 9},
                RLPValueTestCase{"F7", kind_list, 55, 1, 56},
                RLPValueTestCase{"F90400", kind_list, 1024, 3, 1027}
        )
);

TEST_P(RLPDecodeTest, decodeElement) {
    auto params = GetParam();

    uint8_t data[100];
    parseHexString(data, sizeof(data), params.data);

    rlp_kind_e kind;
    uint16_t len;
    uint16_t dataOffset;

    uint16_t bytesConsumed;

    parser_error_t err = rlp_decode(data, &kind, &len, &dataOffset, &bytesConsumed);
    EXPECT_THAT(err, parser_ok);
    EXPECT_THAT(kind, testing::Eq(params.expectedKind));
    EXPECT_THAT(len, testing::Eq(params.expectedLen));
    EXPECT_THAT(dataOffset, testing::Eq(params.expectedDataOffset));
    EXPECT_THAT(bytesConsumed, testing::Eq(params.expectedConsumed));
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

struct RLPStreamTestCase {
    const char *name;
    const char *data;
    uint16_t expectedFieldCount;
    const char *expectedFields;
    const char *expectedValueStr;
};

class RLPStreamParamTest : public testing::TestWithParam<RLPStreamTestCase> {
public:
    struct PrintToStringParamName {
        std::string operator()(const ::testing::TestParamInfo<RLPStreamTestCase> &info) const {
            std::stringstream ss;
            ss << info.index << "_" << info.param.name;
            return ss.str();
        }
    };
};

INSTANTIATE_TEST_SUITE_P(
        InstantiationName,
        RLPStreamParamTest,
        Values(
                RLPStreamTestCase{"byte_5", "05", 1, "BYTE@0[0|0]", "BYTE=5"},
                RLPStreamTestCase{"byte_6", "06", 1, "BYTE@0[0|0]", "BYTE=6"},
                RLPStreamTestCase{"string_empty", "80", 1, "STRING@0[1|0]", "STRING="},
                RLPStreamTestCase{"string", "820505", 1, "STRING@0[1|2]", "STRING=\x5\x5"},
                RLPStreamTestCase{"string", "83050505", 1, "STRING@0[1|3]", "STRING=\x5\x5\x5"},
                RLPStreamTestCase{"string", "8405050505", 1, "STRING@0[1|4]", "STRING=\x5\x5\x5\x5"},
                RLPStreamTestCase{"string", "850505050505", 1, "STRING@0[1|5]", "STRING=\x5\x5\x5\x5\x5"},
                RLPStreamTestCase{"string", "8D6162636465666768696A6B6C6D", 1, "STRING@0[1|13]",
                                  "STRING=abcdefghijklm"},
                RLPStreamTestCase{"string", "820505820505", 2, "STRING@0[1|2]STRING@3[1|2]", "STRING=\x5\x5"},
                // [5, '444']
                RLPStreamTestCase{"list", "C50583343434", 1, "LIST@0[1|5]", "LIST=BYTE@1[0|0]STRING@2[1|3]"},
                // [1, [2, [3, ]]]
                RLPStreamTestCase{"list", "C601C402C203C0", 1, "LIST@0[1|6]", "LIST=BYTE@1[0|0]LIST@2[1|4]"}
        ),
        RLPStreamParamTest::PrintToStringParamName()
);

std::string getKind(uint8_t kind) {
    switch (kind) {
        case kind_byte:
            return "BYTE";
        case kind_string:
            return "STRING";
        case kind_list:
            return "LIST";
        default:
            return "?????";
    }
}

std::string dumpRLPFields(rlp_field_t *fields, uint8_t fieldCount) {
    std::stringstream ss;
    for (int i = 0; i < fieldCount; i++) {
        rlp_field_t *f = fields + i;
        ss << getKind(f->kind) << "@";
        ss << f->fieldOffset << "[" << f->valueOffset << "|" << f->valueLen << "]";
    }
    return ss.str();
};

TEST_P(RLPStreamParamTest, stream) {
    auto params = GetParam();

    uint8_t data[10000];
    uint64_t dataSize = parseHexString(data, sizeof(data), params.data);

    rlp_field_t fields[32];
    uint16_t fieldCount;
    auto err = rlp_parseStream(data, 0, dataSize, fields, 32, &fieldCount);
    EXPECT_THAT(err, testing::Eq(parser_ok));
    EXPECT_THAT(fieldCount, testing::Eq(params.expectedFieldCount));

    auto s = dumpRLPFields(fields, fieldCount);
    std::cout << s << std::endl;
    EXPECT_THAT(s, testing::Eq(params.expectedFields));
}

TEST(RLPStreamParamTest, readStringPaging0) {
    uint8_t data[10000];
    const char *input = "8D6162636465666768696A6B6C6D";
    uint64_t dataSize = parseHexString(data, sizeof(data), input);

    rlp_field_t fields[2];
    uint16_t fieldCount;
    auto err = rlp_parseStream(data, 0, dataSize, fields, 2, &fieldCount);
    EXPECT_THAT(err, testing::Eq(parser_ok));
    EXPECT_THAT(fieldCount, testing::Eq(1));

    std::stringstream ss;

    // "abcdefghijklm"

    char value[6];
    uint8_t numPages;

    for (int i = 0; i < 3; i++) {
        uint16_t valueLen;
        uint16_t maxValueLen = sizeof(value);
        err = rlp_readStringPaging(data, fields + 0, value, maxValueLen, &valueLen, i, &numPages);
        EXPECT_THAT(err, testing::Eq(parser_ok));

        if (i == 2) {
            EXPECT_THAT(valueLen, testing::Eq(3));
        } else {
            EXPECT_THAT(valueLen, testing::Eq(5));
        }

        EXPECT_THAT(numPages, testing::Eq(3));
        ss << "[" << i << "]" << value;
    }

    EXPECT_THAT(ss.str(), testing::StrEq("[0]abcde[1]fghij[2]klm"));
}

TEST(RLPStreamParamTest, readStringPaging1) {
    uint8_t data[10000];
    const char *input = "8E6162636465666768696A6B6C6D6E";
    uint64_t dataSize = parseHexString(data, sizeof(data), input);

    rlp_field_t fields[2];
    uint16_t fieldCount;
    auto err = rlp_parseStream(data, 0,
                               dataSize, fields,
                               2, &fieldCount);
    EXPECT_THAT(err, testing::Eq(parser_ok));
    EXPECT_THAT(fieldCount, testing::Eq(1));

    std::stringstream ss;

    // "abcdefghijklm"

    char value[8];
    uint8_t numPages;

    for (int i = 0; i < 2; i++) {
        uint16_t valueLen;
        err = rlp_readStringPaging(data, fields + 0,
                                   value, sizeof(value), &valueLen,
                                   i, &numPages);
        EXPECT_THAT(err, testing::Eq(parser_ok));
        EXPECT_THAT(numPages, testing::Eq(2));
        EXPECT_THAT(valueLen, testing::Eq(7));
        ss << "[" << i << "]" << value;
    }

    EXPECT_THAT(ss.str(), testing::StrEq("[0]abcdefg[1]hijklmn"));
}

TEST_P(RLPStreamParamTest, streamReadValues) {
    auto params = GetParam();

    uint8_t data[10000];
    uint64_t dataSize = parseHexString(data, sizeof(data), params.data);

    rlp_field_t fields[32];
    uint16_t fieldCount;
    auto err = rlp_parseStream(data, 0, dataSize, fields, 32, &fieldCount);
    EXPECT_THAT(err, testing::Eq(parser_ok));
    EXPECT_THAT(fieldCount, testing::Eq(params.expectedFieldCount));

    for (int i = 0; i < fieldCount; i++) {
        std::stringstream ss;
        auto currentField = fields + i;

        ss << getKind(fields[i].kind) << "=";

        switch (fields[i].kind) {
            case kind_byte: {
                uint8_t value;
                err = rlp_readByte(data, currentField, &value);
                EXPECT_THAT(err, testing::Eq(parser_ok));
                ss << (int) value;
                break;
            }
            case kind_string: {
                char value[1000];
                err = rlp_readString(data, currentField, value, sizeof(value));
                EXPECT_THAT(err, testing::Eq(parser_ok));
                ss << (char *) value;
                break;
            }
            case kind_list: {
                // When a list is found, we can use the value as a stream again
                // no need to extract the data
                rlp_field_t listFields[32];
                uint16_t listFieldCount;

                err = rlp_readList(data, currentField, listFields, 32, &listFieldCount);
                EXPECT_THAT(err, testing::Eq(parser_ok));
                EXPECT_THAT(fieldCount, testing::Eq(params.expectedFieldCount));

                ss << dumpRLPFields(listFields, listFieldCount);
                break;
            }
        }

        EXPECT_THAT(ss.str(), testing::Eq(params.expectedValueStr));
    }
}
