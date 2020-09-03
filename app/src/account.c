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

slot_store_t NV_CONST
N_slot_store_impl __attribute__ ((aligned(64)));
#define N_slot_store (*(NV_VOLATILE slot_store_t *)PIC(&N_slot_store_impl))

zxerr_t slot_status(uint8_t *out, uint16_t outLen) {
    if (outLen != SLOT_COUNT) {
        return zxerr_out_of_bounds;
    }

    MEMZERO(out, outLen);
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        const account_slot_t *tmp = &N_slot_store.slot[i];
        if (tmp->path.data[0] != 0) {
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

zxerr_t slot_setSlot(uint8_t slotIndex, uint8_t *data, uint16_t dataLen) {
    zemu_log_stack("slot_setSlot");

    if (dataLen != sizeof(account_slot_t)) {
        zemu_log_stack("dataLen does not match");
        return zxerr_encoding_failed;
    }

    if (slotIndex >= SLOT_COUNT) {
        zemu_log_stack("slot index too big");
        return zxerr_out_of_bounds;
    }

    account_slot_t tmp;
    MEMCPY(&tmp, data, sizeof(account_slot_t));

    zemu_log_stack("tmp copied");

    const bool mainnet = tmp.path.data[0] == HDPATH_0_DEFAULT && tmp.path.data[1] == HDPATH_1_DEFAULT;
    const bool testnet = tmp.path.data[0] == HDPATH_0_TESTNET && tmp.path.data[1] == HDPATH_1_TESTNET;
    const bool empty = tmp.path.data[0] == 0 && tmp.path.data[1] == 0;

    if (!mainnet && !testnet && !empty) {
        char buffer[50];
        array_to_hexstr(buffer, sizeof(buffer), tmp.account.data, 8);
        zemu_log(buffer);
        zemu_log("\n");

        bip32_to_str(buffer, sizeof(buffer), tmp.path.data, 5);
        zemu_log(buffer);
        zemu_log("\n");

        zemu_log_stack("invalid path");
        return zxerr_out_of_bounds;
    }

    zemu_log_stack("storing in nvram");
    MEMCPY_NV(&N_slot_store.slot[slotIndex], &tmp, sizeof(account_slot_t));

    zemu_log_stack("stored");
    return zxerr_ok;
}
