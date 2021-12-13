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
#include <parser.h>
#include <string.h>


const auto token2 = "{\"type\":\"Optional\",\"value\":null}";
parser_context_t context2 = {(const uint8_t *)token2, (uint16_t)(strlen(token2)), 0};
const auto token3 = "{\"type\":\"Optional\",\"value\":{\"type\":\"UFix64\",\"value\":\"545.77\"}}";
parser_context_t context3 = {(const uint8_t *)token3, (uint16_t)(strlen(token3)), 0};

const auto token4 = "{\"type\":\"Optional\",\"value\":{\"type\": \"Array\",\"value\":"
                         "[{\"type\":\"String\",\"value\":\"f845b8406e4f43f79d3c1d8cacb3d5f3e7aeedb29feaeb4559fdb71a97e2fd0438565310e87670035d83bc10fe67fe314dba5363c81654595d64884b1ecad1512a64e65e020164\"}]}}";
parser_context_t context4 = {(const uint8_t *)token4, (uint16_t)(strlen(token4)), 0};
const auto token5 = "{\"type\":\"Optional\",\"value\":{\"type\": \"Array\",\"value\":"
                        "[{\"type\":\"String\",\"value\":\"e845b8406e4f43f79d3c1d8cacb3d5f3e7aeedb29feaeb4559fdb71a97e2fd0438565310e87670035d83bc10fe67fe314dba5363c81654595d64884b1ecad1512a64e65e020164\"},"
                         "{\"type\":\"String\",\"value\":\"d845b8406e4f43f79d3c1d8cacb3d5f3e7aeedb29feaeb4559fdb71a97e2fd0438565310e87670035d83bc10fe67fe314dba5363c81654595d64884b1ecad1512a64e65e020164\"}]}}";
parser_context_t context5 = {(const uint8_t *)token5, (uint16_t)(strlen(token5)), 0};

flow_argument_list_t arg_list = {{},{context2, context3, context4, context5}, 4};



TEST(parser, printOptionalDelegatorID) {
    char outValBuf[40];
    uint8_t pageCountVar = 0;

    char ufix64[] = "UFix64";
    parser_error_t err = parser_printArgumentOptionalDelegatorID(&arg_list, 0, ufix64, JSMN_STRING,
                                               outValBuf, 40, 0, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_THAT(pageCountVar, 1);
    EXPECT_STREQ(outValBuf, "None");

    err = parser_printArgumentOptionalDelegatorID(&arg_list, 1, ufix64, JSMN_STRING,
                                               outValBuf, 40, 0, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "545.77");
    EXPECT_THAT(pageCountVar, 1);
}


TEST(parser, printOptionalPublicKeys) {
    char outValBuf[40];
    uint8_t pageCountVar = 0;

    parser_error_t err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[0], 0,
                                               outValBuf, 40, 0, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_THAT(pageCountVar, 1);
    EXPECT_STREQ(outValBuf, "None");

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[2], 0,
                                               outValBuf, 40, 0, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "f845b8406e4f43f79d3c1d8cacb3d5f3e7aeedb");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[2], 0,
                                               outValBuf, 40, 1, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "29feaeb4559fdb71a97e2fd0438565310e87670");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[2], 0,
                                               outValBuf, 40, 2, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "035d83bc10fe67fe314dba5363c81654595d648");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[2], 0,
                                               outValBuf, 40, 3, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "84b1ecad1512a64e65e020164");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 0,
                                               outValBuf, 40, 0, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "e845b8406e4f43f79d3c1d8cacb3d5f3e7aeedb");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 0,
                                               outValBuf, 40, 1, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "29feaeb4559fdb71a97e2fd0438565310e87670");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 0,
                                               outValBuf, 40, 2, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "035d83bc10fe67fe314dba5363c81654595d648");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 0,
                                               outValBuf, 40, 3, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "84b1ecad1512a64e65e020164");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 1,
                                               outValBuf, 40, 0, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "d845b8406e4f43f79d3c1d8cacb3d5f3e7aeedb");
    EXPECT_THAT(pageCountVar, 4);
    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 1,
                                               outValBuf, 40, 1, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "29feaeb4559fdb71a97e2fd0438565310e87670");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 1,
                                               outValBuf, 40, 2, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "035d83bc10fe67fe314dba5363c81654595d648");
    EXPECT_THAT(pageCountVar, 4);

    err = parser_printArgumentOptionalPublicKeys(&arg_list.argCtx[3], 1,
                                               outValBuf, 40, 3, &pageCountVar);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_STREQ(outValBuf, "84b1ecad1512a64e65e020164");
    EXPECT_THAT(pageCountVar, 4);
}

