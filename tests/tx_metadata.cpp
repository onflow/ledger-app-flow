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
#include <tx_metadata.h>
#include <string.h>

const uint8_t TX_METADATA_ADD_NEW_KEY[] = {
    1, //number of hashes + hashes
    0x59, 0x5c, 0x86, 0x56, 0x14, 0x41, 0xb3, 0x2b, 0x2b, 0x91, 0xee, 0x03, 0xf9, 0xe1, 0x0c, 0xa6, 0xef, 0xa7, 0xb4, 0x1b, 0xcc, 0x99, 0x4f, 0x51, 0x31, 0x7e, 0xc0, 0xaa, 0x9d, 0x8f, 0x8a, 0x42,
    'A', 'd', 'd', ' ', 'N', 'e', 'w', ' ', 'K', 'e', 'y', 0,  //tx name (to display)
    1,  //number of arguments

    //Argument 1
    ARGUMENT_TYPE_NORMAL,
    'P', 'u', 'b', ' ', 'k', 'e', 'y', 0, //arg name (to display)
    0, //argument index
    'S','t', 'r', 'i', 'n', 'g',  0, //expected value type
    JSMN_STRING //expected value json token type
};

const uint8_t TX_METADATA_TOKEN_TRANSFER[] = {
    3, //number of hashes + hashes
    0xca, 0x80, 0xb6, 0x28, 0xd9, 0x85, 0xb3, 0x58, 0xae, 0x1c, 0xb1, 0x36, 0xbc, 0xd9, 0x76, 0x99, 0x7c, 0x94, 0x2f, 0xa1, 0x0d, 0xba, 0xbf, 0xea, 0xfb, 0x4e, 0x20, 0xfa, 0x66, 0xa5, 0xa5, 0xe2,
    0xd5, 0x6f, 0x4e, 0x1d, 0x23, 0x55, 0xcd, 0xcf, 0xac, 0xfd, 0x01, 0xe4, 0x71, 0x45, 0x9c, 0x6e, 0xf1, 0x68, 0xbf, 0xdf, 0x84, 0x37, 0x1a, 0x68, 0x5c, 0xcf, 0x31, 0xcf, 0x3c, 0xde, 0xdc, 0x2d,
    0x47, 0x85, 0x15, 0x86, 0xd9, 0x62, 0x33, 0x5e, 0x3f, 0x7d, 0x9e, 0x5d, 0x11, 0xa4, 0xc5, 0x27, 0xee, 0x4b, 0x5f, 0xd1, 0xc3, 0x89, 0x5e, 0x3c, 0xe1, 0xb9, 0xc2, 0x82, 0x1f, 0x60, 0xb1, 0x66,
    'T', 'o', 'k', 'e', 'n', ' ', 'T', 'r', 'a', 'n', 's', 'f', 'e', 'r', 0,  //tx name (to display)
    2,  //number of arguments

    //Argument 1
    ARGUMENT_TYPE_OPTIONALARRAY, 5, 10,
    'A', 'm', 'o', 'u', 'n', 't', 0, //arg name (to display)
    0, //argument index
    'U','I', 'n', 't', '6', '4',  0, //expected value type
    JSMN_STRING, //expected value json token type

    //Argument 2
    ARGUMENT_TYPE_NORMAL,
    'D', 'e', 's', 't', 'i', 'n', 'a', 't', 'i', 'o', 'n', 0, //arg name (to display)
    1, //argument index
    'A','d', 'd', 'r', 'e', 's', 's', 0, //expected value type
    JSMN_STRING //expected value json token type
};

uint8_t hashAddNewKey[32]     = {0x59, 0x5c, 0x86, 0x56, 0x14, 0x41, 0xb3, 0x2b, 0x2b, 0x91, 0xee, 0x03, 0xf9, 0xe1, 0x0c, 0xa6, 0xef, 0xa7, 0xb4, 0x1b, 0xcc, 0x99, 0x4f, 0x51, 0x31, 0x7e, 0xc0, 0xaa, 0x9d, 0x8f, 0x8a, 0x42};
uint8_t hashTokenTranfer1[32] = {0xca, 0x80, 0xb6, 0x28, 0xd9, 0x85, 0xb3, 0x58, 0xae, 0x1c, 0xb1, 0x36, 0xbc, 0xd9, 0x76, 0x99, 0x7c, 0x94, 0x2f, 0xa1, 0x0d, 0xba, 0xbf, 0xea, 0xfb, 0x4e, 0x20, 0xfa, 0x66, 0xa5, 0xa5, 0xe2};
uint8_t hashTokenTranfer2[32] = {0xd5, 0x6f, 0x4e, 0x1d, 0x23, 0x55, 0xcd, 0xcf, 0xac, 0xfd, 0x01, 0xe4, 0x71, 0x45, 0x9c, 0x6e, 0xf1, 0x68, 0xbf, 0xdf, 0x84, 0x37, 0x1a, 0x68, 0x5c, 0xcf, 0x31, 0xcf, 0x3c, 0xde, 0xdc, 0x2d};
uint8_t hashTokenTranfer3[32] = {0x47, 0x85, 0x15, 0x86, 0xd9, 0x62, 0x33, 0x5e, 0x3f, 0x7d, 0x9e, 0x5d, 0x11, 0xa4, 0xc5, 0x27, 0xee, 0x4b, 0x5f, 0xd1, 0xc3, 0x89, 0x5e, 0x3c, 0xe1, 0xb9, 0xc2, 0x82, 0x1f, 0x60, 0xb1, 0x66};


TEST(tx_data, validateHash) {
    parser_error_t err;
    err = _validateHash(hashAddNewKey, TX_METADATA_ADD_NEW_KEY, sizeof(TX_METADATA_ADD_NEW_KEY));
    EXPECT_THAT(err, PARSER_OK);

    err = _validateHash(hashTokenTranfer1, TX_METADATA_ADD_NEW_KEY, sizeof(TX_METADATA_ADD_NEW_KEY));
    EXPECT_THAT(err, PARSER_UNEXPECTED_SCRIPT);

    err = _validateHash(hashTokenTranfer1, TX_METADATA_TOKEN_TRANSFER, sizeof(TX_METADATA_TOKEN_TRANSFER));
    EXPECT_THAT(err, PARSER_OK);

    err = _validateHash(hashTokenTranfer2, TX_METADATA_TOKEN_TRANSFER, sizeof(TX_METADATA_TOKEN_TRANSFER));
    EXPECT_THAT(err, PARSER_OK);

    err = _validateHash(hashTokenTranfer3, TX_METADATA_TOKEN_TRANSFER, sizeof(TX_METADATA_TOKEN_TRANSFER));
    EXPECT_THAT(err, PARSER_OK);

    err = _validateHash(hashAddNewKey, TX_METADATA_TOKEN_TRANSFER, sizeof(TX_METADATA_TOKEN_TRANSFER));
    EXPECT_THAT(err, PARSER_UNEXPECTED_SCRIPT);

    err = _validateHash(hashTokenTranfer3, TX_METADATA_ADD_NEW_KEY, 0);
    EXPECT_THAT(err, PARSER_METADATA_ERROR);

    err = _validateHash(hashTokenTranfer3, TX_METADATA_TOKEN_TRANSFER, 3*32);
    EXPECT_THAT(err, PARSER_METADATA_ERROR);

    err = _validateHash(hashTokenTranfer3, TX_METADATA_TOKEN_TRANSFER, 3*32+1);
    EXPECT_THAT(err, PARSER_OK);
}


TEST(tx_data, parseCompressedTxData) {
    parser_error_t err;
    parsed_tx_metadata_t result;
    err = parseTxMetadata(hashTokenTranfer1, TX_METADATA_ADD_NEW_KEY, sizeof(TX_METADATA_ADD_NEW_KEY), &result);
    EXPECT_THAT(err, PARSER_UNEXPECTED_SCRIPT);
    
    err = parseTxMetadata(hashAddNewKey, TX_METADATA_ADD_NEW_KEY, sizeof(TX_METADATA_ADD_NEW_KEY), &result);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_THAT(std::string(result.txName), "Add New Key");
    EXPECT_THAT(result.txNameLength, 11);
    EXPECT_THAT(result.argCount, 1);
    EXPECT_THAT(result.arguments[0].argumentType, ARGUMENT_TYPE_NORMAL);
    EXPECT_THAT(std::string(result.arguments[0].displayKey), "Pub key");
    EXPECT_THAT(result.arguments[0].displayKeyLength, 7);
    EXPECT_THAT(result.arguments[0].argumentIndex, 0);
    EXPECT_THAT(std::string(result.arguments[0].jsonExpectedType), "String");
    EXPECT_THAT(result.arguments[0].jsonExpectedTypeLength, 6);
    EXPECT_THAT(result.arguments[0].jsonExpectedKind, JSMN_STRING);

    err = parseTxMetadata(hashTokenTranfer3, TX_METADATA_TOKEN_TRANSFER, sizeof(TX_METADATA_TOKEN_TRANSFER), &result);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_THAT(std::string(result.txName), "Token Transfer");
    EXPECT_THAT(result.txNameLength, 14);
    EXPECT_THAT(result.argCount, 2);
    EXPECT_THAT(result.arguments[0].argumentType, ARGUMENT_TYPE_OPTIONALARRAY);
    EXPECT_THAT(result.arguments[0].arrayMinElements, 5);
    EXPECT_THAT(result.arguments[0].arrayMaxElements, 10);
    EXPECT_THAT(std::string(result.arguments[0].displayKey), "Amount");
    EXPECT_THAT(result.arguments[0].displayKeyLength, 6);
    EXPECT_THAT(result.arguments[0].argumentIndex, 0);
    EXPECT_THAT(std::string(result.arguments[0].jsonExpectedType), "UInt64");
    EXPECT_THAT(result.arguments[0].jsonExpectedTypeLength, 6);
    EXPECT_THAT(result.arguments[0].jsonExpectedKind, JSMN_STRING);
    EXPECT_THAT(result.arguments[1].argumentType, ARGUMENT_TYPE_NORMAL);
    EXPECT_THAT(std::string(result.arguments[1].displayKey), "Destination");
    EXPECT_THAT(result.arguments[1].displayKeyLength, 11);
    EXPECT_THAT(result.arguments[1].argumentIndex, 1);
    EXPECT_THAT(std::string(result.arguments[1].jsonExpectedType), "Address");
    EXPECT_THAT(result.arguments[1].jsonExpectedTypeLength, 7);
    EXPECT_THAT(result.arguments[1].jsonExpectedKind, JSMN_STRING);    

    err = parseTxMetadata(hashAddNewKey, TX_METADATA_ADD_NEW_KEY, sizeof(TX_METADATA_ADD_NEW_KEY)-1, &result);
    EXPECT_THAT(err, PARSER_METADATA_ERROR);
    err = parseTxMetadata(hashTokenTranfer3, TX_METADATA_TOKEN_TRANSFER, sizeof(TX_METADATA_TOKEN_TRANSFER)+1, &result);
    EXPECT_THAT(err, PARSER_METADATA_ERROR);
}

TEST(tx_data, matchStoreTxMetadata) {
    parser_error_t err;
    parsed_tx_metadata_t result;

    uint8_t hashWrong[32]     = {0x59, 0x5c, 0x86, 0x56, 0x14, 0x41, 0xb3, 0x2b, 0x2b, 0x91, 0xee, 0x03, 0xf9, 0xe1, 0x0c, 0xa6, 0xef, 0xa7, 0xb4, 0x1b, 0xcc, 0x99, 0x4f, 0x51, 0x31, 0x7e, 0xc0, 0xaa, 0x9d, 0x8f, 0x8a, 0x41};
    err = matchStoredTxMetadata(hashWrong, &result);
    EXPECT_THAT(err, PARSER_UNEXPECTED_SCRIPT);

    err = matchStoredTxMetadata(hashAddNewKey, &result);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_THAT(std::string(result.txName), "Add New Key");
    EXPECT_THAT(result.txNameLength, 11);
    EXPECT_THAT(result.argCount, 1);
    EXPECT_THAT(result.arguments[0].argumentType, ARGUMENT_TYPE_NORMAL);
    EXPECT_THAT(std::string(result.arguments[0].displayKey), "Pub key");
    EXPECT_THAT(result.arguments[0].displayKeyLength, 7);
    EXPECT_THAT(result.arguments[0].argumentIndex, 0);
    EXPECT_THAT(std::string(result.arguments[0].jsonExpectedType), "String");
    EXPECT_THAT(result.arguments[0].jsonExpectedTypeLength, 6);
    EXPECT_THAT(result.arguments[0].jsonExpectedKind, JSMN_STRING);

    err = matchStoredTxMetadata(hashTokenTranfer3, &result);
    EXPECT_THAT(err, PARSER_OK);
    EXPECT_THAT(std::string(result.txName), "Token Transfer");
    EXPECT_THAT(result.txNameLength, 14);
    EXPECT_THAT(result.argCount, 2);
    EXPECT_THAT(result.arguments[0].argumentType, ARGUMENT_TYPE_NORMAL);
    EXPECT_THAT(std::string(result.arguments[0].displayKey), "Amount");
    EXPECT_THAT(result.arguments[0].displayKeyLength, 6);
    EXPECT_THAT(result.arguments[0].argumentIndex, 0);
    EXPECT_THAT(std::string(result.arguments[0].jsonExpectedType), "UFix64");
    EXPECT_THAT(result.arguments[0].jsonExpectedTypeLength, 6);
    EXPECT_THAT(result.arguments[0].jsonExpectedKind, JSMN_STRING);
    EXPECT_THAT(result.arguments[1].argumentType, ARGUMENT_TYPE_NORMAL);
    EXPECT_THAT(std::string(result.arguments[1].displayKey), "Destination");
    EXPECT_THAT(result.arguments[1].displayKeyLength, 11);
    EXPECT_THAT(result.arguments[1].argumentIndex, 1);
    EXPECT_THAT(std::string(result.arguments[1].jsonExpectedType), "Address");
    EXPECT_THAT(result.arguments[1].jsonExpectedTypeLength, 7);
    EXPECT_THAT(result.arguments[1].jsonExpectedKind, JSMN_STRING);
}

