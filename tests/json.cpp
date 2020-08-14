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
#include <json/json_parser.h>
#include "rlp.h"

const auto sendToken = "{\"type\":\"UFix64\",\"value\":\"545.77\"}";

parser_error_t json_validateToken(parsed_json_t *parsedJson, uint16_t tokenIdx) {
    if (!parsedJson->isValid) {
        return parser_json_invalid;
    }

    if (tokenIdx >= parsedJson->numberOfTokens) {
        return parser_json_invalid_token_idx;
    }

    const jsmntok_t token = parsedJson->tokens[tokenIdx];
    if (token.start < 0) {
        return parser_json_unexpected_error;
    }

    if (token.end > parsedJson->bufferLen) {
        return parser_unexpected_buffer_end;
    }

    return parser_ok;
}

parser_error_t json_extractToken(char *outVal, uint16_t outValLen, parsed_json_t *parsedJson, uint16_t tokenIdx) {
    MEMZERO(outVal, outValLen);
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    const jsmntok_t token = parsedJson->tokens[tokenIdx];
    if (token.end - token.start > outValLen - 1) {
        return parser_unexpected_buffer_end;
    }


    MEMCPY(outVal, parsedJson->buffer + token.start, token.end - token.start);
    return parser_ok;
}

parser_error_t json_matchToken(parsed_json_t *parsedJson, uint16_t tokenIdx, char *expectedValue) {
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    const jsmntok_t token = parsedJson->tokens[tokenIdx];
    if (token.type != jsmntype_t::JSMN_STRING) {
        return parser_unexpected_type;
    }

    if (strlen(expectedValue) != token.end - token.start) {
        return parser_unexpected_value;
    }

    if (MEMCMP(expectedValue, parsedJson->buffer + token.start, token.end - token.start) != 0) {
        return parser_unexpected_value;
    }

    return parser_ok;
}

parser_error_t json_matchKeyValue(parsed_json_t *parsedJson,
                                  uint16_t tokenIdx, char *expectedType, jsmntype_t jsonType, uint16_t *valueTokenIdx) {
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    if (tokenIdx + 4 >= parsedJson->numberOfTokens) {
        // we need this token a 4 more
        return parser_json_invalid_token_idx;
    }

    if (parsedJson->tokens[tokenIdx].type != jsmntype_t::JSMN_OBJECT) {
        return parser_unexpected_type;
    }

    if (parsedJson->tokens[tokenIdx].size != 2) {
        return parser_unexpected_number_items;
    }

    // Type key/value
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 1, (char *) "type"))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 2, expectedType))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 3, (char *) "value"))
    if (parsedJson->tokens[tokenIdx + 4].type != jsonType) {
        return parser_unexpected_number_items;
    }

    *valueTokenIdx = tokenIdx + 4;

    return parser_ok;
}

parser_error_t
json_extractFormattedPubKey(char *outVal, uint16_t outValLen, parsed_json_t *parsedJson, uint16_t tokenIdx) {
    MEMZERO(outVal, outValLen);
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    uint16_t internalTokenElementIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(parsedJson,
                                        tokenIdx, (char *) "Array", jsmntype_t::JSMN_ARRAY, &internalTokenElementIdx))

    // Extract values

    return parser_ok;
}

TEST(JSON, basicSingleKeyValue) {
    parsed_json_t parsedJson = {false};

    auto err = json_parse(&parsedJson, sendToken, strlen(sendToken));

    // We could parse valid JSON
    EXPECT_THAT(err, parser_ok);
    EXPECT_TRUE(parsedJson.isValid);
    EXPECT_EQ(5, parsedJson.numberOfTokens);

    EXPECT_EQ(parsedJson.tokens[0].type, jsmntype_t::JSMN_OBJECT);
    EXPECT_EQ(parsedJson.tokens[1].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[2].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[3].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[4].type, jsmntype_t::JSMN_STRING);

    char tmpBuffer[100];

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 1), parser_ok);
    EXPECT_STREQ(tmpBuffer, "type");
    ASSERT_THAT(json_matchToken(&parsedJson, 1, (char *) "type"), parser_ok);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 2), parser_ok);
    EXPECT_STREQ(tmpBuffer, "UFix64");
    ASSERT_THAT(json_matchToken(&parsedJson, 2, (char *) "UFix64"), parser_ok);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 3), parser_ok);
    EXPECT_STREQ(tmpBuffer, "value");
    ASSERT_THAT(json_matchToken(&parsedJson, 3, (char *) "value"), parser_ok);

    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 4), parser_ok);
    EXPECT_STREQ(tmpBuffer, "545.77");
    ASSERT_THAT(json_matchToken(&parsedJson, 4, (char *) "545.77"), parser_ok);

    uint16_t internalTokenElementIdx;
    ASSERT_THAT(
            json_matchKeyValue(&parsedJson, 0, (char *) "UFix64", JSMN_STRING, &internalTokenElementIdx),
            parser_ok);
    ASSERT_THAT(internalTokenElementIdx, 4);
}

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

TEST(JSON, basicArrayOfArrays) {
    parsed_json_t parsedJson = {false};

    auto err = json_parse(&parsedJson, validCreationInput, strlen(validCreationInput));

    // We could parse valid JSON
    EXPECT_THAT(err, parser_ok);
    EXPECT_TRUE(parsedJson.isValid);
    EXPECT_EQ(160, parsedJson.numberOfTokens);

    EXPECT_EQ(parsedJson.tokens[0].type, jsmntype_t::JSMN_OBJECT);
    EXPECT_EQ(parsedJson.tokens[0].size, 2);    // type and value
    // Type key/value
    EXPECT_EQ(parsedJson.tokens[1].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[2].type, jsmntype_t::JSMN_STRING);
    // Value key/value
    EXPECT_EQ(parsedJson.tokens[3].type, jsmntype_t::JSMN_STRING);
    EXPECT_EQ(parsedJson.tokens[4].type, jsmntype_t::JSMN_ARRAY);

    char tmpBuffer[100];

    // Check first group
    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 1), parser_ok);
    EXPECT_STREQ(tmpBuffer, "type");
    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 2), parser_ok);
    EXPECT_STREQ(tmpBuffer, "Array");
    ASSERT_THAT(json_extractToken(tmpBuffer, sizeof(tmpBuffer), &parsedJson, 3), parser_ok);
    EXPECT_STREQ(tmpBuffer, "value");

    const uint16_t arrayTokenIdx = 4;
    uint16_t arrayTokenCount;
    ASSERT_THAT(array_get_element_count(&parsedJson, arrayTokenIdx, &arrayTokenCount), parser_ok);
    EXPECT_EQ(arrayTokenCount, 4);

    for (int arrayElementIdx = 0; arrayElementIdx < 4; arrayElementIdx++) {
        uint16_t arrayElementToken;
        ASSERT_THAT(array_get_nth_element(&parsedJson, arrayTokenIdx, arrayElementIdx, &arrayElementToken),
                    parser_ok);
        EXPECT_EQ(parsedJson.tokens[arrayElementToken].type, jsmntype_t::JSMN_OBJECT);

        ASSERT_THAT(json_extractFormattedPubKey(tmpBuffer, sizeof(tmpBuffer), &parsedJson, arrayElementToken),
                    parser_ok);
        fprintf(stderr, "%s\n", tmpBuffer);
    }
}
