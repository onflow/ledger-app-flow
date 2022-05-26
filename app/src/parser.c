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
#include "hdpath.h"
#include "app_mode.h"

#define FLOW_PUBLIC_KEY_SIZE 64     // 64 bytes for public key
#define FLOW_SIG_ALGO_SIZE 1        // 8 bits for signature algorithm (uint8)
#define FLOW_HASH_ALGO_SIZE 1       // 8 bits for hash algorithm (uint8)
#define FLOW_WEIGHT_SIZE 2          // 16 bits for weight (uint16)
#define RLP_PREFIX 1

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
                                   __Z_UNUSED uint8_t pageIdx, uint8_t *pageCount) {
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

parser_error_t parser_printArgument(const flow_argument_list_t *v, uint8_t argIndex, 
                                    const char *expectedType, jsmntype_t jsonType,
                                    char *outVal, uint16_t outValLen,
                                    uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    if (argIndex >= v->argCount) {
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) v->argCtx[argIndex].buffer, v->argCtx[argIndex].bufferLen));

    char bufferUI[ARGUMENT_BUFFER_SIZE_STRING];
    uint16_t valueTokenIndex;

    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, 0, expectedType, jsonType, &valueTokenIndex))
    CHECK_PARSER_ERR(json_extractToken(bufferUI, sizeof(bufferUI), &parsedJson, valueTokenIndex))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return PARSER_DISPLAY_PAGE_OUT_OF_RANGE;
    }

    return PARSER_OK;
}

parser_error_t parser_printOptionalArgument(const flow_argument_list_t *v, uint8_t argIndex,
                                            const char *expectedType, jsmntype_t jsonType,
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
        char bufferUI[ARGUMENT_BUFFER_SIZE_STRING];
        CHECK_PARSER_ERR(json_extractToken(bufferUI, sizeof(bufferUI), &parsedJson, valueTokenIndex))
        pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);
    }

    return PARSER_OK;
}

parser_error_t parser_printArgumentArray(const flow_argument_list_t *v, uint8_t argIndex, uint8_t arrayIndex,
                                         const char *expectedType, jsmntype_t jsonType,
                                         char *outVal, uint16_t outValLen,
                                         uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) v->argCtx[argIndex].buffer, v->argCtx[argIndex].bufferLen));

    // Estimate number of pages
    uint16_t internalTokenElementIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, 0, (char *) "Array", JSMN_ARRAY, &internalTokenElementIdx));
    uint16_t arrayTokenCount;
    CHECK_PARSER_ERR(array_get_element_count(&parsedJson, internalTokenElementIdx, &arrayTokenCount));
    if (arrayTokenCount > MAX_JSON_ARRAY_TOKEN_COUNT) {  //indirectly limits the maximum number of public keys
        return PARSER_UNEXPECTED_NUMBER_ITEMS;
    }

    uint16_t arrayElementToken;
    char bufferUI[ARGUMENT_BUFFER_SIZE_STRING];
    CHECK_PARSER_ERR(array_get_nth_element(&parsedJson, internalTokenElementIdx, arrayIndex, &arrayElementToken))
    uint16_t internalTokenElemIdx;
    CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, arrayElementToken, expectedType, jsonType, &internalTokenElemIdx))
    CHECK_PARSER_ERR(json_extractToken(bufferUI, sizeof(bufferUI), &parsedJson, internalTokenElemIdx))
    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);

    // Check requested page is in range
    if (pageIdx > *pageCount) {
        return PARSER_DISPLAY_PAGE_OUT_OF_RANGE;
    }

    return PARSER_OK;
}

parser_error_t parser_printArgumentOptionalArray(const flow_argument_list_t *v, uint8_t argIndex, uint8_t arrayIndex,
                                                 const char *expectedType, jsmntype_t jsonType,
                                                 char *outVal, uint16_t outValLen,
                                                 uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outVal, outValLen);

    parsed_json_t parsedJson = {false};
    CHECK_PARSER_ERR(json_parse(&parsedJson, (char *) v->argCtx[argIndex].buffer, v->argCtx[argIndex].bufferLen));

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

        uint16_t arrayElementToken;
        char bufferUI[ARGUMENT_BUFFER_SIZE_STRING];
        CHECK_PARSER_ERR(array_get_nth_element(&parsedJson, internalTokenElementIdx, arrayIndex, &arrayElementToken))
        uint16_t internalTokenElemIdx;
        CHECK_PARSER_ERR(json_matchKeyValue(&parsedJson, arrayElementToken, expectedType, jsonType, &internalTokenElemIdx))
        CHECK_PARSER_ERR(json_extractToken(bufferUI, sizeof(bufferUI), &parsedJson, internalTokenElemIdx))
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


#define CREATE_ACCOUNT_MAX_PUB_KEYS 5
#define SCO03_REGISTER_NODE_MAX_PUB_KEYS 3

parser_error_t parser_getItem_internal(__Z_UNUSED const parser_context_t *ctx, int8_t *displayIdx,
                                       char *outKey, uint16_t outKeyLen,
                                       char *outVal, uint16_t outValLen,
                                       uint8_t pageIdx, uint8_t *pageCount) {
    MEMZERO(outKey, outKeyLen);
    MEMZERO(outVal, outValLen);
    snprintf(outKey, outKeyLen, "? %d", *displayIdx);
    snprintf(outVal, outValLen, "?");

    #define SCREEN(condition) if ((condition) && ((*displayIdx)--==0) && pageCount && (*pageCount = 1))

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "Type");
        snprintf(outVal, outValLen, "%s", parser_tx_obj.metadata.txName);
        return PARSER_OK;
    }

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "ChainID");
        return parser_printChainID(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
    }

    //Arguments
    uint8_t screenCount = 0;
    for(size_t i=0; i<parser_tx_obj.metadata.argCount; i++) {
        parsed_tx_metadata_argument_t *marg = &parser_tx_obj.metadata.arguments[i];
        switch(marg->argumentType) {
            case ARGUMENT_TYPE_NORMAL:
                SCREEN(true) {
                    snprintf(outKey, outKeyLen, "%s", marg->displayKey);
                    return parser_printArgument(&parser_tx_obj.arguments, marg->argumentIndex, 
                                                marg->jsonExpectedType, marg->jsonExpectedKind,
                                                outVal, outValLen, pageIdx, pageCount);                
                }
                break;
            case ARGUMENT_TYPE_OPTIONAL:
                SCREEN(true) {
                    snprintf(outKey, outKeyLen, "%s", marg->displayKey);
                    return parser_printOptionalArgument(&parser_tx_obj.arguments, marg->argumentIndex, 
                                                        marg->jsonExpectedType, marg->jsonExpectedKind,
                                                        outVal, outValLen, pageIdx, pageCount);                
                }
                break;
            case ARGUMENT_TYPE_ARRAY:
                CHECK_PARSER_ERR(_countArgumentItems(&parser_tx_obj.arguments, marg->argumentIndex, 
                                                     marg->arrayMinElements, marg->arrayMaxElements, &screenCount));
                for(size_t j=0; j<screenCount; j++) {
                    SCREEN(true) {
                        snprintf(outKey, outKeyLen, "%s %d", marg->displayKey, (int)(j+1));
                        return parser_printArgumentArray(&parser_tx_obj.arguments, marg->argumentIndex, j,
                                                         marg->jsonExpectedType, marg->jsonExpectedKind,
                                                         outVal, outValLen, pageIdx, pageCount);

                    }
                }
                break;
            case ARGUMENT_TYPE_OPTIONALARRAY:
                CHECK_PARSER_ERR(_countArgumentOptionalItems(&parser_tx_obj.arguments, marg->argumentIndex, 
                                                             marg->arrayMinElements, marg->arrayMaxElements, &screenCount));
                for(size_t j=0; j<screenCount; j++) {
                    SCREEN(true) {
                        snprintf(outKey, outKeyLen, "%s %d", marg->displayKey, (int)(j+1));
                        return parser_printArgumentOptionalArray(&parser_tx_obj.arguments, marg->argumentIndex, j,
                                                                 marg->jsonExpectedType, marg->jsonExpectedKind,
                                                                 outVal, outValLen, pageIdx, pageCount);

                    }
                }
                break;
            default:
                return PARSER_METADATA_ERROR;
        }
    }

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "Ref Block");
        return parser_printBlockId(&parser_tx_obj.referenceBlockId, outVal, outValLen, pageIdx, pageCount);
    }

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "Gas Limit");
        return parser_printGasLimit(&parser_tx_obj.gasLimit, outVal, outValLen, pageIdx, pageCount);
    }

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "Prop Key Addr");
        return parser_printPropKeyAddr(&parser_tx_obj.proposalKeyAddress, outVal, outValLen, pageIdx, pageCount);
    }

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "Prop Key Id");
        return parser_printPropKeyId(&parser_tx_obj.proposalKeyId, outVal, outValLen, pageIdx, pageCount);
    }

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "Prop Key Seq Num");
        return parser_printPropSeqNum(&parser_tx_obj.proposalKeySequenceNumber, outVal, outValLen, pageIdx, pageCount);
    }

    SCREEN(true) {
        snprintf(outKey, outKeyLen, "Payer");
        return parser_printPayer(&parser_tx_obj.payer, outVal, outValLen, pageIdx, pageCount);
    }

    for(size_t i=0; i<parser_tx_obj.authorizers.authorizer_count; i++) {
        SCREEN(true) {
            snprintf(outKey, outKeyLen, "Authorizer %d", (int)(i+1));
            return parser_printAuthorizer(&parser_tx_obj.authorizers.authorizer[i], outVal, outValLen, pageIdx, pageCount);
        }
    }

    SCREEN(app_mode_expert()) {
        snprintf(outKey, outKeyLen, "Your Path");
        char buffer[100];
        path_options_to_string(buffer, sizeof(buffer), hdPath.data, HDPATH_LEN_DEFAULT, cryptoOptions); 
        pageString(outVal, outValLen, buffer, pageIdx, pageCount);
        return PARSER_OK;
    }

    switch(show_address) {
        case SHOW_ADDRESS_YES:
        case SHOW_ADDRESS_YES_HASH_MISMATCH:
            SCREEN(!addressUsedInTx) {
                snprintf(outKey, outKeyLen, "Warning:");
                snprintf(outVal, outValLen, "Incorrect address in transaction.");
                return PARSER_OK;
            }
            SCREEN(show_address == SHOW_ADDRESS_YES_HASH_MISMATCH) {
                snprintf(outKey, outKeyLen, "Warning:");
                #if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
                pageString(outVal, outValLen, "Specified hash algorithm does not match stored value.", pageIdx, pageCount);
                #else
                pageString(outVal, outValLen, " Specified hash   algorithm does  not match stored value.", pageIdx, pageCount);
                #endif
                return PARSER_OK;
            }
            break;
        case SHOW_ADDRESS_EMPTY_SLOT:
            SCREEN(true) {
                snprintf(outKey, outKeyLen, "Warning:");
                snprintf(outVal, outValLen, "No address stored on the device.");
                return PARSER_OK;
            }
            break; 
        case SHOW_ADDRESS_HDPATHS_NOT_EQUAL:
            SCREEN(true) {
                snprintf(outKey, outKeyLen, "Warning:");
                snprintf(outVal, outValLen, "Different address stored on device.");
                return PARSER_OK;
            }
            break; 
        default:
            SCREEN(true) {
                snprintf(outKey, outKeyLen, "Warning:");
                snprintf(outVal, outValLen, "Slot error.");
                return PARSER_OK;
            }
    }

    return PARSER_DISPLAY_IDX_OUT_OF_RANGE;

    #undef SCREEN
}

#define ARBITRARY_NEGATIVE_INTEGER -1
parser_error_t parser_getNumItems(const parser_context_t *ctx, uint8_t *num_items) {
    int8_t displays = ARBITRARY_NEGATIVE_INTEGER;
    parser_error_t err = parser_getItem_internal(ctx, &displays, NULL, 0, NULL, 0, 0, NULL);
    int8_t pages = ARBITRARY_NEGATIVE_INTEGER - displays;

    if (pages < 0) {
        *num_items = 0;
        return PARSER_UNEXPECTED_ERROR;
    }

    *num_items = (uint8_t) pages;

    if (err == PARSER_DISPLAY_IDX_OUT_OF_RANGE) {
        return PARSER_OK;
    }
    if (err == PARSER_OK) {
        return PARSER_UNEXPECTED_ERROR;
    }
    return err;
}

parser_error_t parser_getItem(const parser_context_t *ctx, uint8_t displayIdx,
                               char *outKey, uint16_t outKeyLen,
                               char *outVal, uint16_t outValLen,
                               uint8_t pageIdx, uint8_t *pageCount) {
    if (displayIdx > INT8_MAX) {
        return PARSER_DISPLAY_IDX_OUT_OF_RANGE;        
    }
    return parser_getItem_internal(ctx, (int8_t *) &displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}
