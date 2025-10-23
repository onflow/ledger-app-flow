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

#include "tx.h"
#include "apdu_codes.h"
#include "buffering.h"
#include "parser_tx.h"
#include <string.h>
#include "zxmacros.h"
#include "app_mode.h"

#define RAM_BUFFER_SIZE   8192
#define FLASH_BUFFER_SIZE 16384

// Ram
uint8_t ram_buffer[RAM_BUFFER_SIZE];

// Flash
typedef struct {
    uint8_t buffer[FLASH_BUFFER_SIZE];
} storage_t;

#if defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2) || \
    defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX_P)
storage_t NV_CONST N_appdata_impl __attribute__((aligned(64)));
#define N_appdata (*(NV_VOLATILE storage_t *) PIC(&N_appdata_impl))
#endif

parser_context_t ctx_parsed_tx;

// UTF-8 encoding of "FLOW-V0.0-transaction" padded with zeros to 32 bytes
#define DOMAIN_TAG_LENGTH 32
const uint8_t TX_DOMAIN_TAG[DOMAIN_TAG_LENGTH] = {
    0x46, 0x4C, 0x4F, 0x57, 0x2D, 0x56, 0x30, 0x2E, 0x30, 0x2D, 0x74, 0x72, 0x61, 0x6E, 0x73, 0x61,
    0x63, 0x74, 0x69, 0x6F, 0x6E, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};

void tx_initialize() {
    buffering_init(ram_buffer, sizeof(ram_buffer), (uint8_t *) N_appdata.buffer, FLASH_BUFFER_SIZE);
}

void tx_reset() {
    buffering_reset();
}

uint32_t tx_append(unsigned char *buffer, uint32_t length) {
    return buffering_append(buffer, length);
}

uint32_t tx_get_buffer_length() {
    return buffering_get_buffer()->pos;
}

uint8_t *tx_get_buffer() {
    return buffering_get_buffer()->data;
}

const char *tx_parse(process_chunk_response_t typeOfCall) {
    // parse tx
    uint8_t err = parser_parse(&ctx_parsed_tx, tx_get_buffer(), tx_get_buffer_length());

    if (err != PARSER_OK) {
        ZEMU_TRACE();
        return parser_getErrorDescription(err);
    }

    // parse metadata
    parser_tx_obj.metadataInitialized = false;
    switch (typeOfCall) {
        case PROCESS_CHUNK_FINISHED_WITH_METADATA:
            MEMZERO(&parser_tx_obj.metadata, sizeof(parser_tx_obj.metadata));
            err = parseTxMetadata(parser_tx_obj.hash.digest, &parser_tx_obj.metadata);
            if (err != PARSER_OK) {
                ZEMU_TRACE();
                return parser_getErrorDescription(err);
            }
            parser_tx_obj.metadataInitialized = true;
            break;
        case PROCESS_CHUNK_FINISHED_NO_METADATA:
            if (!app_mode_expert()) {  // we do not need metadata for these scripts, but this
                                       // workflow should work only in expert mode
                ZEMU_TRACE();
                return parser_getErrorDescription(PARSER_UNEXPECTED_SCRIPT);
            }
            break;
        default:
            ZEMU_TRACE();
            return parser_getErrorDescription(PARSER_UNEXPECTED_ERROR);
    }

    CHECK_APP_CANARY()

    // validate
    err = parser_validate(&ctx_parsed_tx);
    if (err != PARSER_OK) {
        ZEMU_TRACE();
        return parser_getErrorDescription(err);
    }

    return NULL;
}

zxerr_t tx_getNumItems(uint8_t *num_items) {
    parser_error_t err = parser_getNumItems(&ctx_parsed_tx, num_items);

    if (err != PARSER_OK) {
        return zxerr_no_data;
    }

    return zxerr_ok;
}

zxerr_t tx_getItem(int8_t displayIdx,
                   char *outKey,
                   uint16_t outKeyLen,
                   char *outVal,
                   uint16_t outValLen,
                   uint8_t pageIdx,
                   uint8_t *pageCount) {
    uint8_t numItems = 0;

    CHECK_ZXERR(tx_getNumItems(&numItems))

    if (displayIdx < 0 || displayIdx > numItems) {
        return zxerr_no_data;
    }

    parser_error_t err = parser_getItem(&ctx_parsed_tx,
                                        displayIdx,
                                        outKey,
                                        outKeyLen,
                                        outVal,
                                        outValLen,
                                        pageIdx,
                                        pageCount);

    // Convert error codes
    if (err == PARSER_NO_DATA || err == PARSER_DISPLAY_IDX_OUT_OF_RANGE ||
        err == PARSER_DISPLAY_PAGE_OUT_OF_RANGE)
        return zxerr_no_data;

    if (err != PARSER_OK) return zxerr_unknown;

    return zxerr_ok;
}
