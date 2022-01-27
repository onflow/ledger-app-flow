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

#include <stdio.h>
#include "coin.h"
#include "zxerror.h"
#include "zxmacros.h"
#include "zxformat.h"
#include "app_mode.h"
#include "crypto.h"
#include "addr.h"

show_addres_t show_address;
uint8_t pubkey_to_display[SECP256_PK_LEN];
flow_account_t address_to_display;

__Z_INLINE bool is_pubkey(show_addres_t show_address) {
    return (show_address != show_address_no_pubkey) && (show_address != show_address_error);
}

zxerr_t addr_getNumItems(uint8_t *num_items) {
    zemu_log_stack("addr_getNumItems");
    *num_items = 1;
    if (is_pubkey(show_address)) {
        *num_items += 1;
    }
    if (app_mode_expert() && is_pubkey(show_address)) {
        *num_items += 1;
    }
    if (show_address == show_address_yes) {
        *num_items += 2;
    }
    return zxerr_ok;
}

zxerr_t addr_getItem(int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outVal, uint16_t outValLen,
                     uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("addr_getItem");

    if (is_pubkey(show_address) && displayIdx--==0) {
            snprintf(outKey, outKeyLen, "Pub Key");
            // +1 is to skip 0x04 prefix that indicates uncompresed key 
            pageHexString(outVal, outValLen, pubkey_to_display+1, sizeof(pubkey_to_display)-1, pageIdx, pageCount);
            return zxerr_ok;
    }

    if (displayIdx--==0) {
            switch(show_address) {
                case show_address_error:
                    snprintf(outKey, outKeyLen, "Error");
                    pageString(outVal, outValLen, "deriving public key.", pageIdx, pageCount);
                    return zxerr_ok;
                case show_address_no_pubkey:
                    snprintf(outKey, outKeyLen, "Account data");
                    pageString(outVal, outValLen, "not saved on the device.", pageIdx, pageCount);
                    return zxerr_ok;
                case show_address_empty_slot:
                    snprintf(outKey, outKeyLen, "Address:");
                    pageString(outVal, outValLen, "Not saved on the device.", pageIdx, pageCount);
                    return zxerr_ok;
                case show_address_hdpaths_not_equal:
                    snprintf(outKey, outKeyLen, "Address:");
                    pageString(outVal, outValLen, "Other path is saved on the device.", pageIdx, pageCount);
                    return zxerr_ok;
                case show_address_yes:
                    snprintf(outKey, outKeyLen, "Address:");
                    pageHexString(outVal, outValLen, address_to_display.data, sizeof(address_to_display.data), pageIdx, pageCount);
                    return zxerr_ok;
                default:
                    return zxerr_no_data;
            }
    }

    if (show_address == show_address_yes && displayIdx--==0) {
        snprintf(outKey, outKeyLen, "Verify if this");
        snprintf(outVal, outValLen, " public key was   added to");
        return zxerr_ok;
    }

    if (show_address == show_address_yes && displayIdx--==0) {
        array_to_hexstr(outKey, outKeyLen, address_to_display.data, sizeof(address_to_display.data)); 
#if defined(TARGET_NANOX)
        snprintf(outVal, outValLen, " using any Flow blockchain explorer.");
#else
        snprintf(outVal, outValLen, " using any Flow  blockch. explorer.");
#endif
        return zxerr_ok;
    }

    if (app_mode_expert() && is_pubkey(show_address) && displayIdx--==0) {
            snprintf(outKey, outKeyLen, "Your Path");
            char buffer[300];
            bip32_to_str(buffer, sizeof(buffer), hdPath.data, HDPATH_LEN_DEFAULT);
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);
            return zxerr_ok;
    }
    return zxerr_no_data;
}
