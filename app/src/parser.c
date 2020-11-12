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

#define STRING_ARGUMENT_SIZE 96

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

// based on Dapper provided code at https://github.com/onflow/flow-go-sdk/blob/96796f0cabc1847d7879a5230ab55fd3cdd41ae8/address.go#L286

const uint16_t linearCodeN = 64;
const uint64_t codeword_mainnet = 0;
const uint64_t codeword_testnet = 0x6834ba37b3980209;
const uint64_t codeword_emulatornet = 0x1cb159857af02018;

const uint32_t parityCheckMatrixColumns[] = {
        0x00001, 0x00002, 0x00004, 0x00008,
        0x00010, 0x00020, 0x00040, 0x00080,
        0x00100, 0x00200, 0x00400, 0x00800,
        0x01000, 0x02000, 0x04000, 0x08000,
        0x10000, 0x20000, 0x40000, 0x7328d,
        0x6689a, 0x6112f, 0x6084b, 0x433fd,
        0x42aab, 0x41951, 0x233ce, 0x22a81,
        0x21948, 0x1ef60, 0x1deca, 0x1c639,
        0x1bdd8, 0x1a535, 0x194ac, 0x18c46,
        0x1632b, 0x1529b, 0x14a43, 0x13184,
        0x12942, 0x118c1, 0x0f812, 0x0e027,
        0x0d00e, 0x0c83c, 0x0b01d, 0x0a831,
        0x0982b, 0x07034, 0x0682a, 0x05819,
        0x03807, 0x007d2, 0x00727, 0x0068e,
        0x0067c, 0x0059d, 0x004eb, 0x003b4,
        0x0036a, 0x002d9, 0x001c7, 0x0003f,
};

bool validateChainAddress(uint64_t chainCodeWord, uint64_t address) {
    uint64_t codeWord = address ^chainCodeWord;

    if (codeWord == 0) {
        return false;
    }

    uint64_t parity = 0;
    for (uint16_t i = 0; i < linearCodeN; i++) {
        if ((codeWord & 1) == 1) {
            parity ^= parityCheckMatrixColumns[i];
        }
        codeWord >>= 1;
    }

    return parity == 0;
}

parser_error_t chainIDFromPayer(const flow_payer_t *v, chain_id_e *chainID) {
    if (v->ctx.bufferLen != 8) {
        return parser_invalid_address;
    }

    uint64_t address = 0;
    for (uint8_t i = 0; i < 8; i++) {
        address <<= 8;
        address += v->ctx.buffer[i];
    }

    if (validateChainAddress(codeword_mainnet, address)) {
        *chainID = chain_id_mainnet;
        return parser_ok;
    }

    if (validateChainAddress(codeword_testnet, address)) {
        *chainID = chain_id_testnet;
        return parser_ok;
    }

    if (validateChainAddress(codeword_emulatornet, address)) {
        *chainID = chain_id_emulator;
        return parser_ok;
    }

    return parser_unexpected_value;
}

parser_error_t parser_printChainID(const flow_payer_t *v,
                                   char *outVal, uint16_t outValLen,
                                   uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);
    chain_id_e chainID;
    CHECK_PARSER_ERR(chainIDFromPayer(v, &chainID));

    *pageCount = 1;
    switch (chainID) {
        case chain_id_mainnet:
            snprintf(outVal, outValLen, "Mainnet");
            return parser_ok;
        case chain_id_testnet:
            snprintf(outVal, outValLen, "Testnet");
            return parser_ok;
        case chain_id_emulator:
            snprintf(outVal, outValLen, "Emulator");
            return parser_ok;
        case chain_id_unknown:
        default:
            return parser_invalid_address;
    }

    return parser_invalid_address;
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

parser_error_t parser_printArgumentString(const parser_context_t *argumentCtx,
                                             char *outVal, uint16_t outValLen,
                                             uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argumentCtx->buffer, argumentCtx->bufferLen));

    char bufferUI[STRING_ARGUMENT_SIZE];
    CHECK_PARSER_ERR(json_extractPubKey(bufferUI, sizeof(bufferUI), &parsedJson, 0))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return parser_display_page_out_of_range;
    }

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
    *pageCount = 1;

    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Token Transfer");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Destination");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "Address", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 9:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 10;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemCreateAccount(const parser_context_t *ctx,
                                           uint16_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("parser_getItemCreateAccount");
    *pageCount = 1;

    if (displayIdx == 0) {
        snprintf(outKey, outKeyLen, "Type");
        snprintf(outVal, outValLen, "Create Account");
        return parser_ok;
    }
    displayIdx--;
    if (displayIdx == 0) {
        snprintf(outKey, outKeyLen, "ChainID");
        return parser_printChainID(&parser_tx_obj.payer,
                                   outVal, outValLen, pageIdx, pageCount);
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

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemAddNewKey(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Add New Key");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2: {
            CHECK_PARSER_ERR(
                    parser_printArgumentPublicKey(
                            &parser_tx_obj.arguments.argCtx[0], outVal, outValLen,
                            pageIdx, pageCount))
            snprintf(outKey, outKeyLen, "Pub key");
            return parser_ok;
        }
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemWithdrawUnlockedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw FLOW from Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2: 
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemDepositUnlockedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Deposit FLOW to Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2: 
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}


parser_error_t parser_getItemRegisterNode(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Register Staked Node");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Node Role");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "UInt8", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Networking Address");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[2],
                                              outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Networking Key");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[3],
                                              outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Staking Key");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[4],
                                              outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 5,
                                        "UFix64", JSMN_STRING,
                                    outVal, outValLen, pageIdx, pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 9:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 10:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 11:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 12:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 13:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 14;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemStakeNewTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Stake FLOW from Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemRestakeUnstakedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Restake Unstaked FLOW");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemRestakeRewardedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Restake Rewarded FLOW");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemUnstakeTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Unstake FLOW");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemUnstakeAllTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Unstake All FLOW");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
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

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemWithdrawUnstakedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw Unstaked FLOW to Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemWithdrawRewardedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw Rewarded FLOW to Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemRegisterOperatorNode(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Register Operator Node");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Operator Address");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "Address", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[1],
                                              outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 9:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 10:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 11;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemRegisterDelegator(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Register Delegator");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 9:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 10;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemDelegateNewTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Delegate FLOW from Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemRestakeUnstakedDelegatedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Re-delegate Unstaked FLOW");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemRestakeRewardedDelegatedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Re-delegate Rewarded FLOW");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemUnstakeDelegatedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Unstake Delegated FLOW");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemWithdrawUnstakedDelegatedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw Undelegated FLOW to Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
}

parser_error_t parser_getItemWithdrawRewardedDelegatedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw Delegate Rewards to Lockbox");
            return parser_ok;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Ref Block");
            return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Gas Limit");
            return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Prop Key Addr");
            return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
        case 6:
            snprintf(outKey, outKeyLen, "Prop Key Id");
            return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
        case 7:
            snprintf(outKey, outKeyLen, "Prop Key Seq Num");
            return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx,
                                          pageCount);
        case 8:
            snprintf(outKey, outKeyLen, "Payer");
            return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 9;

    if (displayIdx < parser_tx_obj.authorizers.authorizer_count) {
        snprintf(outKey, outKeyLen, "Authorizer %d", displayIdx + 1);
        return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[displayIdx], outVal, outValLen, pageIdx,
                                      pageCount);
    }

    return parser_display_idx_out_of_range;
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
        case script_th01_withdraw_unlocked_tokens:
            return parser_getItemWithdrawUnlockedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, 
                                                        pageIdx, pageCount);
        case script_th02_deposit_unlocked_tokens:
            return parser_getItemDepositUnlockedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                       pageIdx, pageCount);
        case script_th06_register_node:
            return parser_getItemRegisterNode(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                              pageIdx, pageCount);
        case script_th08_stake_new_tokens:
            return parser_getItemStakeNewTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                pageIdx, pageCount);
        case script_th09_restake_unstaked_tokens:
            return parser_getItemRestakeUnstakedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                       pageIdx, pageCount);
        case script_th10_restake_rewarded_tokens:
            return parser_getItemRestakeRewardedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                       pageIdx, pageCount);
        case script_th11_unstake_tokens:
            return parser_getItemUnstakeTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                               pageIdx, pageCount);
        case script_th12_unstake_all_tokens:
            return parser_getItemUnstakeAllTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                  pageIdx, pageCount);
        case script_th13_withdraw_unstaked_tokens:
            return parser_getItemWithdrawUnstakedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                        pageIdx, pageCount);
        case script_th14_withdraw_rewarded_tokens:
            return parser_getItemWithdrawRewardedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                        pageIdx, pageCount);
        case script_th16_register_operator_node:
            return parser_getItemRegisterOperatorNode(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                      pageIdx, pageCount);
        case script_th17_register_delegator:
            return parser_getItemRegisterDelegator(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                   pageIdx, pageCount);
        case script_th19_delegate_new_tokens:
            return parser_getItemDelegateNewTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                   pageIdx, pageCount);
        case script_th20_restake_unstaked_delegated_tokens:
            return parser_getItemRestakeUnstakedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                outValLen, pageIdx, pageCount);
        case script_th21_restake_rewarded_delegated_tokens:
            return parser_getItemRestakeRewardedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                outValLen, pageIdx, pageCount);
        case script_th22_unstake_delegated_tokens:
            return parser_getItemUnstakeDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                        outValLen, pageIdx, pageCount);
        case script_th23_withdraw_unstaked_delegated_tokens:
            return parser_getItemWithdrawUnstakedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case script_th24_withdraw_rewarded_delegated_tokens:
            return parser_getItemWithdrawRewardedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
    }

    return parser_unexpected_script;
}
