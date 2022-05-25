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
#include <parser_txdef.h>
#include <parser_common.h>
#include <jsmn.h>

typedef enum {
    ARGUMENT_TYPE_NORMAL = 1,
    ARGUMENT_TYPE_OPTIONAL = 2,
    ARGUMENT_TYPE_ARRAY = 3,
    ARGUMENT_TYPE_OPTIONALARRAY = 4
} parsed_tx_template_argument_type_e;

#define MAX_TEMPLATE_NUMBER_OF_HASHES 10
#define MAX_TEMPLATE_STRING_LENGTH 100

//It is planed that all these strings may be on flash, thus they are volatile (for NanoX and NanoSPlus)
typedef struct {
    parsed_tx_template_argument_type_e argumentType;
    const NV_VOLATILE char *displayKey; 
    uint8_t displayKeyLength;
    uint8_t argumentIndex; //argument index within transaction
    const NV_VOLATILE char *jsonExpectedType; //pointer to null terminated string
    uint8_t jsonExpectedTypeLength;
    jsmntype_t jsonExpectedKind;
} parsed_tx_template_argument_t;

typedef struct {
    script_type_e script;
    const NV_VOLATILE char *txName;
    uint8_t txNameLength;
    uint8_t argCount;
    parsed_tx_template_argument_t arguments[PARSER_MAX_ARGCOUNT]; //order of arguments in which they should be displayed
} parsed_tx_template_t;


#define SCRIPT_HASH_SIZE 32

//It is planed that compressedData may be on flash, thus they are volatile (for NanoX and NanoSPlus)
parser_error_t parseCompressedTxData(uint8_t scriptHash[SCRIPT_HASH_SIZE], const NV_VOLATILE uint8_t *compressedData, uint16_t compressedDataLen, 
                                     parsed_tx_template_t *parsedTxTempate);

parser_error_t matchStoredCompressedTxData(uint8_t scriptHash[SCRIPT_HASH_SIZE], parsed_tx_template_t *parsedTxTempate);

#ifdef __cplusplus

//For C++ unit tests
parser_error_t _validateHash(uint8_t scriptHash[SCRIPT_HASH_SIZE], const NV_VOLATILE uint8_t *compressedData, uint16_t compressedDataLen);
}
#endif
