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
#include <stdlib.h>
#include <hexutils.h>
#include <json/json_parser.h>
#include <parser_impl.h>
#include "rlp.h"

const auto sendToken = "{\"type\":\"UFix64\",\"value\":\"545.77\"}";

const auto sendToken2 = "{\"type\":\"Optional\",\"value\":null}";
const auto sendToken3 = "{\"type\":\"Optional\",\"value\":{\"type\":\"UFix64\",\"value\":\"545.77\"}}";

const auto sendToken4 = "{\"type\":\"Optional\",\"value\":{\"type\":\"Array\",\"value\":[{\"type\":\"UFix64\",\"value\":\"545.77\"}]}}";

const auto validCreationInput = "{\n"
                                "  \"type\": \"Array\",\n"
                                "  \"value\": [\n"
                                "    {\n"
                                "      \"type\": \"Array\",\n"
                                "      \"value\": [\n"
                                "        { \"type\": \"UInt8\", \"value\": 233 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 22 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 168 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 152 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 64 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 204 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 194 }\n"
                                "      ]\n"
                                "    },\n"
                                "    {\n"
                                "      \"type\": \"Array\",\n"
                                "      \"value\": [\n"
                                "        { \"type\": \"UInt8\", \"value\": 251 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 245 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 137 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 188 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 58 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 164 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 229 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 174 }\n"
                                "      ]\n"
                                "    },\n"
                                "    {\n"
                                "      \"type\": \"Array\",\n"
                                "      \"value\": [\n"
                                "        { \"type\": \"UInt8\", \"value\": 172 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 68 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 145 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 243 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 154 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 211 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 52 }\n"
                                "      ]\n"
                                "    },\n"
                                "    {\n"
                                "      \"type\": \"Array\",\n"
                                "      \"value\": [\n"
                                "        { \"type\": \"UInt8\", \"value\": 79 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 144 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 18 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 172 },\n"
                                "        { \"type\": \"UInt8\", \"value\": 201 }\n"
                                "      ]\n"
                                "    }\n"
                                "  ]\n"
                                "}";

TEST(JSON, basicSingleKeyValue) {
    parsed_json_t parsedJson = {false};

    auto err = json_parse(&parsedJson, sendToken, strlen(sendToken));

    // We could parse valid JSON
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_TRUE(parsedJson.isValid);
    EXPECT_EQ(5, parsedJson.numberOfTokens);

    EXPECT_EQ(parsedJson.tokens[0].type, jsmntype_t::JSMN_OBJECT);
    EXPECT_EQ(parsedJson.tokens[1].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[2].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[3].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[4].type, jsmntype_t::JSMN_STRING);

    char tmpBuffer[100];

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 1), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "type");
    ASSERT_THAT(json_matchToken(&parsedJson, 1, (char *) "type"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 2), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "UFix64");
    ASSERT_THAT(json_matchToken(&parsedJson, 2, (char *) "UFix64"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 3), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "value");
    ASSERT_THAT(json_matchToken(&parsedJson, 3, (char *) "value"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 4), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "545.77");
    ASSERT_THAT(json_matchToken(&parsedJson, 4, (char *) "545.77"), PARSER_OK);

    uint16_t internalTokenElementIdx;
    ASSERT_THAT(
            json_matchKeyValue(&parsedJson, 0, (char *) "UFix64", JSMN_STRING, &internalTokenElementIdx),
            PARSER_OK);
    ASSERT_THAT(internalTokenElementIdx, 4);
}

TEST(JSON, OptionalKeyValueNull) {
    parsed_json_t parsedJson = {false};

    auto err = json_parse(&parsedJson, sendToken2, strlen(sendToken2));

     // We could parse valid JSON
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_TRUE(parsedJson.isValid);
    EXPECT_EQ(5, parsedJson.numberOfTokens);

    EXPECT_EQ(parsedJson.tokens[0].type, jsmntype_t::JSMN_OBJECT);
    EXPECT_EQ(parsedJson.tokens[1].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[2].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[3].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[4].type, jsmntype_t::JSMN_PRIMITIVE);

    char tmpBuffer[100];

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 1), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "type");
    ASSERT_THAT(json_matchToken(&parsedJson, 1, (char *) "type"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 2), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "Optional");
    ASSERT_THAT(json_matchToken(&parsedJson, 2, (char *) "Optional"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 3), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "value");
    ASSERT_THAT(json_matchToken(&parsedJson, 3, (char *) "value"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 4), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "null");
    ASSERT_THAT(json_matchNull(&parsedJson, 4), PARSER_OK);

    uint16_t internalTokenElementIdx;
    EXPECT_EQ(5, parsedJson.numberOfTokens);
    ASSERT_THAT(
            json_matchOptionalKeyValue(&parsedJson, 0, (char *) "UFix64", JSMN_STRING, &internalTokenElementIdx),
            PARSER_OK);
    ASSERT_THAT(internalTokenElementIdx, JSON_MATCH_VALUE_IDX_NONE);
}


TEST(JSON, OptionalKeyValueNotNull) {
    parsed_json_t parsedJson = {false};

    auto err = json_parse(&parsedJson, sendToken3, strlen(sendToken3));

     // We could parse valid JSON
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_TRUE(parsedJson.isValid);
    EXPECT_EQ(9, parsedJson.numberOfTokens);

    EXPECT_EQ(parsedJson.tokens[0].type, jsmntype_t::JSMN_OBJECT);
    EXPECT_EQ(parsedJson.tokens[1].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[2].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[3].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[4].type, jsmntype_t::JSMN_OBJECT);
    EXPECT_EQ(parsedJson.tokens[5].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[6].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[7].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[8].type, jsmntype_t::JSMN_STRING);

    char tmpBuffer[100];

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 1), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "type");
    ASSERT_THAT(json_matchToken(&parsedJson, 1, (char *) "type"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 2), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "Optional");
    ASSERT_THAT(json_matchToken(&parsedJson, 2, (char *) "Optional"), PARSER_OK);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 3), PARSER_OK);
    EXPECT_STREQ(tmpBuffer, "value");
    ASSERT_THAT(json_matchToken(&parsedJson, 3, (char *) "value"), PARSER_OK);


    uint16_t internalTokenElementIdx;
    ASSERT_THAT(
            json_matchOptionalKeyValue(&parsedJson, 0, (char *) "UFix64", JSMN_STRING, &internalTokenElementIdx),
            PARSER_OK);
    ASSERT_THAT(internalTokenElementIdx, 8);
}

TEST(JSON, OptionalArrayNull) {
    parsed_json_t parsedJson = {false};

    auto err = json_parse(&parsedJson, sendToken2, strlen(sendToken2));

     // We could parse valid JSON

    uint16_t internalTokenElementIdx;
    ASSERT_THAT(
            json_matchOptionalArray(&parsedJson, 0, &internalTokenElementIdx),
            PARSER_OK);
    ASSERT_THAT(internalTokenElementIdx, JSON_MATCH_VALUE_IDX_NONE);
}

TEST(JSON, OptionalArrayNotNull) {
    parsed_json_t parsedJson = {false};

    auto err = json_parse(&parsedJson, sendToken4, strlen(sendToken4));

     // We could parse valid JSON

    uint16_t internalTokenElementIdx;
    ASSERT_THAT(
            json_matchOptionalArray(&parsedJson, 0, &internalTokenElementIdx),
            PARSER_OK);
    ASSERT_THAT(internalTokenElementIdx, 8);
}
