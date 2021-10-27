/*******************************************************************************
*   (c) 2020 Zondax GmbH
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
#include "account.h"
#include "zxmacros.h"
#include "stdbool.h"
#include "apdu_codes.h"

typedef enum {
    SLOT_OP_SET,
    SLOT_OP_UPDATE,
    SLOT_UP_DELETE
} slotop_t;

account_slot_t tmp_slot;
uint8_t tmp_slotIdx;
slotop_t tmp_slotop;

slot_store_t NV_CONST
N_slot_store_impl __attribute__ ((aligned(64)));
#define N_slot_store (*(NV_VOLATILE slot_store_t *)PIC(&N_slot_store_impl))

uint8_t slot_is_empty(const account_slot_t *tmp) {
    return tmp->path.data[0] == 0;
}

zxerr_t slot_getNumItems(uint8_t *num_items) {
    *num_items = 0;
    switch (tmp_slotop) {
        case SLOT_OP_SET: {
            *num_items = 3;
            return zxerr_ok;
        }
        case SLOT_OP_UPDATE: {
            *num_items = 5;
            return zxerr_ok;
        }
        case SLOT_UP_DELETE: {
            *num_items = 3;
            return zxerr_ok;
        }
    }
    return zxerr_no_data;
}

zxerr_t slot_getItem(int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outVal, uint16_t outValLen,
                     uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("slot_getItem");
    *pageCount = 1;

    switch (tmp_slotop) {
        case SLOT_OP_SET: {
            switch (displayIdx) {
                case 0: {
                    snprintf(outKey, outKeyLen, "Set");
                    snprintf(outVal, outValLen, "Account %d", tmp_slotIdx);
                    return zxerr_ok;
                }
                case 1: {
                    snprintf(outKey, outKeyLen, "Account");
                    array_to_hexstr(outVal, outValLen, tmp_slot.account.data, 8);
                    return zxerr_ok;
                }
                case 2: {
                    char bufferUI[130];
                    snprintf(outKey, outKeyLen, "Path");
                    bip32_to_str(bufferUI, sizeof(bufferUI), tmp_slot.path.data, HDPATH_LEN_DEFAULT);
                    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);
                    return zxerr_ok;
                }
                default:
                    return zxerr_no_data;
            }
        }
        case SLOT_OP_UPDATE: {
            const account_slot_t *oldSlot = &N_slot_store.slot[tmp_slotIdx];
            switch (displayIdx) {
                case 0: {
                    snprintf(outKey, outKeyLen, "Update");
                    snprintf(outVal, outValLen, "Account %d", tmp_slotIdx);
                    return zxerr_ok;
                }
                case 1: {
                    snprintf(outKey, outKeyLen, "Old Account");
                    array_to_hexstr(outVal, outValLen, oldSlot->account.data, 8);
                    return zxerr_ok;
                }
                case 2: {
                    char bufferUI[130];
                    snprintf(outKey, outKeyLen, "Old Path");
                    bip32_to_str(bufferUI, sizeof(bufferUI), oldSlot->path.data, HDPATH_LEN_DEFAULT);
                    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);
                    return zxerr_ok;
                }
                case 3: {
                    snprintf(outKey, outKeyLen, "New Account");
                    array_to_hexstr(outVal, outValLen, tmp_slot.account.data, 8);
                    return zxerr_ok;
                }
                case 4: {
                    char bufferUI[130];
                    snprintf(outKey, outKeyLen, "New Path");
                    bip32_to_str(bufferUI, sizeof(bufferUI), tmp_slot.path.data, HDPATH_LEN_DEFAULT);
                    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);
                    return zxerr_ok;
                }
                default:
                    return zxerr_no_data;
            }
            break;
        }
        case SLOT_UP_DELETE: {
            const account_slot_t *oldSlot = &N_slot_store.slot[tmp_slotIdx];
            switch (displayIdx) {
                case 0: {
                    snprintf(outKey, outKeyLen, "Delete");
                    snprintf(outVal, outValLen, "Account %d", tmp_slotIdx);
                    return zxerr_ok;
                }
                case 1: {
                    snprintf(outKey, outKeyLen, "Old Account");
                    array_to_hexstr(outVal, outValLen, oldSlot->account.data, 8);
                    return zxerr_ok;
                }
                case 2: {
                    char bufferUI[130];
                    snprintf(outKey, outKeyLen, "Old Path");
                    bip32_to_str(bufferUI, sizeof(bufferUI), oldSlot->path.data, HDPATH_LEN_DEFAULT);
                    pageString(outVal, outValLen, bufferUI, pageIdx, pageCount);
                    return zxerr_ok;
                }
                default:
                    return zxerr_no_data;
            }
            break;
        }
    }
}

zxerr_t slot_status(uint8_t *out, uint16_t outLen) {
    if (outLen != SLOT_COUNT) {
        return zxerr_out_of_bounds;
    }

    MEMZERO(out, outLen);
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        const account_slot_t *tmp = &N_slot_store.slot[i];
        if (!slot_is_empty(tmp)) {
            out[i] = 1;
        }
    }

    return zxerr_ok;
}

zxerr_t slot_getSlot(uint8_t slotIndex, uint8_t *out, uint16_t outLen) {
    if (outLen < sizeof(account_slot_t)) {
        return zxerr_buffer_too_small;
    }

    if (slotIndex >= SLOT_COUNT) {
        return zxerr_out_of_bounds;
    }

    // Check if empty
    const account_slot_t *tmp = &N_slot_store.slot[slotIndex];
    if (tmp->path.data[0] == 0) {
        return zxerr_no_data;
    }

    // Copy slot
    MEMCPY(out, &N_slot_store.slot[slotIndex], sizeof(account_slot_t));

    return zxerr_ok;
}

zxerr_t slot_parseSlot(uint8_t *buffer, uint16_t bufferLen) {
    zemu_log_stack("slot_parseSlot");
    char bufferUI[50];

    if (bufferLen != 1 + sizeof(account_slot_t)) {
        snprintf(bufferUI, sizeof(bufferUI), "dataLen does not match: %d", bufferLen);
        zemu_log_stack(bufferUI);
        return zxerr_encoding_failed;
    }

    tmp_slotIdx = buffer[0];
    if (tmp_slotIdx >= SLOT_COUNT) {
        zemu_log_stack("slot index too big");
        return zxerr_out_of_bounds;
    }

    MEMCPY(&tmp_slot, buffer + 1, sizeof(account_slot_t));

    const bool mainnet = tmp_slot.path.data[0] == HDPATH_0_DEFAULT && tmp_slot.path.data[1] == HDPATH_1_DEFAULT;
    const bool testnet = tmp_slot.path.data[0] == HDPATH_0_TESTNET && tmp_slot.path.data[1] == HDPATH_1_TESTNET;
    const bool empty = tmp_slot.path.data[0] == 0 && tmp_slot.path.data[1] == 0;

    if (!mainnet && !testnet && !empty) {
        array_to_hexstr(bufferUI, sizeof(bufferUI), tmp_slot.account.data, 8);
        zemu_log(bufferUI);
        zemu_log("\n");

        bip32_to_str(bufferUI, sizeof(bufferUI), tmp_slot.path.data, 5);
        zemu_log(bufferUI);
        zemu_log("\n");

        zemu_log_stack("invalid path");
        return zxerr_out_of_bounds;
    }

    tmp_slotop = SLOT_OP_UPDATE;
    if (slot_is_empty(&N_slot_store.slot[tmp_slotIdx])) {
        tmp_slotop = SLOT_OP_SET;
    } else if (slot_is_empty(&tmp_slot)) {
        tmp_slotop = SLOT_UP_DELETE;
    }

    return zxerr_ok;
}

void app_slot_setSlot() {
    MEMCPY_NV(&N_slot_store.slot[tmp_slotIdx], &tmp_slot, sizeof(account_slot_t));
    set_code(G_io_apdu_buffer, 0, APDU_CODE_OK);
    io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, 2);
}
