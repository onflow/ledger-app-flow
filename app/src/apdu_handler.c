/*******************************************************************************
*   (c) 2018, 2019 Zondax GmbH
*   (c) 2016 Ledger
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

#include "app_main.h"

#include <string.h>
#include <os_io_seproxyhal.h>
#include <os.h>

#include "view.h"
#include "actions.h"
#include "tx.h"
#include "addr.h"
#include "crypto.h"
#include "coin.h"
#include "account.h"
#include "zxmacros.h"
#include "zxformat.h"

__Z_INLINE void handleGetPubkey(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    extractHDPath(rx, OFFSET_DATA);

    uint8_t requireConfirmation = G_io_apdu_buffer[OFFSET_P1];

    if (requireConfirmation) {
        app_fill_address();

        account_slot_t slot;
        zxerr_t err = slot_getSlot(MAIN_SLOT, (uint8_t *) &slot, sizeof(slot));

        if (!(err == zxerr_no_data  || err == zxerr_ok)) {
            zemu_log_stack("Unknown slot error");
            THROW(APDU_CODE_UNKNOWN);
        }

        //Case 1 Empty slot 0 
        if (err == zxerr_no_data) {
            show_address = show_address_empty_slot;
        }
        else {
            //Case 2 Slot 0 derivation path is not the same as APDU derivation path
            if (memcmp(slot.path.data, hdPath, sizeof(hdPath))) {
                show_address = show_address_hdpaths_not_equal;            
            }
            //Case 3 Everything is OK
            else {
                show_address = show_address_yes;

                uint32_t dstLen = sizeof(G_io_apdu_buffer) - ADDRESS_OFFSET;
                if (dstLen < 2*ADDRESS_LENGTH + 1) { //+1 for terminating 0
                    zemu_log_stack("G_io_apdu_buffer unexpectedly short.");
                    THROW(APDU_CODE_UNKNOWN);
                }
         
                uint32_t len = array_to_hexstr((char *)(G_io_apdu_buffer + ADDRESS_OFFSET), dstLen, slot.account.data, sizeof(slot.account.data));

                if (len != 2*ADDRESS_LENGTH) {
                    zemu_log_stack("Address conversion error.");
                    THROW(APDU_CODE_EXECUTION_ERROR+len);
                }
            }
        }

        view_review_init(addr_getItem, addr_getNumItems, app_reply_address);
        view_review_show();

        *flags |= IO_ASYNCH_REPLY;
        return;
    }

    *tx = app_fill_address();
    THROW(APDU_CODE_OK);
}

__Z_INLINE void handleSign(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    if (!process_chunk(tx, rx)) {
        THROW(APDU_CODE_OK);
    }

    const char *error_msg = tx_parse();

    if (error_msg != NULL) {
        int error_msg_length = strlen(error_msg);
        MEMCPY(G_io_apdu_buffer, error_msg, error_msg_length);
        *tx += (error_msg_length);
        THROW(APDU_CODE_DATA_INVALID);
    }

    CHECK_APP_CANARY()
    view_review_init(tx_getItem, tx_getNumItems, app_sign);
    view_review_show();
    *flags |= IO_ASYNCH_REPLY;
}

__Z_INLINE void handleSlotStatus(__Z_UNUSED volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    if (rx != 5) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    if (slot_status(G_io_apdu_buffer, SLOT_COUNT) != zxerr_ok) {
        THROW(APDU_CODE_EXECUTION_ERROR);
    }
    *tx = SLOT_COUNT;
    THROW(APDU_CODE_OK);
}

__Z_INLINE void handleGetSlot(__Z_UNUSED volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    if (rx != 6) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    const uint8_t slotIdx = G_io_apdu_buffer[OFFSET_DATA];

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%d", slotIdx);
    zemu_log_stack(buffer);

    zxerr_t err = slot_getSlot(slotIdx, G_io_apdu_buffer, IO_APDU_BUFFER_SIZE);
    snprintf(buffer, sizeof(buffer), "err: %d", err);
    zemu_log_stack(buffer);

    if (err == zxerr_no_data) {
        zemu_log_stack("Empty slot");
        THROW(APDU_CODE_EMPTY_BUFFER);
    }

    if (err != zxerr_ok) {
        THROW(APDU_CODE_EXECUTION_ERROR);
    }

    *tx = sizeof(account_slot_t);
    THROW(APDU_CODE_OK);
}

__Z_INLINE void handleSetSlot(volatile uint32_t *flags, __Z_UNUSED volatile uint32_t *tx, uint32_t rx) {
    if (rx != 5 + 1 + 8 + 20) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    zxerr_t err = slot_parseSlot(G_io_apdu_buffer + OFFSET_DATA, rx - OFFSET_DATA);
    if (err != zxerr_ok) {
        THROW(APDU_CODE_DATA_INVALID);
    }

    view_review_init(slot_getItem, slot_getNumItems, app_slot_setSlot);
    view_review_show();
    *flags |= IO_ASYNCH_REPLY;
}

void handleApdu(volatile uint32_t *flags, volatile uint32_t *tx, uint32_t rx) {
    uint16_t sw = 0;

    BEGIN_TRY
    {
        TRY
        {
            if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
                THROW(APDU_CODE_CLA_NOT_SUPPORTED);
            }

            if (rx < APDU_MIN_LENGTH) {
                THROW(APDU_CODE_WRONG_LENGTH);
            }

            switch (G_io_apdu_buffer[OFFSET_INS]) {
                case INS_GET_VERSION: {
                    handle_getversion(flags, tx, rx);
                    break;
                }

                case INS_GET_PUBKEY: {
                    handleGetPubkey(flags, tx, rx);
                    break;
                }

                case INS_SIGN: {
                    handleSign(flags, tx, rx);
                    break;
                }

                case INS_SLOT_STATUS: {
                    handleSlotStatus(flags, tx, rx);
                    break;
                }

                case INS_SLOT_GET: {
                    handleGetSlot(flags, tx, rx);
                    break;
                }

                case INS_SLOT_SET: {
                    handleSetSlot(flags, tx, rx);
                    break;
                }

                default:
                    THROW(APDU_CODE_INS_NOT_SUPPORTED);
            }
        }
        CATCH(EXCEPTION_IO_RESET)
        {
            THROW(EXCEPTION_IO_RESET);
        }
        CATCH_OTHER(e)
        {
            switch (e & 0xF000) {
                case 0x6000:
                case APDU_CODE_OK:
                    sw = e;
                    break;
                default:
                    sw = 0x6800 | (e & 0x7FF);
                    break;
            }
            G_io_apdu_buffer[*tx] = sw >> 8;
            G_io_apdu_buffer[*tx + 1] = sw;
            *tx += 2;
        }
        FINALLY
        {
        }
    }
    END_TRY;
}
