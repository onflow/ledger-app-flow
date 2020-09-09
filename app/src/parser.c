/*******************************************************************************
*   (c) 2019 Zondax GmbH
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

#include <stdio.h>
#include <zxmacros.h>
#include "parser_impl.h"
#include "bignum.h"
#include "parser.h"
#include "parser_txdef.h"
#include "coin.h"

#if defined(TARGET_NANOX)
// For some reason NanoX requires this function
void __assert_fail(const char * assertion, const char * file, unsigned int line, const char * function){
    while(1) {};
}
#endif

#define FLOW_PUBLIC_KEY_SIZE 64     // 64 bytes for public key
#define FLOW_SIG_ALGO_SIZE 1        // 8 bits for signature algorithm (uint8)
#define FLOW_HASH_ALGO_SIZE 1       // 8 bits for hash algorithm (uint8)
#define FLOW_WEIGHT_SIZE 2          // 16 bits for weight (uint16)
#define RLP_PREFIX 1

#define FLOW_ACCOUNT_KEY_SIZE (2 * ( \
    (RLP_PREFIX * 2) + \
    ((RLP_PREFIX * 2) + FLOW_PUBLIC_KEY_SIZE) + \
    (RLP_PREFIX + FLOW_SIG_ALGO_SIZE) + \
    (RLP_PREFIX + FLOW_HASH_ALGO_SIZE) + \
    (RLP_PREFIX + FLOW_WEIGHT_SIZE) \
) + 2)

parser_error_t parser_parse(parser_context_t *ctx, const uint8_t *data, size_t dataLen) {
    CHECK_PARSER_ERR(parser_init(ctx, data, dataLen))
    return _read(ctx, &parser_tx_obj);
}

parser_error_t parser_validate(const parser_context_t *ctx) {
    CHECK_PARSER_ERR(_validateTx(ctx, &parser_tx_obj))

    // Iterate through all items to check that all can be shown and are valid
    uint8_t numItems = 0;
    CHECK_PARSER_ERR(parser_getNumItems(ctx, &numItems));

    char tmpKey[40];
    char tmpVal[40];

    for (uint8_t idx = 0; idx < numItems; idx++) {
        uint8_t pageCount = 0;
        CHECK_PARSER_ERR(parser_getItem(ctx, idx, tmpKey, sizeof(tmpKey), tmpVal, sizeof(tmpVal), 0, &pageCount))
    }

    return parser_ok;
}

parser_error_t parser_getNumItems(const parser_context_t *ctx, uint8_t *num_items) {
    *num_items = _getNumItems(ctx, &parser_tx_obj);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printArgument(const flow_argument_list_t *v,
                                               uint8_t argIndex, char *expectedType, jsmntype_t jsonType,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    if (argIndex >= v->argCount) {
        return parser_unexpected_number_items;
    }

    *pageCount = 1;

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) v->argCtx[argIndex].buffer, v->argCtx[argIndex].bufferLen));
    uint16_t valueTokenIndex;
    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, 0, expectedType, jsonType, &valueTokenIndex))
    CHECK_PARSER_ERR(json_extractToken(outVal, outValLen, &parsedJson, valueTokenIndex))

    return parser_ok;
}

parser_error_t parser_printArgumentPublicKey(const parser_context_t *argumentCtx,
                                             char *outVal, uint16_t outValLen,
                                             uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argumentCtx->buffer, argumentCtx->bufferLen));

    char bufferUI[FLOW_ACCOUNT_KEY_SIZE];
    CHECK_PARSER_ERR(json_extractPubKey(bufferUI, sizeof(bufferUI), &parsedJson, 0))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return parser_display_page_out_of_range;
    }

    return parser_ok;
}

parser_error_t parser_printArgumentPublicKeys(const parser_context_t *argumentCtx, uint8_t argumentIndex,
                                              char *outVal, uint16_t outValLen,
                                              uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argumentCtx->buffer, argumentCtx->bufferLen));

    // Estimate number of pages
    uint16_t internalTokenElementIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, 0, (char *) "Array", JSMN_ARRAY, &internalTokenElementIdx));
    uint16_t arrayTokenCount;
    CHECK_PARSER_ERR(array_get_element_count(&parsedJson, internalTokenElementIdx, &arrayTokenCount));
    if (arrayTokenCount > 64) {
        return parser_unexpected_number_items;
    }

    zemu_log_stack("PublicKeys");

    uint16_t arrayElementToken;
    char bufferUI[FLOW_ACCOUNT_KEY_SIZE];
    CHECK_PARSER_ERR(array_get_nth_element(&parsedJson, internalTokenElementIdx, argumentIndex, &arrayElementToken))
    CHECK_PARSER_ERR(json_extractPubKey(bufferUI, sizeof(bufferUI), &parsedJson, arrayElementToken))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return parser_display_page_out_of_range;
    }

    return parser_ok;
}

parser_error_t parser_printBlockId(const flow_reference_block_id_t *v,
                                   char *outVal, uint16_t outValLen,
                                   uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 32) {
        return parser_invalid_address;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 64) {
        return parser_invalid_address;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

parser_error_t parser_printGasLimit(const flow_gaslimit_t *v,
                                    char *outVal, uint16_t outValLen,
                                    uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (uint64_to_str(outBuffer, sizeof(outBuffer), *v) != NULL) {
        return parser_unexpected_value;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printPropKeyAddr(const flow_proposal_key_address_t *v,
                                                  char *outVal, uint16_t outValLen,
                                                  uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 8) {
        return parser_invalid_address;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 16) {
        return parser_invalid_address;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

parser_error_t parser_printPropKeyId(const flow_proposal_keyid_t *v,
                                     char *outVal, uint16_t outValLen,
                                     uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (uint64_to_str(outBuffer, sizeof(outBuffer), *v) != NULL) {
        return parser_unexpected_value;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

parser_error_t parser_printPropSeqNum(const flow_proposal_key_sequence_number_t *v,
                                      char *outVal, uint16_t outValLen,
                                      uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (uint64_to_str(outBuffer, sizeof(outBuffer), *v) != NULL) {
        return parser_unexpected_value;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

parser_error_t parser_printPayer(const flow_payer_t *v,
                                 char *outVal, uint16_t outValLen,
                                 uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 8) {
        return parser_invalid_address;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 16) {
        return parser_invalid_address;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

parser_error_t parser_printAuthorizer(const flow_proposal_authorizer_t *v,
                                      char *outVal, uint16_t outValLen,
                                      uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 8) {
        return parser_invalid_address;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 16) {
        return parser_invalid_address;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

parser_error_t parser_getItemTokenTransfer(const parser_context_t *ctx,
                                           uint16_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    if (displayIdx == 0) {
        snprintf(outKey, outKeyLen, "Type");
        snprintf(outVal, outValLen, "Token Transfer");
        return parser_ok;
    }
    displayIdx--;

    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, displayIdx,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 1:
            snprintf(outKey, outKeyLen, "Destination");
            return parser_printArgument(&parser_tx_obj.arguments, displayIdx,
                                        "Address", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 8;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_ok;
}

parser_error_t parser_getItemCreateAccount(const parser_context_t *ctx,
                                           uint16_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("parser_getItemCreateAccount");

    if (displayIdx == 0) {
        snprintf(outKey, outKeyLen, "Type");
        snprintf(outVal, outValLen, "Create Account");
        return parser_ok;
    }
    displayIdx--;

    const uint8_t pkCount = _countArgumentItems(&parser_tx_obj.arguments, 0);
    if (displayIdx < pkCount) {
        snprintf(outKey, outKeyLen, "Pub key %d", displayIdx + 1);
        CHECK_PARSER_ERR(
                parser_printArgumentPublicKeys(
                        &parser_tx_obj.arguments.argCtx[0],
                        displayIdx, outVal, outValLen,
                        pageIdx, pageCount))
        return parser_ok;
    }
    displayIdx -= pkCount;

    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 1:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 6;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_ok;
}

parser_error_t parser_getItemAddNewKey(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Add New Key");
            return parser_ok;
        case 1: {
            CHECK_PARSER_ERR(
                    parser_printArgumentPublicKey(
                            &parser_tx_obj.arguments.argCtx[0], outVal, outValLen,
                            pageIdx, pageCount))
            snprintf(outKey, outKeyLen, "Pub key");
            return parser_ok;
        }
        case 2:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 8;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_ok;
}

parser_error_t parser_getItem(const parser_context_t *ctx,
                              uint16_t displayIdx,
                              char *outKey, uint16_t outKeyLen,
                              char *outVal, uint16_t outValLen,
                              uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);
    snprintf(outKey, outKeyLen, "? %d", displayIdx);
    snprintf(outVal, outValLen, "?");
    *pageCount = 0;

    uint8_t numItems;
    CHECK_PARSER_ERR(parser_getNumItems(ctx, &numItems))
    CHECK_APP_CANARY()

    if (displayIdx < 0 || displayIdx >= numItems) {
        return parser_no_data;
    }
    *pageCount = 1;

    switch (parser_tx_obj.script.type) {
        case script_unknown:
            return parser_unexpected_script;
        case script_token_transfer:
            return parser_getItemTokenTransfer(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx,
                                               pageCount);
        case script_create_account:
            return parser_getItemCreateAccount(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx,
                                               pageCount);
        case script_add_new_key:
            return parser_getItemAddNewKey(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx,
                                           pageCount);
    }

    return parser_unexpected_script;
}
