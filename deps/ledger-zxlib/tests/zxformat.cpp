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
#include <gmock/gmock.h>
#include <zxformat.h>
#include <zxmacros.h>

namespace {
TEST(FORMAT, nothingAdded) {
    char buffer[10];

    snprintf(buffer, sizeof(buffer), "TEST");
    auto prefix = std::string("");

    EXPECT_EQ(zxerr_ok, z_str3join(buffer, sizeof(buffer), prefix.c_str(), ""));
    EXPECT_STREQ(buffer, "TEST");
}

TEST(FORMAT, noPrefix) {
    char buffer[10];

    snprintf(buffer, sizeof(buffer), "TEST");
    auto prefix = std::string("");
    auto suffix = std::string("1");

    EXPECT_EQ(zxerr_ok, z_str3join(buffer, sizeof(buffer), prefix.c_str(), suffix.c_str()));
    EXPECT_STREQ(buffer, "TEST1");
}

TEST(FORMAT, prefixSimple) {
    char buffer[10];

    snprintf(buffer, sizeof(buffer), "TEST");
    auto prefix = std::string("xyz ");
    auto suffix = std::string("1");

    EXPECT_EQ(zxerr_ok, z_str3join(buffer, sizeof(buffer), prefix.c_str(), suffix.c_str()));
    EXPECT_STREQ(buffer, "xyz TEST1");
}

TEST(FORMAT, limitBuffer0) {
    char buffer[10];

    snprintf(buffer, sizeof(buffer), "TEST");
    auto prefix = std::string("xyz");
    auto suffix = std::string("4");

    EXPECT_EQ(zxerr_ok, z_str3join(buffer, sizeof(buffer), prefix.c_str(), suffix.c_str()));
    EXPECT_STREQ(buffer, "xyzTEST4");
}

TEST(FORMAT, limitBuffer1) {
    char buffer[9];

    snprintf(buffer, sizeof(buffer), "TEST");
    auto prefix = std::string("xyz");
    auto suffix = std::string("4");

    EXPECT_EQ(zxerr_ok, z_str3join(buffer, sizeof(buffer), prefix.c_str(), suffix.c_str()));
    EXPECT_STREQ(buffer, "xyzTEST4");
}

TEST(FORMAT, limitBuffer2) {
    char buffer[8];

    snprintf(buffer, sizeof(buffer), "TEST");
    auto prefix = std::string("xyz");
    auto suffix = std::string("4");

    EXPECT_EQ(zxerr_buffer_too_small, z_str3join(buffer, sizeof(buffer), prefix.c_str(), suffix.c_str()));
    EXPECT_STREQ(buffer, "ERR???");
}

TEST(FORMAT, smallBuffer) {
    char buffer[7];

    snprintf(buffer, sizeof(buffer), "TEST");
    auto prefix = std::string("xyz");
    auto suffix = std::string("4");

    EXPECT_EQ(zxerr_buffer_too_small, z_str3join(buffer, sizeof(buffer), prefix.c_str(), suffix.c_str()));
    EXPECT_STREQ(buffer, "ERR???");
}

TEST(FORMAT, array2hexstr_1) {
    uint8_t s[] = {0x12, 0x34, 0x56, 0x78};
    char data[10];

    auto length = array_to_hexstr(data, sizeof(data), s, sizeof(s));

    EXPECT_STREQ(data, "12345678");
}

TEST(FORMAT, array2hexstr_2) {
    uint8_t s[] = {0xFA, 0x5D, 0x34, 0x87};
    char data[10];

    auto length = array_to_hexstr_uppercase(data, sizeof(data), s, sizeof(s));

    EXPECT_STREQ(data, "FA5D3487");
}

TEST(FORMAT, array2hexstr_small_buffer) {
    uint8_t s[] = {0xFA, 0x5D, 0x34, 0x87};
    char data[4];

    EXPECT_EQ(0, array_to_hexstr(data, sizeof(data), s, sizeof(s)));
}

TEST(FORMAT, hexstr2array_1) {
    char s[] = "12345678";
    uint8_t data[20];

    auto length = hexstr_to_array(data, sizeof(data), s, strlen(s));

    ASSERT_THAT(data[0], testing::Eq(0x12));
    ASSERT_THAT(data[1], testing::Eq(0x34));
    ASSERT_THAT(data[2], testing::Eq(0x56));
    ASSERT_THAT(data[3], testing::Eq(0x78));
}

TEST(FORMAT, hexstr2array_2) {
    char s[] = "FA2345DE";
    uint8_t data[10];

    auto length = hexstr_to_array(data, sizeof(data), s, strlen(s));

    ASSERT_THAT(data[0], testing::Eq(0xFA));
    ASSERT_THAT(data[1], testing::Eq(0x23));
    ASSERT_THAT(data[2], testing::Eq(0x45));
    ASSERT_THAT(data[3], testing::Eq(0xDE));
}

TEST(FORMAT, hexstr2array_odd_value) {
    char s[] = "FA2345DEA";
    uint8_t data[10];

    EXPECT_EQ(0, hexstr_to_array(data, sizeof(data), s, strlen(s)));
}
}  // namespace
