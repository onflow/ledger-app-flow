/*******************************************************************************
*   (c) 2022 Vacuumlabs
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <parser_common.h>
#include <jsmn.h>
#include <zxmacros.h>

typedef enum {
    ARGUMENT_TYPE_NORMAL = 1,
    ARGUMENT_TYPE_OPTIONAL = 2,
    ARGUMENT_TYPE_ARRAY = 3,
    ARGUMENT_TYPE_OPTIONALARRAY = 4
} argument_type_e;

#define PARSER_MAX_ARGCOUNT 10
#define METADATA_HASH_SIZE 32 //CX_SHA256_SIZE
#define MAX_METADATA_MAX_ARRAY_ITEMS 20

//It is planed that all these strings may be on flash, thus they are volatile (for NanoX and NanoSPlus)
typedef struct {
    argument_type_e argumentType;
    uint8_t arrayMinElements; //defined only for ARGUMENT_TYPE_ARRAY and ARGUMENT_TYPE_OPTIONALARRAY
    uint8_t arrayMaxElements; //defined only for ARGUMENT_TYPE_ARRAY and ARGUMENT_TYPE_OPTIONALARRAY
    const char *displayKey; 
    uint8_t displayKeyLength;
    uint8_t argumentIndex; //argument index within transaction
    const char *jsonExpectedType; //pointer to null terminated string
    uint8_t jsonExpectedTypeLength;
    jsmntype_t jsonExpectedKind;
} parsed_tx_metadata_argument_t;

typedef struct {
    const char *txName;
    uint8_t txNameLength;
    uint8_t argCount;
    parsed_tx_metadata_argument_t arguments[PARSER_MAX_ARGCOUNT]; //order of arguments in which they should be displayed
} parsed_tx_metadata_t;


void initStoredTxMetadata();

parser_error_t storeTxMetadata(const uint8_t *txMetadata, uint16_t txMetadataLength);

parser_error_t validateStoredTxMetadataMerkleTreeLevel(const  uint8_t* hashes, size_t hashesLen);

parser_error_t parseTxMetadata(const uint8_t scriptHash[METADATA_HASH_SIZE], parsed_tx_metadata_t *parsedTxMetadata);

#ifdef __cplusplus
//For C++ unit tests
parser_error_t _parseTxMetadata(const uint8_t scriptHash[METADATA_HASH_SIZE], const uint8_t *metadata, size_t metadataLen, 
                                parsed_tx_metadata_t *parsedTxMetadata);
parser_error_t _validateHash(const uint8_t scriptHash[METADATA_HASH_SIZE], const uint8_t *txMetadata, uint16_t txMetadataLength);
} //end extern C
#endif
