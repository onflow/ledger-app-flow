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
#include "parser_impl.h"
#include "parser_txdef.h"
#include "app_mode.h"
#include "rlp.h"

parser_tx_t parser_tx_obj;

#define PARSER_ASSERT_OR_ERROR(CALL, ERROR) if (!(CALL)) return ERROR;

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

parser_error_t _readScript(const parser_context_t *c, flow_script_t *v) {
//    rlp_kind_e kind;
//    uint16_t len;
//    uint16_t valueOffset;
//
//    rlp_decode(c->buffer, &kind, &len, &valueOffset);
//
    return parser_ok;
}

parser_error_t _readArguments(const parser_context_t *c, flow_argument_list_t *v) {
    return parser_ok;
}

parser_error_t _readGasLimit(const parser_context_t *c, flow_gaslimit_t *v) {
    return parser_ok;
}

parser_error_t _readProposalKeyAddress(const parser_context_t *c, flow_proposal_key_address_t *v) {
    return parser_ok;
}

parser_error_t _readProposalKeyId(const parser_context_t *c, flow_proposal_keyid_t *v) {
    return parser_ok;
}

parser_error_t _readProposalKeySequenceNumber(const parser_context_t *c, flow_proposal_key_sequence_number_t *v) {
    return parser_ok;
}

parser_error_t _readPayer(const parser_context_t *c, flow_payer_t *v) {
    return parser_ok;
}

parser_error_t _readProposalAuthorizers(const parser_context_t *c, flow_proposal_authorizers_t *v) {
    return parser_ok;
}

parser_error_t _read(parser_context_t *c, parser_tx_t *v) {
    rlp_kind_e kind;
    uint16_t len;
    uint16_t valueOffset;
    uint16_t bytesConsumed;
    CHECK_PARSER_ERR(rlp_decode(c->buffer, &kind, &len, &valueOffset, &bytesConsumed));
    CTX_CHECK_AND_ADVANCE(c, valueOffset)
    if (kind != kind_list) {
        return parser_rlp_error_invalid_kind;
    }
    if (bytesConsumed != c->bufferLen) {
        // root list should consume the complete buffer
        return parser_unexpected_buffer_end;
    }

    // TODO:
//    CHECK_PARSER_ERR(_readScript(c, &v->script))
//    CHECK_PARSER_ERR(_readArguments(c, &v->arguments))
//    CHECK_PARSER_ERR(_readGasLimit(c, &v->gasLimit))
//    CHECK_PARSER_ERR(_readProposalKeyAddress(c, &v->proposalKeyAddress))
//    CHECK_PARSER_ERR(_readProposalKeyId(c, &v->proposalKeyId))
//    CHECK_PARSER_ERR(_readProposalKeySequenceNumber(c, &v->proposalKeySequenceNumber))
//    CHECK_PARSER_ERR(_readPayer(c, &v->payer))
//    CHECK_PARSER_ERR(_readProposalAuthorizers(c, &v->authorizers))

    return parser_ok;
}

parser_error_t _validateTx(const parser_context_t *c, const parser_tx_t *v) {
    return parser_ok;
}

uint8_t _getNumItems(const parser_context_t *c, const parser_tx_t *v) {
    uint8_t itemCount = 0;

    return itemCount;
}
