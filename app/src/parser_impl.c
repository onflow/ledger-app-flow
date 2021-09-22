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

#include <zxmacros.h>
#include <json/json_parser.h>
#include "parser_impl.h"
#include "parser_txdef.h"
#include "app_mode.h"
#include "rlp.h"

parser_tx_t parser_tx_obj;

#define CHECK_KIND(KIND, EXPECTED_KIND) \
    if (KIND != EXPECTED_KIND) { return PARSER_RLP_ERROR_INVALID_KIND; }

parser_error_t parser_init_context(parser_context_t *ctx,
                                   const uint8_t *buffer,
                                   uint16_t bufferSize) {
    ctx->offset = 0;
    ctx->buffer = NULL;
    ctx->bufferLen = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        return PARSER_INIT_CONTEXT_EMPTY;
    }

    ctx->buffer = buffer;
    ctx->bufferLen = bufferSize;
    return PARSER_OK;
}

parser_error_t parser_init(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
    CHECK_PARSER_ERR(parser_init_context(ctx, buffer, bufferSize))
    return PARSER_OK;
}

const char *parser_getErrorDescription(parser_error_t err) {
    switch (err) {
        // General errors
        case PARSER_OK:
            return "No error";
        case PARSER_NO_DATA:
            return "No more data";
        case PARSER_INIT_CONTEXT_EMPTY:
            return "Initialized empty context";
        case PARSER_DISPLAY_IDX_OUT_OF_RANGE:
            return "display_idx_out_of_range";
        case PARSER_DISPLAY_PAGE_OUT_OF_RANGE:
            return "display_page_out_of_range";
        case PARSER_UNEXPECTED_ERROR:
            return "Unexepected internal error";
            // Coin specific
        case PARSER_RLP_ERROR_INVALID_KIND:
            return "parser_rlp_error_invalid_kind";
        case PARSER_RLP_ERROR_INVALID_VALUE_LEN:
            return "parser_rlp_error_invalid_value_len";
        case PARSER_RLP_ERROR_INVALID_FIELD_OFFSET:
            return "parser_rlp_error_invalid_field_offset";
        case PARSER_RLP_ERROR_BUFFER_TOO_SMALL:
            return "parser_rlp_error_buffer_too_small";
        case PARSER_RLP_ERROR_INVALID_PAGE:
            return "parser_rlp_error_invalid_page";
        case PARSER_UNEXPECTED_TX_VERSION:
            return "tx version is not supported";
        case PARSER_UNEXPECTED_TYPE:
            return "Unexpected data type";
        case PARSER_UNEXPECTED_SCRIPT:
            return "Unexpected script";
        case PARSER_UNEXPECTED_METHOD:
            return "Unexpected method";
        case PARSER_UNEXPECTED_BUFFER_END:
            return "Unexpected buffer end";
        case PARSER_UNEXPECTED_VALUE:
            return "Unexpected value";
        case PARSER_UNEXPECTED_NUMBER_ITEMS:
            return "Unexpected number of items";
        case PARSER_UNEXPECTED_CHARACTERS:
            return "Unexpected characters";
        case PARSER_UNEXPECTED_FIELD:
            return "Unexpected field";
        case PARSER_VALUE_OUT_OF_RANGE:
            return "Value out of range";
        case PARSER_INVALID_ADDRESS:
            return "Invalid address format";
            /////////// Context specific
        case PARSER_CONTEXT_MISMATCH:
            return "context prefix is invalid";
        case PARSER_CONTEXT_UNEXPECTED_SIZE:
            return "context unexpected size";
        case PARSER_CONTEXT_INVALID_CHARS:
            return "context invalid chars";
            // Required fields error
        case PARSER_REQUIRED_NONCE:
            return "Required field nonce";
        case PARSER_REQUIRED_METHOD:
            return "Required field method";
        default:
            return "Unrecognized error code";
    }
}

__Z_INLINE char hexDigit(uint8_t v) {
    if (v < 10) {
        return (char) ('0' + v);
    }
    if (v < 16) {
        return (char) ('a' + v - 10);
    }
    return '?';
}

parser_error_t json_validateToken(parsed_json_t *parsedJson, uint16_t tokenIdx) {
    if (!parsedJson->isValid) {
        return PARSER_JSON_INVALID;
    }

    if (!(tokenIdx < parsedJson->numberOfTokens)) {
        return PARSER_JSON_INVALID_TOKEN_IDX;
    }

    const jsmntok_t token = parsedJson->tokens[tokenIdx];
    if (token.start < 0) {
        return PARSER_JSON_UNEXPECTED_ERROR;
    }

    if (token.end > parsedJson->bufferLen) {
        return PARSER_UNEXPECTED_BUFFER_END;
    }

    return PARSER_OK;
}

parser_error_t json_extractToken(char *outVal, uint16_t outValLen, parsed_json_t *parsedJson, uint16_t tokenIdx) {
    MEMZERO(outVal, outValLen);
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    const jsmntok_t token = parsedJson->tokens[tokenIdx];
    if (token.end - token.start > outValLen - 1) {
        return PARSER_UNEXPECTED_BUFFER_END;
    }

    MEMCPY(outVal, parsedJson->buffer + token.start, token.end - token.start);
    return PARSER_OK;
}

parser_error_t json_matchToken(parsed_json_t *parsedJson, uint16_t tokenIdx, char *expectedValue) {
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    const jsmntok_t token = parsedJson->tokens[tokenIdx];
    if (token.type != JSMN_STRING) {
        return PARSER_UNEXPECTED_TYPE;
    }

    if (token.end < token.start || strlen(expectedValue) != (size_t)(token.end - token.start)) {
        return PARSER_UNEXPECTED_VALUE;
    }

    if (MEMCMP(expectedValue, parsedJson->buffer + token.start, token.end - token.start) != 0) {
        return PARSER_UNEXPECTED_VALUE;
    }

    return PARSER_OK;
}

parser_error_t json_matchNull(parsed_json_t *parsedJson, uint16_t tokenIdx) {
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    const jsmntok_t token = parsedJson->tokens[tokenIdx];
    if (token.type != JSMN_PRIMITIVE) {
        return PARSER_UNEXPECTED_TYPE;
    }

    if (token.end < token.start || 4 != (size_t)(token.end - token.start)) {
        return PARSER_UNEXPECTED_VALUE;
    }

    if (MEMCMP("null", parsedJson->buffer + token.start, token.end - token.start) != 0) {
        return PARSER_UNEXPECTED_VALUE;
    }

    return PARSER_OK;
}

parser_error_t json_matchKeyValue(parsed_json_t *parsedJson,
                                  uint16_t tokenIdx, char *expectedType, jsmntype_t jsonType, uint16_t *valueTokenIdx) {
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    if (! (tokenIdx + 4 < parsedJson->numberOfTokens)) {
        // we need this token and 4 more
        return PARSER_JSON_INVALID_TOKEN_IDX;
    }

    if (parsedJson->tokens[tokenIdx].type != JSMN_OBJECT) {
        return PARSER_UNEXPECTED_TYPE;
    }

    if (parsedJson->tokens[tokenIdx].size != 2) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    // Type key/value
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 1, (char *) "type"))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 2, expectedType))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 3, (char *) "value"))
    if (parsedJson->tokens[tokenIdx + 4].type != jsonType) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    *valueTokenIdx = tokenIdx + 4;

    return PARSER_OK;
}

//valueTokenIdx is JSON_MATCH_VALUE_IDX_NONE if the optional is null
parser_error_t json_matchOptionalKeyValue(parsed_json_t *parsedJson,
                                  uint16_t tokenIdx, char *expectedType, jsmntype_t jsonType, uint16_t *valueTokenIdx) {
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    if (!(tokenIdx + 4 < parsedJson->numberOfTokens)) {
        // we need this token and 4 more.
        return PARSER_JSON_INVALID_TOKEN_IDX;
    }

    if (parsedJson->tokens[tokenIdx].type != JSMN_OBJECT) {
        return PARSER_UNEXPECTED_TYPE;
    }

    if (parsedJson->tokens[tokenIdx].size != 2) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    // Type key/value
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 1, (char *) "type"))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 2, "Optional"))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 3, (char *) "value"))
    if (parsedJson->tokens[tokenIdx + 4].type == JSMN_PRIMITIVE) {  //optional null
        CHECK_PARSER_ERR(json_matchNull(parsedJson, tokenIdx + 4))
        *valueTokenIdx = JSON_MATCH_VALUE_IDX_NONE;
        return PARSER_OK; 
    }
    
    if (parsedJson->tokens[tokenIdx + 4].type == JSMN_OBJECT) {  //optional not not null
        return json_matchKeyValue(parsedJson, tokenIdx+4, expectedType, jsonType, valueTokenIdx);          
    }
     
    return PARSER_UNEXPECTED_VALUE;
}

//valueTokenIdx is JSON_MATCH_VALUE_IDX_NONE if the optional is null
parser_error_t json_matchOptionalArray(parsed_json_t *parsedJson,
                                  uint16_t tokenIdx, uint16_t *valueTokenIdx) {
    CHECK_PARSER_ERR(json_validateToken(parsedJson, tokenIdx))

    if (!(tokenIdx + 4 < parsedJson->numberOfTokens)) {
        // we need this token and 4 more
        return PARSER_JSON_INVALID_TOKEN_IDX;
    }

    if (parsedJson->tokens[tokenIdx].type != JSMN_OBJECT) {
        return PARSER_UNEXPECTED_TYPE ;
    }

    if (parsedJson->tokens[tokenIdx].size != 2) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    // Type key/value
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 1, (char *) "type"))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 2, (char *) "Optional"))
    CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 3, (char *) "value"))
    if (parsedJson->tokens[tokenIdx + 4].type == JSMN_PRIMITIVE) {  //optional null
        CHECK_PARSER_ERR(json_matchNull(parsedJson, tokenIdx + 4))
        *valueTokenIdx = JSON_MATCH_VALUE_IDX_NONE;
        return PARSER_OK; 
    }    
    if (parsedJson->tokens[tokenIdx + 4].type == JSMN_OBJECT) {  //optional not null
        if (!(tokenIdx + 8 < parsedJson->numberOfTokens)) {
            return PARSER_JSON_INVALID_TOKEN_IDX;
        }
        CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 5, (char *) "type"))
        CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 6, (char *) "Array"))
        CHECK_PARSER_ERR(json_matchToken(parsedJson, tokenIdx + 7, (char *) "value"))
        if (parsedJson->tokens[tokenIdx + 8].type == JSMN_ARRAY) {
            *valueTokenIdx = tokenIdx + 8;
            return PARSER_OK; 
        }
    }
     
    return PARSER_UNEXPECTED_VALUE;
}

parser_error_t formatStrUInt8AsHex(const char *decStr, char *hexStr) {
    uint16_t decLen = strnlen(decStr, 5);
    if (decLen > 3 || decLen == 0) {
        return PARSER_UNEXPECTED_BUFFER_END;
    }

    uint16_t v = 0;
    uint16_t m = 1;
    for (int16_t i = decLen - 1; i >= 0; i--) {
        if (decStr[i] < '0' || decStr[i] > '9') {
            return PARSER_UNEXPECTED_VALUE;
        }
        v += (decStr[i] - '0') * m;
        m *= 10;
    }

    hexStr[0] = hexDigit(v / 16);
    hexStr[1] = hexDigit(v % 16);
    hexStr[2] = 0;
    return PARSER_OK;
}

parser_error_t json_extractString(char *outVal, uint16_t outValLen, parsed_json_t *parsedJson, uint16_t tokenIdx) {
    MEMZERO(outVal, outValLen);

    uint16_t internalTokenElemIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(
            parsedJson, tokenIdx, (char *) "String", JSMN_STRING, &internalTokenElemIdx))

    CHECK_PARSER_ERR(json_extractToken(outVal, outValLen, parsedJson, internalTokenElemIdx))

    return PARSER_OK;
}

parser_error_t _matchScriptType(uint8_t scriptHash[32], script_type_e *scriptType) {
    *scriptType = SCRIPT_UNKNOWN;

    char buffer[100];
    MEMZERO(buffer, sizeof(buffer));

    // Check it is a known script digest
    if (array_to_hexstr(buffer, sizeof(buffer), scriptHash, CX_SHA256_SIZE) != 64) {
        return PARSER_UNEXPECTED_ERROR;
    }

    struct known_script_entry {
        script_type_e script_type;
        char* template;
    };

    const struct known_script_entry KNOWN_TYPES[] = {
        {SCRIPT_TOKEN_TRANSFER, TEMPLATE_HASH_TOKEN_TRANSFER_EMULATOR},
        {SCRIPT_TOKEN_TRANSFER, TEMPLATE_HASH_TOKEN_TRANSFER_TESTNET},
        {SCRIPT_TOKEN_TRANSFER, TEMPLATE_HASH_TOKEN_TRANSFER_MAINNET},

        {SCRIPT_CREATE_ACCOUNT, TEMPLATE_HASH_CREATE_ACCOUNT},

        {SCRIPT_ADD_NEW_KEY, TEMPLATE_HASH_ADD_NEW_KEY},
        
        {SCRIPT_TH01_WITHDRAW_UNLOCKED_TOKENS, TEMPLATE_HASH_TH01_WITHDRAW_UNLOCKED_TOKENS_TESTNET},
        {SCRIPT_TH01_WITHDRAW_UNLOCKED_TOKENS, TEMPLATE_HASH_TH01_WITHDRAW_UNLOCKED_TOKENS_MAINNET},

        {SCRIPT_TH02_DEPOSIT_UNLOCKED_TOKENS, TEMPLATE_HASH_TH02_DEPOSIT_UNLOCKED_TOKENS_TESTNET},
        {SCRIPT_TH02_DEPOSIT_UNLOCKED_TOKENS, TEMPLATE_HASH_TH02_DEPOSIT_UNLOCKED_TOKENS_MAINNET},

        {SCRIPT_TH06_REGISTER_NODE, TEMPLATE_HASH_TH06_REGISTER_NODE_TESTNET},
        {SCRIPT_TH06_REGISTER_NODE, TEMPLATE_HASH_TH06_REGISTER_NODE_MAINNET},

        {SCRIPT_TH08_STAKE_NEW_TOKENS, TEMPLATE_HASH_TH08_STAKE_NEW_TOKENS_TESTNET},
        {SCRIPT_TH08_STAKE_NEW_TOKENS, TEMPLATE_HASH_TH08_STAKE_NEW_TOKENS_MAINNET},

        {SCRIPT_TH09_RESTAKE_UNSTAKED_TOKENS, TEMPLATE_HASH_TH09_RESTAKE_UNSTAKED_TOKENS_TESTNET},
        {SCRIPT_TH09_RESTAKE_UNSTAKED_TOKENS, TEMPLATE_HASH_TH09_RESTAKE_UNSTAKED_TOKENS_MAINNET},
        
        {SCRIPT_TH10_RESTAKE_REWARDED_TOKENS, TEMPLATE_HASH_TH10_RESTAKE_REWARDED_TOKENS_TESTNET},
        {SCRIPT_TH10_RESTAKE_REWARDED_TOKENS, TEMPLATE_HASH_TH10_RESTAKE_REWARDED_TOKENS_MAINNET},

        {SCRIPT_TH11_UNSTAKE_TOKENS, TEMPLATE_HASH_TH11_UNSTAKE_TOKENS_TESTNET},
        {SCRIPT_TH11_UNSTAKE_TOKENS, TEMPLATE_HASH_TH11_UNSTAKE_TOKENS_MAINNET},

        {SCRIPT_TH12_UNSTAKE_ALL_TOKENS, TEMPLATE_HASH_TH12_UNSTAKE_ALL_TOKENS_TESTNET},
        {SCRIPT_TH12_UNSTAKE_ALL_TOKENS, TEMPLATE_HASH_TH12_UNSTAKE_ALL_TOKENS_MAINNET},

        {SCRIPT_TH13_WITHDRAW_UNSTAKED_TOKENS, TEMPLATE_HASH_TH13_WITHDRAW_UNSTAKED_TOKENS_TESTNET},
        {SCRIPT_TH13_WITHDRAW_UNSTAKED_TOKENS, TEMPLATE_HASH_TH13_WITHDRAW_UNSTAKED_TOKENS_MAINNET},

        {SCRIPT_TH14_WITHDRAW_REWARDED_TOKENS, TEMPLATE_HASH_TH14_WITHDRAW_REWARDED_TOKENS_TESTNET},
        {SCRIPT_TH14_WITHDRAW_REWARDED_TOKENS, TEMPLATE_HASH_TH14_WITHDRAW_REWARDED_TOKENS_MAINNET},

        {SCRIPT_TH16_REGISTER_OPERATOR_NODE, TEMPLATE_HASH_TH16_REGISTER_OPERATOR_NODE_TESTNET},
        {SCRIPT_TH16_REGISTER_OPERATOR_NODE, TEMPLATE_HASH_TH16_REGISTER_OPERATOR_NODE_MAINNET},

        {SCRIPT_TH17_REGISTER_DELEGATOR, TEMPLATE_HASH_TH17_REGISTER_DELEGATOR_TESTNET},
        {SCRIPT_TH17_REGISTER_DELEGATOR, TEMPLATE_HASH_TH17_REGISTER_DELEGATOR_MAINNET},

        {SCRIPT_TH19_DELEGATE_NEW_TOKENS, TEMPLATE_HASH_TH19_DELEGATE_NEW_TOKENS_TESTNET},
        {SCRIPT_TH19_DELEGATE_NEW_TOKENS, TEMPLATE_HASH_TH19_DELEGATE_NEW_TOKENS_MAINNET},

        {SCRIPT_TH20_RESTAKE_UNSTAKED_DELEGATED_TOKENS, TEMPLATE_HASH_TH20_RESTAKE_UNSTAKED_DELEGATED_TOKENS_TESTNET},
        {SCRIPT_TH20_RESTAKE_UNSTAKED_DELEGATED_TOKENS, TEMPLATE_HASH_TH20_RESTAKE_UNSTAKED_DELEGATED_TOKENS_MAINNET},

        {SCRIPT_TH21_RESTAKE_REWARDED_DELEGATED_TOKENS, TEMPLATE_HASH_TH21_RESTAKE_REWARDED_DELEGATED_TOKENS_TESTNET},
        {SCRIPT_TH21_RESTAKE_REWARDED_DELEGATED_TOKENS, TEMPLATE_HASH_TH21_RESTAKE_REWARDED_DELEGATED_TOKENS_MAINNET},

        {SCRIPT_TH22_UNSTAKE_DELEGATED_TOKENS, TEMPLATE_HASH_TH22_UNSTAKE_DELEGATED_TOKENS_TESTNET},
        {SCRIPT_TH22_UNSTAKE_DELEGATED_TOKENS, TEMPLATE_HASH_TH22_UNSTAKE_DELEGATED_TOKENS_MAINNET},

        {SCRIPT_TH23_WITHDRAW_UNSTAKED_DELEGATED_TOKENS, TEMPLATE_HASH_TH23_WITHDRAW_UNSTAKED_DELEGATED_TOKENS_TESTNET},
        {SCRIPT_TH23_WITHDRAW_UNSTAKED_DELEGATED_TOKENS, TEMPLATE_HASH_TH23_WITHDRAW_UNSTAKED_DELEGATED_TOKENS_MAINNET},

        {SCRIPT_TH24_WITHDRAW_REWARDED_DELEGATED_TOKENS, TEMPLATE_HASH_TH24_WITHDRAW_REWARDED_DELEGATED_TOKENS_TESTNET},
        {SCRIPT_TH24_WITHDRAW_REWARDED_DELEGATED_TOKENS, TEMPLATE_HASH_TH24_WITHDRAW_REWARDED_DELEGATED_TOKENS_MAINNET},

        {SCRIPT_TH25_UPDATE_NETWORKING_ADDRESS, TEMPLATE_HASH_TH25_UPDATE_NETWORKING_ADDRESS_TESTNET},
        {SCRIPT_TH25_UPDATE_NETWORKING_ADDRESS, TEMPLATE_HASH_TH25_UPDATE_NETWORKING_ADDRESS_MAINNET},

        {SCRIPT_SCO01_SETUP_STAKING_COLLECTION, TEMPLATE_HASH_SCO01_SETUP_STAKING_COLLECTION_TESTNET},
        {SCRIPT_SCO01_SETUP_STAKING_COLLECTION, TEMPLATE_HASH_SCO01_SETUP_STAKING_COLLECTION_MAINNET},

        {SCRIPT_SCO02_REGISTER_DELEGATOR, TEMPLATE_HASH_SCO02_REGISTER_DELEGATOR_TESTNET},
        {SCRIPT_SCO02_REGISTER_DELEGATOR, TEMPLATE_HASH_SCO02_REGISTER_DELEGATOR_MAINNET},

        {SCRIPT_SCO03_REGISTER_NODE, TEMPLATE_HASH_SCO03_REGISTER_NODE_TESTNET},
        {SCRIPT_SCO03_REGISTER_NODE, TEMPLATE_HASH_SCO03_REGISTER_NODE_MAINNET},

        {SCRIPT_SCO04_CREATE_MACHINE_ACCOUNT, TEMPLATE_HASH_SCO04_CREATE_MACHINE_ACCOUNT_TESTNET},
        {SCRIPT_SCO04_CREATE_MACHINE_ACCOUNT, TEMPLATE_HASH_SCO04_CREATE_MACHINE_ACCOUNT_MAINNET},

        {SCRIPT_SCO05_REQUEST_UNSTAKING, TEMPLATE_HASH_SCO05_REQUEST_UNSTAKING_TESTNET},
        {SCRIPT_SCO05_REQUEST_UNSTAKING, TEMPLATE_HASH_SCO05_REQUEST_UNSTAKING_MAINNET},

        {SCRIPT_SCO06_STAKE_NEW_TOKENS, TEMPLATE_HASH_SCO06_STAKE_NEW_TOKENS_TESTNET},
        {SCRIPT_SCO06_STAKE_NEW_TOKENS, TEMPLATE_HASH_SCO06_STAKE_NEW_TOKENS_MAINNET},

        {SCRIPT_SCO07_STAKE_REWARD_TOKENS, TEMPLATE_HASH_SCO07_STAKE_REWARD_TOKENS_TESTNET},
        {SCRIPT_SCO07_STAKE_REWARD_TOKENS, TEMPLATE_HASH_SCO07_STAKE_REWARD_TOKENS_MAINNET},

        {SCRIPT_SCO08_STAKE_UNSTAKED_TOKENS, TEMPLATE_HASH_SCO08_STAKE_UNSTAKED_TOKENS_TESTNET},
        {SCRIPT_SCO08_STAKE_UNSTAKED_TOKENS, TEMPLATE_HASH_SCO08_STAKE_UNSTAKED_TOKENS_MAINNET},

        {SCRIPT_SCO09_UNSTAKE_ALL, TEMPLATE_HASH_SCO09_UNSTAKE_ALL_TESTNET},
        {SCRIPT_SCO09_UNSTAKE_ALL, TEMPLATE_HASH_SCO09_UNSTAKE_ALL_MAINNET},

        {SCRIPT_SCO10_WITHDRAW_REWARD_TOKENS, TEMPLATE_HASH_SCO10_WITHDRAW_REWARD_TOKENS_TESTNET},
        {SCRIPT_SCO10_WITHDRAW_REWARD_TOKENS, TEMPLATE_HASH_SCO10_WITHDRAW_REWARD_TOKENS_MAINNET},

        {SCRIPT_SCO11_WITHDRAW_UNSTAKED_TOKENS, TEMPLATE_HASH_SCO11_WITHDRAW_UNSTAKED_TOKENS_TESTNET},
        {SCRIPT_SCO11_WITHDRAW_UNSTAKED_TOKENS, TEMPLATE_HASH_SCO11_WITHDRAW_UNSTAKED_TOKENS_MAINNET},

        {SCRIPT_SCO12_CLOSE_STAKE, TEMPLATE_HASH_SCO12_CLOSE_STAKE_TESTNET},
        {SCRIPT_SCO12_CLOSE_STAKE, TEMPLATE_HASH_SCO12_CLOSE_STAKE_MAINNET},

        {SCRIPT_SCO13_TRANSFER_NODE, TEMPLATE_HASH_SCO13_TRANSFER_NODE_TESTNET},
        {SCRIPT_SCO13_TRANSFER_NODE, TEMPLATE_HASH_SCO13_TRANSFER_NODE_MAINNET},

        {SCRIPT_SCO14_TRANSFER_DELEGATOR, TEMPLATE_HASH_SCO14_TRANSFER_DELEGATOR_TESTNET},
        {SCRIPT_SCO14_TRANSFER_DELEGATOR, TEMPLATE_HASH_SCO14_TRANSFER_DELEGATOR_MAINNET},

        {SCRIPT_SCO15_WITHDRAW_FROM_MACHINE_ACCOUNT, TEMPLATE_HASH_SCO15_WITHDRAW_FROM_MACHINE_ACCOUNT_TESTNET},
        {SCRIPT_SCO15_WITHDRAW_FROM_MACHINE_ACCOUNT, TEMPLATE_HASH_SCO15_WITHDRAW_FROM_MACHINE_ACCOUNT_MAINNET},

        {SCRIPT_SCO16_UPDATE_NETWORKING_ADDRESS, TEMPLATE_HASH_SCO16_UPDATE_NETWORKING_ADDRESS_TESTNET},
        {SCRIPT_SCO16_UPDATE_NETWORKING_ADDRESS, TEMPLATE_HASH_SCO16_UPDATE_NETWORKING_ADDRESS_MAINNET},

        {SCRIPT_FUSD01_SETUP_FUSD_VAULT, TEMPLATE_HASH_FUSD01_SETUP_FUSD_VAULT_TESTNET},
        {SCRIPT_FUSD01_SETUP_FUSD_VAULT, TEMPLATE_HASH_FUSD01_SETUP_FUSD_VAULT_MAINNET},

        {SCRIPT_FUSD02_TRANSFER_FUSD, TEMPLATE_HASH_FUSD02_TRANSFER_FUSD_TESTNET},
        {SCRIPT_FUSD02_TRANSFER_FUSD, TEMPLATE_HASH_FUSD02_TRANSFER_FUSD_MAINNET},

        {SCRIPT_TS01_SET_UP_TOPSHOT_COLLECTION, TEMPLATE_HASH_TS01_SET_UP_TOPSHOT_COLLECTION_TESTNET},
        {SCRIPT_TS01_SET_UP_TOPSHOT_COLLECTION, TEMPLATE_HASH_TS01_SET_UP_TOPSHOT_COLLECTION_MAINNET},

        {SCRIPT_TS02_TRANSFER_TOP_SHOT_MOMENT, TEMPLATE_HASH_TS02_TRANSFER_TOP_SHOT_MOMENT_TESTNET},
        {SCRIPT_TS02_TRANSFER_TOP_SHOT_MOMENT, TEMPLATE_HASH_TS02_TRANSFER_TOP_SHOT_MOMENT_MAINNET},
        // sentinel, do not remove
        {0, NULL}
    };

    int i = 0;
    while (KNOWN_TYPES[i].template) {
        if (MEMCMP(PIC(KNOWN_TYPES[i].template), buffer, 64) == 0) {
            *scriptType = KNOWN_TYPES[i].script_type;
            return PARSER_OK;
        }
        i++;
    }

    return PARSER_UNEXPECTED_SCRIPT;
}

parser_error_t _readScript(parser_context_t *c, flow_script_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_script_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)

    MEMZERO(v->digest, sizeof(v->digest));
    sha256(v->ctx.buffer, v->ctx.bufferLen, v->digest);

    CHECK_PARSER_ERR(_matchScriptType(v->digest, &v->type))

    return PARSER_OK;
}

parser_error_t _readArguments(parser_context_t *c, flow_argument_list_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_argument_list_t));

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_LIST)

    v->argCount = 0;
    while (v->ctx.offset < v->ctx.bufferLen && v->argCount < PARSER_MAX_ARGCOUNT) {
        CHECK_PARSER_ERR(rlp_decode(&v->ctx, &v->argCtx[v->argCount], &kind, &bytesConsumed))
        CTX_CHECK_AND_ADVANCE(&v->ctx, bytesConsumed)
        CHECK_KIND(kind, RLP_KIND_STRING)
        v->argCount++;
    }
    v->ctx.offset = 0;
    if (v->argCount >= PARSER_MAX_ARGCOUNT) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    return PARSER_OK;
}

parser_error_t _readReferenceBlockId(parser_context_t *c, flow_reference_block_id_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_reference_block_id_t));

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)
    return PARSER_OK;
}

parser_error_t _readGasLimit(parser_context_t *c, flow_gaslimit_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;
    parser_context_t ctx_local;

    MEMZERO(v, sizeof(flow_gaslimit_t));

    CHECK_PARSER_ERR(rlp_decode(c, &ctx_local, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)

    CHECK_PARSER_ERR(rlp_readUInt64(&ctx_local, kind, v))

    return PARSER_OK;
}

parser_error_t _readProposalKeyAddress(parser_context_t *c, flow_proposal_key_address_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_proposal_key_address_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)
    return PARSER_OK;
}

parser_error_t _readProposalKeyId(parser_context_t *c, flow_proposal_keyid_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;
    parser_context_t ctx_local;

    MEMZERO(v, sizeof(flow_proposal_keyid_t));

    CHECK_PARSER_ERR(rlp_decode(c, &ctx_local, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)

    CHECK_PARSER_ERR(rlp_readUInt64(&ctx_local, kind, v))

    return PARSER_OK;
}

parser_error_t _readProposalKeySequenceNumber(parser_context_t *c, flow_proposal_key_sequence_number_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;
    parser_context_t ctx_local;

    MEMZERO(v, sizeof(flow_proposal_key_sequence_number_t));

    CHECK_PARSER_ERR(rlp_decode(c, &ctx_local, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)

    CHECK_PARSER_ERR(rlp_readUInt64(&ctx_local, kind, v))

    return PARSER_OK;
}

parser_error_t _readPayer(parser_context_t *c, flow_payer_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_payer_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)
    return PARSER_OK;
}

parser_error_t _readProposalAuthorizer(parser_context_t *c, flow_proposal_authorizer_t *v) {

    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_proposal_authorizer_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_STRING)

    return PARSER_OK;
}

parser_error_t _readProposalAuthorizers(parser_context_t *c, flow_proposal_authorizers_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_proposal_authorizers_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_LIST)

    while (v->ctx.offset < v->ctx.bufferLen) {
        if (v->authorizer_count >= 16) {
           return PARSER_UNEXPECTED_NUMBER_ITEMS;
        }

        CHECK_PARSER_ERR(_readProposalAuthorizer(&v->ctx, &v->authorizer[v->authorizer_count]))
        
        v->authorizer_count++;
    }
    v->ctx.offset = 0;

    return PARSER_OK;
}

parser_error_t _read(parser_context_t *c, parser_tx_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    parser_context_t ctx_rootList;
    parser_context_t ctx_rootInnerList;

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(c, &ctx_rootList, &kind, &bytesConsumed))
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_LIST)
    if (bytesConsumed != c->bufferLen) {
        // root list should consume the complete buffer
        return PARSER_UNEXPECTED_BUFFER_END;
    }

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(&ctx_rootList, &ctx_rootInnerList, &kind, &bytesConsumed))
    CTX_CHECK_AND_ADVANCE(&ctx_rootList, bytesConsumed)
    CHECK_KIND(kind, RLP_KIND_LIST)

    // Go through the inner list
    CHECK_PARSER_ERR(_readScript(&ctx_rootInnerList, &v->script))
    CHECK_PARSER_ERR(_readArguments(&ctx_rootInnerList, &v->arguments))
    CHECK_PARSER_ERR(_readReferenceBlockId(&ctx_rootInnerList, &v->referenceBlockId))
    CHECK_PARSER_ERR(_readGasLimit(&ctx_rootInnerList, &v->gasLimit))
    CHECK_PARSER_ERR(_readProposalKeyAddress(&ctx_rootInnerList, &v->proposalKeyAddress))
    CHECK_PARSER_ERR(_readProposalKeyId(&ctx_rootInnerList, &v->proposalKeyId))
    CHECK_PARSER_ERR(_readProposalKeySequenceNumber(&ctx_rootInnerList, &v->proposalKeySequenceNumber))
    CHECK_PARSER_ERR(_readPayer(&ctx_rootInnerList, &v->payer))
    CHECK_PARSER_ERR(_readProposalAuthorizers(&ctx_rootInnerList, &v->authorizers))

    if (ctx_rootInnerList.offset != ctx_rootInnerList.bufferLen) {
        // ctx_rootInnerList should be consumed completely
        return PARSER_UNEXPECTED_BUFFER_END;
    }

    // Check last item? signers?
    // TODO: Do we want to show signers too?
    // TODO: confirm that things are not completed
    return PARSER_OK;
}

parser_error_t _validateTx(const parser_context_t *c, const parser_tx_t *v) {
    // Placeholder to run any coin specific validation
    return PARSER_OK;
}

parser_error_t _countArgumentItems(const flow_argument_list_t *v, uint8_t argumentIndex, 
                                   uint8_t max_number_of_items, uint8_t *number_of_items) {
    *number_of_items = 0;
    parsed_json_t parsedJson = {false};

    if (argumentIndex >= v->argCount) {
        return PARSER_UNEXPECTED_FIELD;
    }

    const parser_context_t argCtx = v->argCtx[argumentIndex];
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argCtx.buffer, argCtx.bufferLen));

    // Get number of items
    uint16_t internalTokenElementIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, 0, (char *) "Array", JSMN_ARRAY, &internalTokenElementIdx));
    uint16_t arrayTokenCount;
    CHECK_PARSER_ERR(array_get_element_count(&parsedJson, internalTokenElementIdx, &arrayTokenCount));
    if (arrayTokenCount > max_number_of_items) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    *number_of_items = arrayTokenCount;
    return PARSER_OK;
}

//if Optional is null, number_of_items is set to 1 as one screen is needed to dispay "None"
parser_error_t _countArgumentOptionalItems(const flow_argument_list_t *v, uint8_t argumentIndex, 
                                           uint8_t max_number_of_items, uint8_t *number_of_items) {
    *number_of_items = 0;
    parsed_json_t parsedJson = {false};

    if (argumentIndex >= v->argCount) {
        return PARSER_UNEXPECTED_FIELD;
    }

    const parser_context_t argCtx = v->argCtx[argumentIndex];
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argCtx.buffer, argCtx.bufferLen));

    uint16_t internalTokenElementIdx;
    CHECK_PARSER_ERR(json_matchOptionalArray(&parsedJson, 0, &internalTokenElementIdx));
    if (internalTokenElementIdx == JSON_MATCH_VALUE_IDX_NONE) {
        *number_of_items = 1;
        return PARSER_OK;
    }
    
    // Get numnber of items
    uint16_t arrayTokenCount;
    CHECK_PARSER_ERR(array_get_element_count(&parsedJson, internalTokenElementIdx, &arrayTokenCount));
    if (arrayTokenCount > max_number_of_items) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    *number_of_items = arrayTokenCount;
    return PARSER_OK;
}

parser_error_t _getNumItems(const parser_context_t *c, const parser_tx_t *v, uint8_t *numItems) {
    uint8_t argArrayLength = 0;
    switch (v->script.type) {
        case SCRIPT_TOKEN_TRANSFER:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_CREATE_ACCOUNT:
            //array length is checked while we are parsing it        
            CHECK_PARSER_ERR(_countArgumentItems(&v->arguments, 0, UINT8_MAX, &argArrayLength))
            *numItems = 8 + argArrayLength + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_ADD_NEW_KEY:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH01_WITHDRAW_UNLOCKED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH02_DEPOSIT_UNLOCKED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH06_REGISTER_NODE:
            *numItems = 14 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH08_STAKE_NEW_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH09_RESTAKE_UNSTAKED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH10_RESTAKE_REWARDED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH11_UNSTAKE_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH12_UNSTAKE_ALL_TOKENS:
            *numItems = 8 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH13_WITHDRAW_UNSTAKED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH14_WITHDRAW_REWARDED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH16_REGISTER_OPERATOR_NODE:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH17_REGISTER_DELEGATOR:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH19_DELEGATE_NEW_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH20_RESTAKE_UNSTAKED_DELEGATED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH21_RESTAKE_REWARDED_DELEGATED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH22_UNSTAKE_DELEGATED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH23_WITHDRAW_UNSTAKED_DELEGATED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH24_WITHDRAW_REWARDED_DELEGATED_TOKENS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TH25_UPDATE_NETWORKING_ADDRESS:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO01_SETUP_STAKING_COLLECTION:
            *numItems = 8 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO02_REGISTER_DELEGATOR:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO03_REGISTER_NODE:
            //array length is checked while we are parsing it
            CHECK_PARSER_ERR(_countArgumentOptionalItems(&v->arguments, 6, UINT8_MAX, &argArrayLength)); 
            *numItems = 14 + argArrayLength + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO04_CREATE_MACHINE_ACCOUNT:
            //array length is checked while we are parsing it
            CHECK_PARSER_ERR(_countArgumentItems(&v->arguments, 1, UINT8_MAX, &argArrayLength)) 
            *numItems = 9 + argArrayLength + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO05_REQUEST_UNSTAKING:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO06_STAKE_NEW_TOKENS:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO07_STAKE_REWARD_TOKENS:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO08_STAKE_UNSTAKED_TOKENS:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO09_UNSTAKE_ALL:
            *numItems = 9 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO10_WITHDRAW_REWARD_TOKENS:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO11_WITHDRAW_UNSTAKED_TOKENS:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO12_CLOSE_STAKE:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO13_TRANSFER_NODE:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO14_TRANSFER_DELEGATOR:
            *numItems = 11 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO15_WITHDRAW_FROM_MACHINE_ACCOUNT:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_SCO16_UPDATE_NETWORKING_ADDRESS:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_FUSD01_SETUP_FUSD_VAULT:
            *numItems = 8 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_FUSD02_TRANSFER_FUSD:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TS01_SET_UP_TOPSHOT_COLLECTION:
            *numItems = 8 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_TS02_TRANSFER_TOP_SHOT_MOMENT:
            *numItems = 10 + v->authorizers.authorizer_count;
            return PARSER_OK;
        case SCRIPT_UNKNOWN:
        default:
            return PARSER_UNEXPECTED_SCRIPT;
    }
}
