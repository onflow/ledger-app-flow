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

__Z_INLINE parser_error_t parser_printBlockId(const flow_reference_block_id_t *v,
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

__Z_INLINE parser_error_t parser_printGasLimit(const flow_gaslimit_t *v,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (!tostring256(v, 10, outBuffer, sizeof(outBuffer))) {
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

__Z_INLINE parser_error_t parser_printPropKeyId(const flow_proposal_keyid_t *v,
                                                char *outVal, uint16_t outValLen,
                                                uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (!tostring256(v, 10, outBuffer, sizeof(outBuffer))) {
        return parser_unexpected_value;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printPropSeqNum(const flow_proposal_key_sequence_number_t *v,
                                                 char *outVal, uint16_t outValLen,
                                                 uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (!tostring256(v, 10, outBuffer, sizeof(outBuffer))) {
        return parser_unexpected_value;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return parser_ok;
}

__Z_INLINE parser_error_t parser_printPayer(const flow_payer_t *v,
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

__Z_INLINE parser_error_t parser_printAuthorizer(const flow_proposal_authorizer_t *v,
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

    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            switch (parser_tx_obj.script.type) {
                case script_unknown:
                    return parser_unexpected_script;
                case script_token_transfer:
                    snprintf(outVal, outValLen, "Token Transfer");
                    break;
                case script_create_account:
                    snprintf(outVal, outValLen, "Create Account");
                    break;
                default:
                    return parser_unexpected_script;
            }
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "Param");
            pageStringExt(outVal, outValLen,
                          (const char *) parser_tx_obj.arguments.ctx.buffer,
                          parser_tx_obj.arguments.ctx.bufferLen, pageIdx, pageCount);
            return parser_ok;
        case 2:
            snprintf(outKey, outKeyLen, "Ref Block Id");
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
