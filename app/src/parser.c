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
#include "zxformat.h"

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

#define ARGUMENT_BUFFER_SIZE_ACCOUNT_KEY (2 * ( \
    (RLP_PREFIX * 2) + \
    ((RLP_PREFIX * 2) + FLOW_PUBLIC_KEY_SIZE) + \
    (RLP_PREFIX + FLOW_SIG_ALGO_SIZE) + \
    (RLP_PREFIX + FLOW_HASH_ALGO_SIZE) + \
    (RLP_PREFIX + FLOW_WEIGHT_SIZE) \
) + 2)

#define ARGUMENT_BUFFER_SIZE_STRING 256

#define MAX_JSON_ARRAY_TOKEN_COUNT 64  

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

    return PARSER_OK;
}

parser_error_t parser_getNumItems(const parser_context_t *ctx, uint8_t *num_items) {
    CHECK_PARSER_ERR(_getNumItems(ctx, &parser_tx_obj, num_items))
    return PARSER_OK;
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
        return PARSER_INVALID_ADDRESS;
    }

    uint64_t address = 0;
    for (uint8_t i = 0; i < 8; i++) {
        address <<= 8;
        address += v->ctx.buffer[i];
    }

    if (validateChainAddress(codeword_mainnet, address)) {
        *chainID = CHAIN_ID_MAINNET;
        return PARSER_OK;
    }

    if (validateChainAddress(codeword_testnet, address)) {
        *chainID = CHAIN_ID_TESTNET;
        return PARSER_OK;
    }

    if (validateChainAddress(codeword_emulatornet, address)) {
        *chainID = CHAIN_ID_EMULATOR;
        return PARSER_OK;
    }

    return PARSER_UNEXPECTED_VALUE;
}

parser_error_t parser_printChainID(const flow_payer_t *v,
                                   char *outVal, uint16_t outValLen,
                                   uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);
    chain_id_e chainID;
    CHECK_PARSER_ERR(chainIDFromPayer(v, &chainID));

    *pageCount = 1;
    switch (chainID) {
        case CHAIN_ID_MAINNET:
            snprintf(outVal, outValLen, "Mainnet");
            return PARSER_OK;
        case CHAIN_ID_TESTNET:
            snprintf(outVal, outValLen, "Testnet");
            return PARSER_OK;
        case CHAIN_ID_EMULATOR:
            snprintf(outVal, outValLen, "Emulator");
            return PARSER_OK;
        case CHAIN_ID_UNKNOWN:
        default:
            return PARSER_INVALID_ADDRESS;
    }

    return PARSER_INVALID_ADDRESS;
}

parser_error_t parser_printArgument(const flow_argument_list_t *v,
                                               uint8_t argIndex, char *expectedType, jsmntype_t jsonType,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    if (argIndex >= v->argCount) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    *pageCount = 1;

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) v->argCtx[argIndex].buffer, v->argCtx[argIndex].bufferLen));
    uint16_t valueTokenIndex;
    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, 0, expectedType, jsonType, &valueTokenIndex))
    CHECK_PARSER_ERR(json_extractToken(outVal, outValLen, &parsedJson, valueTokenIndex))

    return PARSER_OK;
}


parser_error_t parser_printArgumentOptionalDelegatorID(const flow_argument_list_t *v,
                                               uint8_t argIndex, char *expectedType, jsmntype_t jsonType,
                                               char *outVal, uint16_t outValLen,
                                               uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    if (argIndex >= v->argCount) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    *pageCount = 1;

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) v->argCtx[argIndex].buffer, v->argCtx[argIndex].bufferLen));
    uint16_t valueTokenIndex;
    CHECK_PARSER_ERR(json_matchOptionalKeyValue(&parsedJson, 0, expectedType, jsonType, &valueTokenIndex))
    if (valueTokenIndex == JSON_MATCH_VALUE_IDX_NONE) {
        if (outValLen < 5) {
            return PARSER_UNEXPECTED_BUFFER_END;
        }         
        strncpy_s(outVal, "None", 5);
    }
    else {
        CHECK_PARSER_ERR(json_extractToken(outVal, outValLen, &parsedJson, valueTokenIndex))
    }

    return PARSER_OK;
}


parser_error_t parser_printArgumentString(const parser_context_t *argumentCtx,
                                             char *outVal, uint16_t outValLen,
                                             uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argumentCtx->buffer, argumentCtx->bufferLen));

    char bufferUI[ARGUMENT_BUFFER_SIZE_STRING];
    CHECK_PARSER_ERR(json_extractString(bufferUI, sizeof(bufferUI), &parsedJson, 0))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return PARSER_DISPLAY_PAGE_OUT_OF_RANGE;
    }

    return PARSER_OK;
}

parser_error_t parser_printArgumentPublicKey(const parser_context_t *argumentCtx,
                                             char *outVal, uint16_t outValLen,
                                             uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argumentCtx->buffer, argumentCtx->bufferLen));

    char bufferUI[ARGUMENT_BUFFER_SIZE_ACCOUNT_KEY];
    CHECK_PARSER_ERR(json_extractString(bufferUI, sizeof(bufferUI), &parsedJson, 0))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return PARSER_DISPLAY_PAGE_OUT_OF_RANGE;
    }

    return PARSER_OK;
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
    if (arrayTokenCount > MAX_JSON_ARRAY_TOKEN_COUNT) {  //indirectly limits the maximum number of public keys
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    zemu_log_stack("PublicKeys");

    uint16_t arrayElementToken;
    char bufferUI[ARGUMENT_BUFFER_SIZE_ACCOUNT_KEY];
    CHECK_PARSER_ERR(array_get_nth_element(&parsedJson, internalTokenElementIdx, argumentIndex, &arrayElementToken))
    CHECK_PARSER_ERR(json_extractString(bufferUI, sizeof(bufferUI), &parsedJson, arrayElementToken))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return PARSER_DISPLAY_PAGE_OUT_OF_RANGE;
    }

    return PARSER_OK;
}

parser_error_t parser_printArgumentOptionalPublicKeys(const parser_context_t *argumentCtx, uint8_t argumentIndex,
                                              char *outVal, uint16_t outValLen,
                                              uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) argumentCtx->buffer, argumentCtx->bufferLen));

    // Estimate number of pages
    uint16_t internalTokenElementIdx;
    CHECK_PARSER_ERR(json_matchOptionalArray(&parsedJson, 0, &internalTokenElementIdx));
    if (internalTokenElementIdx == JSON_MATCH_VALUE_IDX_NONE) {
        if (outValLen < 5) {
            return  PARSER_UNEXPECTED_BUFFER_END;
        }
        *pageCount = 1;
        strncpy_s(outVal, "None", 5);
    }
    else {
        uint16_t arrayTokenCount;
        CHECK_PARSER_ERR(array_get_element_count(&parsedJson, internalTokenElementIdx, &arrayTokenCount));
        if (arrayTokenCount > MAX_JSON_ARRAY_TOKEN_COUNT) { //indirectly limits the maximum number of public keys
            return PARSER_UNEXPECTED_NUMBER_ITEMS;
        }

        zemu_log_stack("PublicKeys");

        uint16_t arrayElementToken;
        char bufferUI[ARGUMENT_BUFFER_SIZE_ACCOUNT_KEY];
        CHECK_PARSER_ERR(array_get_nth_element(&parsedJson, internalTokenElementIdx, argumentIndex, &arrayElementToken))
        CHECK_PARSER_ERR(json_extractString(bufferUI, sizeof(bufferUI), &parsedJson, arrayElementToken))
        pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

        // Check requested page is in range
        if (pageIdx > *pageCount) {
            return PARSER_DISPLAY_PAGE_OUT_OF_RANGE;
        }
    }

    return PARSER_OK;
}

parser_error_t parser_printBlockId(const flow_reference_block_id_t *v,
                                   char *outVal, uint16_t outValLen,
                                   uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 32) {
        return PARSER_INVALID_ADDRESS;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 64) {
        return PARSER_INVALID_ADDRESS;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return PARSER_OK;
}

parser_error_t parser_printGasLimit(const flow_gaslimit_t *v,
                                    char *outVal, uint16_t outValLen,
                                    uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (uint64_to_str(outBuffer, sizeof(outBuffer), *v) != NULL) {
        return PARSER_UNEXPECTED_VALUE;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return PARSER_OK;
}

__Z_INLINE parser_error_t parser_printPropKeyAddr(const flow_proposal_key_address_t *v,
                                                  char *outVal, uint16_t outValLen,
                                                  uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 8) {
        return PARSER_INVALID_ADDRESS;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 16) {
        return PARSER_INVALID_ADDRESS;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return PARSER_OK;
}

parser_error_t parser_printPropKeyId(const flow_proposal_keyid_t *v,
                                     char *outVal, uint16_t outValLen,
                                     uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (uint64_to_str(outBuffer, sizeof(outBuffer), *v) != NULL) {
        return PARSER_UNEXPECTED_VALUE;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return PARSER_OK;
}

parser_error_t parser_printPropSeqNum(const flow_proposal_key_sequence_number_t *v,
                                      char *outVal, uint16_t outValLen,
                                      uint8_t pageIdx, uint8_t *pageCount) {
    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (uint64_to_str(outBuffer, sizeof(outBuffer), *v) != NULL) {
        return PARSER_UNEXPECTED_VALUE;
    }

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return PARSER_OK;
}

parser_error_t parser_printPayer(const flow_payer_t *v,
                                 char *outVal, uint16_t outValLen,
                                 uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 8) {
        return PARSER_INVALID_ADDRESS;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 16) {
        return PARSER_INVALID_ADDRESS;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return PARSER_OK;
}

parser_error_t parser_printAuthorizer(const flow_proposal_authorizer_t *v,
                                      char *outVal, uint16_t outValLen,
                                      uint8_t pageIdx, uint8_t *pageCount) {
    if (v->ctx.bufferLen != 8) {
        return PARSER_INVALID_ADDRESS;
    }

    char outBuffer[100];
    MEMZERO(outBuffer, sizeof(outBuffer));

    if (array_to_hexstr(outBuffer, sizeof(outBuffer), v->ctx.buffer, v->ctx.bufferLen) != 16) {
        return PARSER_INVALID_ADDRESS;
    };

    pageString(outVal, outValLen, outBuffer, pageIdx, pageCount);
    return PARSER_OK;
}

parser_error_t parser_getItemAfterArguments(const parser_context_t *ctx,
                                           uint16_t displayIdx,
                                           char *outKey, uint16_t outKeyLen,
                                           char *outVal, uint16_t outValLen,
                                           uint8_t pageIdx, uint8_t *pageCount) {
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

    return PARSER_DISPLAY_IDX_OUT_OF_RANGE;
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
            return PARSER_OK;
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
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

#define CREATE_ACCOUNT_MAX_PUB_KEYS 5
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
        return PARSER_OK;
    }
    displayIdx--;
    if (displayIdx == 0) {
        snprintf(outKey, outKeyLen, "ChainID");
        return parser_printChainID(&parser_tx_obj.payer,
                                   outVal, outValLen, pageIdx, pageCount);
    }
    displayIdx--;

    uint8_t pkCount = 0;
    CHECK_PARSER_ERR(_countArgumentItems(&parser_tx_obj.arguments, 0, 
                                         CREATE_ACCOUNT_MAX_PUB_KEYS, &pkCount));
    if (displayIdx < pkCount) {
        snprintf(outKey, outKeyLen, "Pub key %d", displayIdx + 1);
        CHECK_PARSER_ERR(
                parser_printArgumentPublicKeys(
                        &parser_tx_obj.arguments.argCtx[0],
                        displayIdx, outVal, outValLen,
                        pageIdx, pageCount))
        return PARSER_OK;
    }
    displayIdx -= pkCount;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
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
            return PARSER_OK;
        }
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2: 
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2: 
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            CHECK_PARSER_ERR(
                parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                           outVal, outValLen, pageIdx, pageCount));
            snprintf(outKey, outKeyLen, "Node ID");
            return PARSER_OK;
        case 3:
            snprintf(outKey, outKeyLen, "Node Role");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "UInt8", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 4:
            CHECK_PARSER_ERR(
                parser_printArgumentString(&parser_tx_obj.arguments.argCtx[2],
                                              outVal, outValLen, pageIdx, pageCount));
            snprintf(outKey, outKeyLen, "Networking Address");
            return PARSER_OK;
        case 5:
            CHECK_PARSER_ERR(
                parser_printArgumentString(&parser_tx_obj.arguments.argCtx[3],
                                              outVal, outValLen, pageIdx, pageCount));
            snprintf(outKey, outKeyLen, "Networking Key");
            return PARSER_OK;
        case 6:
            CHECK_PARSER_ERR(
                parser_printArgumentString(&parser_tx_obj.arguments.argCtx[4],
                                              outVal, outValLen, pageIdx, pageCount));
            snprintf(outKey, outKeyLen, "Staking Key");
            return PARSER_OK;
        case 7:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 5,
                                        "UFix64", JSMN_STRING,
                                    outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 8;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 2;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
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
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
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
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

parser_error_t parser_getItemUpdateNetworkingAddress(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Update Networking Address");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Address");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.01
parser_error_t parser_getItemSetupStaingCollection(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Setup Staking Collection");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 2;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.02
parser_error_t parser_getItemRegisterDelegatorSCO(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Register Delegator");
            return PARSER_OK;
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
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.03
#define SCO03_REGISTER_NODE_MAX_PUB_KEYS 3
parser_error_t parser_getItemRegisterNodeSCO(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("parser_getItemRegisterNodeSCO");
    *pageCount = 1;

    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Register Node");
            return PARSER_OK;
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
            snprintf(outKey, outKeyLen, "Netw. Addr.");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[2],
                                              outVal, outValLen, pageIdx, pageCount);
        case 5:
            snprintf(outKey, outKeyLen, "Netw. Key");
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
        default:
            break;
    }
    displayIdx -= 8;
    

    uint8_t pkCount = 0;
    CHECK_PARSER_ERR(_countArgumentOptionalItems(&parser_tx_obj.arguments, 6, 
                                                SCO03_REGISTER_NODE_MAX_PUB_KEYS, &pkCount))
    if (displayIdx < pkCount) {
        snprintf(outKey, outKeyLen, "Pub key %d", displayIdx + 1);
        CHECK_PARSER_ERR(
                parser_printArgumentOptionalPublicKeys(
                        &parser_tx_obj.arguments.argCtx[6],
                        displayIdx, outVal, outValLen,
                        pageIdx, pageCount))
        return PARSER_OK;
    }
    displayIdx -= pkCount;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.04
#define SCO04_CREATE_MACHINE_ACOUNT_MAX_PUB_KEYS 3
parser_error_t parser_getItemCreateMachineAccount(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("parser_getItemCreateAccount");
    *pageCount = 1;

    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Create Machine Account");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    

    uint8_t pkCount = 0;
    CHECK_PARSER_ERR(_countArgumentItems(&parser_tx_obj.arguments, 1, 
                                         SCO04_CREATE_MACHINE_ACOUNT_MAX_PUB_KEYS, &pkCount));
    if (displayIdx < pkCount) {
        snprintf(outKey, outKeyLen, "Pub key %d", displayIdx + 1);
        CHECK_PARSER_ERR(
                parser_printArgumentPublicKeys(
                        &parser_tx_obj.arguments.argCtx[1],
                        displayIdx, outVal, outValLen,
                        pageIdx, pageCount))
        return PARSER_OK;
    }
    displayIdx -= pkCount;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.05
parser_error_t parser_getItemRequestUnstaking(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Request Unstaking");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgumentOptionalDelegatorID(&parser_tx_obj.arguments, 1,
                                              "UInt32", JSMN_STRING,
                                              outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.06
parser_error_t parser_getItemStakeNewTokensSCO(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Stake New Tokens");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgumentOptionalDelegatorID(&parser_tx_obj.arguments, 1,
                                              "UInt32", JSMN_STRING,
                                              outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.07
parser_error_t parser_getItemStakeRewardTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Stake Reward Tokens");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgumentOptionalDelegatorID(&parser_tx_obj.arguments, 1,
                                              "UInt32", JSMN_STRING,
                                              outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.08
parser_error_t parser_getItemStakeUnstakedTokens(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Stake Unstaked Tokens");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgumentOptionalDelegatorID(&parser_tx_obj.arguments, 1,
                                              "UInt32", JSMN_STRING,
                                              outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.09
parser_error_t parser_getItemUnstakeAll(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Unstake All");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 3;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.10
parser_error_t parser_getItemWithdrawRewardTokensSCO(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw Reward Tokens");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgumentOptionalDelegatorID(&parser_tx_obj.arguments, 1,
                                              "UInt32", JSMN_STRING,
                                              outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.11
parser_error_t parser_getItemWithdrawUnstakedTokensSCO(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw Unstaked Tokens");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgumentOptionalDelegatorID(&parser_tx_obj.arguments, 1,
                                              "UInt32", JSMN_STRING,
                                              outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Amount");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "UFix64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.12
parser_error_t parser_getItemCloseStake(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Close Stake");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgumentOptionalDelegatorID(&parser_tx_obj.arguments, 1,
                                              "UInt32", JSMN_STRING,
                                              outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.13
parser_error_t parser_getItemTransferNode(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Transfer Node");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Address");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "Address", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.14
parser_error_t parser_getItemTransferDelegator(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Transfer Delegator");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Delegator ID");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "UInt32", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 4:
            snprintf(outKey, outKeyLen, "Address");
            return parser_printArgument(&parser_tx_obj.arguments, 2,
                                        "Address", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 5;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.15
parser_error_t parser_getItemWithdrawFromMachineAccount(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Withdraw From Machine Account");
            return PARSER_OK;
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
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//SCO.16
parser_error_t parser_getItemUpdateNetworkingAddressSCO(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Update Networking Address");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Node ID");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[0],
                                              outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Address");
            return parser_printArgumentString(&parser_tx_obj.arguments.argCtx[1],
                                              outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//FUSD.01
parser_error_t parser_getItemSetupFUSDVault(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Setup FUSD Vault");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 2;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//FUSD.02
parser_error_t parser_getItemTransferFUSD(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Transfer FUSD");
            return PARSER_OK;
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
            snprintf(outKey, outKeyLen, "Recipient");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "Address", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//TS.01
parser_error_t parser_getItemSetUpTopShotCollection(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Set Up Top Shot Collection");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 2;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}

//TS.02
parser_error_t parser_getItemTransferTopShotMoment(const parser_context_t *ctx,
                                       uint16_t displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    *pageCount = 1;
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Type");
            snprintf(outVal, outValLen, "Transfer Top Shot Moment");
            return PARSER_OK;
        case 1:
            snprintf(outKey, outKeyLen, "ChainID");
            return parser_printChainID(&parser_tx_obj.payer,
                                       outVal, outValLen, pageIdx, pageCount);
        case 2:
            snprintf(outKey, outKeyLen, "Moment ID");
            return parser_printArgument(&parser_tx_obj.arguments, 0,
                                        "UInt64", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        case 3:
            snprintf(outKey, outKeyLen, "Address");
            return parser_printArgument(&parser_tx_obj.arguments, 1,
                                        "Address", JSMN_STRING,
                                        outVal, outValLen, pageIdx, pageCount);
        default:
            break;
    }
    displayIdx -= 4;
    return parser_getItemAfterArguments(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
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
        return PARSER_NO_DATA;
    }
    *pageCount = 1;

    switch (parser_tx_obj.script.type) {
        case SCRIPT_UNKNOWN:
            return PARSER_UNEXPECTED_SCRIPT;
        case SCRIPT_TOKEN_TRANSFER:
            return parser_getItemTokenTransfer(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx,
                                               pageCount);
        case SCRIPT_CREATE_ACCOUNT:
            return parser_getItemCreateAccount(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx,
                                               pageCount);
        case SCRIPT_ADD_NEW_KEY:
            return parser_getItemAddNewKey(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx,
                                           pageCount);
        case SCRIPT_TH01_WITHDRAW_UNLOCKED_TOKENS:
            return parser_getItemWithdrawUnlockedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen, 
                                                        pageIdx, pageCount);
        case SCRIPT_TH02_DEPOSIT_UNLOCKED_TOKENS:
            return parser_getItemDepositUnlockedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                       pageIdx, pageCount);
        case SCRIPT_TH06_REGISTER_NODE:
            return parser_getItemRegisterNode(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                              pageIdx, pageCount);
        case SCRIPT_TH08_STAKE_NEW_TOKENS:
            return parser_getItemStakeNewTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                pageIdx, pageCount);
        case SCRIPT_TH09_RESTAKE_UNSTAKED_TOKENS:
            return parser_getItemRestakeUnstakedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                       pageIdx, pageCount);
        case SCRIPT_TH10_RESTAKE_REWARDED_TOKENS:
            return parser_getItemRestakeRewardedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                       pageIdx, pageCount);
        case SCRIPT_TH11_UNSTAKE_TOKENS:
            return parser_getItemUnstakeTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                               pageIdx, pageCount);
        case SCRIPT_TH12_UNSTAKE_ALL_TOKENS:
            return parser_getItemUnstakeAllTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                  pageIdx, pageCount);
        case SCRIPT_TH13_WITHDRAW_UNSTAKED_TOKENS:
            return parser_getItemWithdrawUnstakedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                        pageIdx, pageCount);
        case SCRIPT_TH14_WITHDRAW_REWARDED_TOKENS:
            return parser_getItemWithdrawRewardedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                        pageIdx, pageCount);
        case SCRIPT_TH16_REGISTER_OPERATOR_NODE:
            return parser_getItemRegisterOperatorNode(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                      pageIdx, pageCount);
        case SCRIPT_TH17_REGISTER_DELEGATOR:
            return parser_getItemRegisterDelegator(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                   pageIdx, pageCount);
        case SCRIPT_TH19_DELEGATE_NEW_TOKENS:
            return parser_getItemDelegateNewTokens(ctx, displayIdx, outKey, outKeyLen, outVal, outValLen,
                                                   pageIdx, pageCount);
        case SCRIPT_TH20_RESTAKE_UNSTAKED_DELEGATED_TOKENS:
            return parser_getItemRestakeUnstakedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                outValLen, pageIdx, pageCount);
        case SCRIPT_TH21_RESTAKE_REWARDED_DELEGATED_TOKENS:
            return parser_getItemRestakeRewardedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                outValLen, pageIdx, pageCount);
        case SCRIPT_TH22_UNSTAKE_DELEGATED_TOKENS:
            return parser_getItemUnstakeDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                        outValLen, pageIdx, pageCount);
        case SCRIPT_TH23_WITHDRAW_UNSTAKED_DELEGATED_TOKENS:
            return parser_getItemWithdrawUnstakedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_TH24_WITHDRAW_REWARDED_DELEGATED_TOKENS:
            return parser_getItemWithdrawRewardedDelegatedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_TH25_UPDATE_NETWORKING_ADDRESS:
            return parser_getItemUpdateNetworkingAddress(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO01_SETUP_STAKING_COLLECTION:
            return parser_getItemSetupStaingCollection(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO02_REGISTER_DELEGATOR:
            return parser_getItemRegisterDelegatorSCO(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO03_REGISTER_NODE:
            return parser_getItemRegisterNodeSCO(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO04_CREATE_MACHINE_ACCOUNT:
            return parser_getItemCreateMachineAccount(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO05_REQUEST_UNSTAKING:
            return parser_getItemRequestUnstaking(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO06_STAKE_NEW_TOKENS:
            return parser_getItemStakeNewTokensSCO(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO07_STAKE_REWARD_TOKENS:
            return parser_getItemStakeRewardTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO08_STAKE_UNSTAKED_TOKENS:
            return parser_getItemStakeUnstakedTokens(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO09_UNSTAKE_ALL:
            return parser_getItemUnstakeAll(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO10_WITHDRAW_REWARD_TOKENS:                   
            return parser_getItemWithdrawRewardTokensSCO(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO11_WITHDRAW_UNSTAKED_TOKENS:
            return parser_getItemWithdrawUnstakedTokensSCO(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO12_CLOSE_STAKE:
            return parser_getItemCloseStake(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO13_TRANSFER_NODE:
            return parser_getItemTransferNode(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO14_TRANSFER_DELEGATOR:
            return parser_getItemTransferDelegator(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO15_WITHDRAW_FROM_MACHINE_ACCOUNT:
            return parser_getItemWithdrawFromMachineAccount(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_SCO16_UPDATE_NETWORKING_ADDRESS:
            return parser_getItemUpdateNetworkingAddressSCO(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_FUSD01_SETUP_FUSD_VAULT:
            return parser_getItemSetupFUSDVault(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_FUSD02_TRANSFER_FUSD:
            return parser_getItemTransferFUSD(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_TS01_SET_UP_TOPSHOT_COLLECTION:
            return parser_getItemSetUpTopShotCollection(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
        case SCRIPT_TS02_TRANSFER_TOP_SHOT_MOMENT:
            return parser_getItemTransferTopShotMoment(ctx, displayIdx, outKey, outKeyLen, outVal, 
                                                                 outValLen, pageIdx, pageCount);
    }

    return PARSER_UNEXPECTED_SCRIPT;
}
