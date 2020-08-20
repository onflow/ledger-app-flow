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
    if (KIND != EXPECTED_KIND) { return parser_rlp_error_invalid_kind; }

parser_error_t parser_init_context(parser_context_t *ctx,
                                   const uint8_t *buffer,
                                   uint16_t bufferSize) {
    ctx->offset = 0;
    ctx->buffer = NULL;
    ctx->bufferLen = 0;

    if (bufferSize == 0 || buffer == NULL) {
        // Not available, use defaults
        return parser_init_context_empty;
    }

    ctx->buffer = buffer;
    ctx->bufferLen = bufferSize;
    return parser_ok;
}

parser_error_t parser_init(parser_context_t *ctx, const uint8_t *buffer, uint16_t bufferSize) {
    CHECK_PARSER_ERR(parser_init_context(ctx, buffer, bufferSize))
    return parser_ok;
}

const char *parser_getErrorDescription(parser_error_t err) {
    switch (err) {
        // General errors
        case parser_ok:
            return "No error";
        case parser_no_data:
            return "No more data";
        case parser_init_context_empty:
            return "Initialized empty context";
        case parser_display_idx_out_of_range:
            return "display_idx_out_of_range";
        case parser_display_page_out_of_range:
            return "display_page_out_of_range";
        case parser_unexpected_error:
            return "Unexepected internal error";
            // Coin specific
        case parser_rlp_error_invalid_kind:
            return "parser_rlp_error_invalid_kind";
        case parser_rlp_error_invalid_value_len:
            return "parser_rlp_error_invalid_value_len";
        case parser_rlp_error_invalid_field_offset:
            return "parser_rlp_error_invalid_field_offset";
        case parser_rlp_error_buffer_too_small:
            return "parser_rlp_error_buffer_too_small";
        case parser_rlp_error_invalid_page:
            return "parser_rlp_error_invalid_page";
        case parser_unexpected_tx_version:
            return "tx version is not supported";
        case parser_unexpected_type:
            return "Unexpected data type";
        case parser_unexpected_script:
            return "Unexpected script";
        case parser_unexpected_method:
            return "Unexpected method";
        case parser_unexpected_buffer_end:
            return "Unexpected buffer end";
        case parser_unexpected_value:
            return "Unexpected value";
        case parser_unexpected_number_items:
            return "Unexpected number of items";
        case parser_unexpected_characters:
            return "Unexpected characters";
        case parser_unexpected_field:
            return "Unexpected field";
        case parser_value_out_of_range:
            return "Value out of range";
        case parser_invalid_address:
            return "Invalid address format";
            /////////// Context specific
        case parser_context_mismatch:
            return "context prefix is invalid";
        case parser_context_unexpected_size:
            return "context unexpected size";
        case parser_context_invalid_chars:
            return "context invalid chars";
            // Required fields error
        case parser_required_nonce:
            return "Required field nonce";
        case parser_required_method:
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
    if (token.type != JSMN_STRING) {
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

    if (parsedJson->tokens[tokenIdx].type != JSMN_OBJECT) {
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

parser_error_t formatStrUInt8AsHex(const char *decStr, char *hexStr) {
    uint16_t decLen = strnlen(decStr, 5);
    if (decLen > 3 || decLen == 0) {
        return parser_unexpected_buffer_end;
    }

    uint16_t v = 0;
    uint16_t m = 1;
    for (int16_t i = decLen - 1; i >= 0; i--) {
        if (decStr[i] < '0' || decStr[i] > '9') {
            return parser_unexpected_value;
        }
        v += (decStr[i] - '0') * m;
        m *= 10;
    }

    hexStr[0] = hexDigit(v / 16);
    hexStr[1] = hexDigit(v % 16);
    hexStr[2] = 0;
    return parser_ok;
}

parser_error_t json_extractPubKey(char *outVal, uint16_t outValLen, parsed_json_t *parsedJson, uint16_t tokenIdx) {
    MEMZERO(outVal, outValLen);

    uint16_t internalTokenElemIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(
            parsedJson, tokenIdx, (char *) "Array", JSMN_ARRAY, &internalTokenElemIdx))

    // Extract values
    uint16_t arrayTokenCount;
    CHECK_PARSER_ERR(array_get_element_count(parsedJson, internalTokenElemIdx, &arrayTokenCount))

    if (outValLen - 1 < arrayTokenCount * 2) {
        return parser_unexpected_buffer_end;
    }

    for (int arrayElementIdx = 0; arrayElementIdx < arrayTokenCount; arrayElementIdx++) {
        uint16_t arrayElementToken;
        CHECK_PARSER_ERR(array_get_nth_element(parsedJson, internalTokenElemIdx, arrayElementIdx, &arrayElementToken))

        uint16_t valueTokenIdx;
        CHECK_PARSER_ERR(json_matchKeyValue(
                parsedJson,
                arrayElementToken,
                (char *) "UInt8", JSMN_PRIMITIVE,
                &valueTokenIdx))

        char tmpDecBuffer[20];
        char tmpHexBuffer[20];

        CHECK_PARSER_ERR(json_extractToken(tmpDecBuffer, sizeof(tmpDecBuffer), parsedJson, valueTokenIdx))
        CHECK_PARSER_ERR(formatStrUInt8AsHex(tmpDecBuffer, tmpHexBuffer))
        strcat(outVal, tmpHexBuffer);
    }

    return parser_ok;
}

parser_error_t _matchScriptType(uint8_t scriptHash[32], script_type_e *scriptType) {
    *scriptType = script_unknown;

    char buffer[100];
    MEMZERO(buffer, sizeof(buffer));

    // Check it is a known script digest
    if (array_to_hexstr(buffer, sizeof(buffer), scriptHash, CX_SHA256_SIZE) != 64) {
        return parser_unexpected_error;
    }

    if (MEMCMP(CONTRACT_HASH_TOKEN_TRANSFER, buffer, 64) == 0) {
        *scriptType = script_token_transfer;
        return parser_ok;
    }

    if (MEMCMP(CONTRACT_HASH_CREATE_ACCOUNT, buffer, 64) == 0) {
        *scriptType = script_create_account;
        return parser_ok;
    }

    if (MEMCMP(CONTRACT_HASH_ADD_NEW_KEY, buffer, 64) == 0) {
        *scriptType = script_add_new_key;
        return parser_ok;
    }

    return parser_unexpected_script;
}

parser_error_t _readScript(parser_context_t *c, flow_script_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_script_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_string)

    MEMZERO(v->digest, sizeof(v->digest));
    sha256(v->ctx.buffer, v->ctx.bufferLen, v->digest);

    CHECK_PARSER_ERR(_matchScriptType(v->digest, &v->type))

    return parser_ok;
}

parser_error_t _readArguments(parser_context_t *c, flow_argument_list_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_argument_list_t));

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_list)

    v->argCount = 0;
    while (v->ctx.offset < v->ctx.bufferLen && v->argCount < PARSER_MAX_ARGCOUNT) {
        CHECK_PARSER_ERR(rlp_decode(&v->ctx, &v->argCtx[v->argCount], &kind, &bytesConsumed))
        CTX_CHECK_AND_ADVANCE(&v->ctx, bytesConsumed)
        CHECK_KIND(kind, kind_string)
        v->argCount++;
    }
    v->ctx.offset = 0;
    if (v->argCount >= PARSER_MAX_ARGCOUNT) {
        return parser_unexpected_number_items;
    }

    return parser_ok;
}

parser_error_t _readReferenceBlockId(parser_context_t *c, flow_reference_block_id_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_reference_block_id_t));

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_string)
    return parser_ok;
}

parser_error_t _readGasLimit(parser_context_t *c, flow_gaslimit_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;
    parser_context_t ctx_local;

    MEMZERO(v, sizeof(flow_gaslimit_t));

    CHECK_PARSER_ERR(rlp_decode(c, &ctx_local, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_byte)

    CHECK_PARSER_ERR(rlp_readUInt64(&ctx_local, kind, v))

    return parser_ok;
}

parser_error_t _readProposalKeyAddress(parser_context_t *c, flow_proposal_key_address_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_proposal_key_address_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_string)
    return parser_ok;
}

parser_error_t _readProposalKeyId(parser_context_t *c, flow_proposal_keyid_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;
    parser_context_t ctx_local;

    MEMZERO(v, sizeof(flow_proposal_keyid_t));

    CHECK_PARSER_ERR(rlp_decode(c, &ctx_local, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_byte)

    CHECK_PARSER_ERR(rlp_readUInt64(&ctx_local, kind, v))

    return parser_ok;
}

parser_error_t _readProposalKeySequenceNumber(parser_context_t *c, flow_proposal_key_sequence_number_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;
    parser_context_t ctx_local;

    MEMZERO(v, sizeof(flow_proposal_key_sequence_number_t));

    CHECK_PARSER_ERR(rlp_decode(c, &ctx_local, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_byte)

    CHECK_PARSER_ERR(rlp_readUInt64(&ctx_local, kind, v))

    return parser_ok;
}

parser_error_t _readPayer(parser_context_t *c, flow_payer_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_payer_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_string)
    return parser_ok;
}

parser_error_t _readProposalAuthorizer(parser_context_t *c, flow_proposal_authorizer_t *v) {

    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_proposal_authorizer_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_string)

    return parser_ok;
}

parser_error_t _readProposalAuthorizers(parser_context_t *c, flow_proposal_authorizers_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    MEMZERO(v, sizeof(flow_proposal_authorizers_t));

    CHECK_PARSER_ERR(rlp_decode(c, &v->ctx, &kind, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_list)

    while (v->ctx.offset < v->ctx.bufferLen) {
        CHECK_PARSER_ERR(_readProposalAuthorizer(&v->ctx, &v->authorizer[v->authorizer_count]))
        v->authorizer_count++;
    }
    v->ctx.offset = 0;

    return parser_ok;
}

parser_error_t _read(parser_context_t *c, parser_tx_t *v) {
    rlp_kind_e kind;
    uint32_t bytesConsumed;

    parser_context_t ctx_rootList;
    parser_context_t ctx_rootInnerList;

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(c, &ctx_rootList, &kind, &bytesConsumed))
    CTX_CHECK_AND_ADVANCE(c, bytesConsumed)
    CHECK_KIND(kind, kind_list)
    if (bytesConsumed != c->bufferLen) {
        // root list should consume the complete buffer
        return parser_unexpected_buffer_end;
    }

    // Consume external list
    CHECK_PARSER_ERR(rlp_decode(&ctx_rootList, &ctx_rootInnerList, &kind, &bytesConsumed))
    CTX_CHECK_AND_ADVANCE(&ctx_rootList, bytesConsumed)
    CHECK_KIND(kind, kind_list)

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
        return parser_unexpected_buffer_end;
    }

    // Check last item? signers?
    // TODO: Do we want to show signers too?
    // TODO: confirm that things are not completed
    return parser_ok;
}

parser_error_t _validateTx(const parser_context_t *c, const parser_tx_t *v) {
    // Placeholded to run any coin specific validation
    return parser_ok;
}

uint8_t _countArgumentItems(const flow_argument_list_t *v, uint8_t argumentIndex) {
    parsed_json_t parsedJson = {false};

    if (argumentIndex >= v->argCount) {
        return 0;
    }

    const parser_context_t argCtx = v->argCtx[argumentIndex];
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argCtx.buffer, argCtx.bufferLen));

    // Get numnber of items
    uint16_t internalTokenElementIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, 0, (char *) "Array", JSMN_ARRAY, &internalTokenElementIdx));
    uint16_t arrayTokenCount;
    CHECK_PARSER_ERR(array_get_element_count(&parsedJson, internalTokenElementIdx, &arrayTokenCount));
    if (arrayTokenCount > 64) {
        return parser_unexpected_number_items;
    }

    return arrayTokenCount;
}

uint8_t _getNumItems(const parser_context_t *c, const parser_tx_t *v) {
    switch (v->script.type) {
        case script_token_transfer:
            return 9 + v->authorizers.authorizer_count;
        case script_create_account:
            return 7 + _countArgumentItems(&v->arguments, 0) + v->authorizers.authorizer_count;
        case script_add_new_key:
            return 8 + v->authorizers.authorizer_count;
        case script_unknown:
        default:
            return 0;
    }
}
