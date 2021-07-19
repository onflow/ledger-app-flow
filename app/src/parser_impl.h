/*******************************************************************************
*  (c) 2019 Zondax GmbH
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

#include "parser_common.h"
#include "parser_txdef.h"
#include "crypto.h"
#include "jsmn.h"
#include <json/json_parser.h>

#ifdef __cplusplus
extern "C" {
#endif


extern parser_tx_t parser_tx_obj;

parser_error_t parser_init(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize);

parser_error_t _matchScriptType(uint8_t scriptHash[32], script_type_e *scriptType);

parser_error_t _read(parser_context_t *c, parser_tx_t *v);

parser_error_t _validateTx(const parser_context_t *c, const parser_tx_t *v);

uint8_t _getNumItems(const parser_context_t *c, const parser_tx_t *v);

parser_error_t _countArgumentItems(const flow_argument_list_t *v, uint8_t argumentIndex, 
                                   uint8_t max_number_of_items, uint8_t *number_of_items);

parser_error_t _countArgumentOptionalItems(const flow_argument_list_t *v, uint8_t argumentIndex, 
                                           uint8_t max_number_of_items, uint8_t *number_of_items);

parser_error_t json_validateToken(parsed_json_t *parsedJson, uint16_t tokenIdx);

parser_error_t json_extractToken(char *outVal, uint16_t outValLen, parsed_json_t *parsedJson, uint16_t tokenIdx);

parser_error_t json_matchToken(parsed_json_t *parsedJson, uint16_t tokenIdx, char *expectedValue);

parser_error_t json_matchNull(parsed_json_t *parsedJson, uint16_t tokenIdx);

parser_error_t json_matchKeyValue(parsed_json_t *parsedJson,
                                  uint16_t tokenIdx, char *expectedType, jsmntype_t jsonType, uint16_t *valueTokenIdx);

#define JSON_MATCH_VALUE_IDX_NONE 65535
parser_error_t json_matchOptionalKeyValue(parsed_json_t *parsedJson,
                                  uint16_t tokenIdx, char *expectedType, jsmntype_t jsonType, uint16_t *valueTokenIdx);

parser_error_t json_matchOptionalArray(parsed_json_t *parsedJson, uint16_t tokenIdx, uint16_t *valueTokenIdx);

parser_error_t formatStrUInt8AsHex(const char *decStr, char *hexStr);

parser_error_t json_extractString(char *outVal, uint16_t outValLen, parsed_json_t *parsedJson, uint16_t tokenIdx);

#ifdef __cplusplus
}
#endif
