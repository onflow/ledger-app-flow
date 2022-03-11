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
#include "hdpath.h"

bool hasPubkey;
uint8_t pubkey_to_display[SECP256_PK_LEN];

//Small trick to avoid duplicated code here which was quite error prone:
//displayIdx is either the index of page to display, or may be a negative integer. 
//In the second case this is used to count the number of pages
//If displayIdx is negative, all other values are undefined
zxerr_t addr_getItem_internal(int8_t *displayIdx,
                                         char *outKey, uint16_t outKeyLen,
                                         char *outVal, uint16_t outValLen,
                                         uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("addr_getItem_internal");

    #define SCREEN(condition) if ((condition) && ((*displayIdx)--==0) && pageCount && (*pageCount = 1))

    SCREEN(hasPubkey) {
        snprintf(outKey, outKeyLen, "Pub Key");
        // +1 is to skip 0x04 prefix that indicates uncompresed key 
        pageHexString(outVal, outValLen, pubkey_to_display+1, sizeof(pubkey_to_display)-1, pageIdx, pageCount);
        return zxerr_ok;
    }

    //this indicates error in pubkey derivation (possible only when in menu_handler - in apdu_handler we throw)
    SCREEN(!hasPubkey && (show_address == SHOW_ADDRESS_YES)) {
        snprintf(outKey, outKeyLen, "Error");
        pageString(outVal, outValLen, " deriving public  key.", pageIdx, pageCount);
        return zxerr_ok;
    }

    SCREEN(true) {
        switch(show_address) {
            case SHOW_ADDRESS_ERROR:
                snprintf(outKey, outKeyLen, "Error reading");
                pageString(outVal, outValLen, "account data.", pageIdx, pageCount);
                return zxerr_ok;
            case SHOW_ADDRESS_EMPTY_SLOT:
                snprintf(outKey, outKeyLen, "Account data");
                pageString(outVal, outValLen, "not saved on the device.", pageIdx, pageCount);
                return zxerr_ok;
            case SHOW_ADDRESS_HDPATHS_NOT_EQUAL:
                snprintf(outKey, outKeyLen, "Address:");
                pageString(outVal, outValLen, "Other path is saved on the device.", pageIdx, pageCount);
                return zxerr_ok;
            case SHOW_ADDRESS_YES:
                snprintf(outKey, outKeyLen, "Address:");
                pageHexString(outVal, outValLen, address_to_display.data, sizeof(address_to_display.data), pageIdx, pageCount);
                return zxerr_ok;
            default:
                return zxerr_no_data;
        }
    }

    SCREEN(show_address == SHOW_ADDRESS_YES) {
        snprintf(outKey, outKeyLen, "Verify if this");
        snprintf(outVal, outValLen, " public key was   added to");
        return zxerr_ok;
    }

    SCREEN(show_address == SHOW_ADDRESS_YES) {
        array_to_hexstr(outKey, outKeyLen, address_to_display.data, sizeof(address_to_display.data)); 
        #if defined(TARGET_NANOX)
            snprintf(outVal, outValLen, " using any Flow blockchain explorer.");
        #else
            snprintf(outVal, outValLen, " using any Flow  blockch. explorer.");
        #endif
        return zxerr_ok;
    }

    SCREEN(app_mode_expert() && hasPubkey) {
        snprintf(outKey, outKeyLen, "Your Path");
        char buffer[300];
        bip32_to_str(buffer, sizeof(buffer), hdPath.data, HDPATH_LEN_DEFAULT);
        pageString(outVal, outValLen, buffer, pageIdx, pageCount);
        return zxerr_ok;
    }

    return zxerr_no_data;

    #undef SCREEN
}


#define ARBITRARY_NEGATIVE_INTEGER -1
zxerr_t addr_getNumItems(uint8_t *num_items) {
    int8_t displays = ARBITRARY_NEGATIVE_INTEGER;
    zxerr_t err = addr_getItem_internal(&displays, NULL, 0, NULL, 0, 0, NULL);

    int8_t pages = ARBITRARY_NEGATIVE_INTEGER - displays;

    if (pages < 0) {
        num_items = 0;
        return zxerr_unknown;
    }

    *num_items = (uint8_t) pages;

    if (err == zxerr_no_data) {
        return zxerr_ok;
    }
    if (err == zxerr_ok) {
        return zxerr_unknown;
    }
    return err;
}

zxerr_t addr_getItem(int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outVal, uint16_t outValLen,
                     uint8_t pageIdx, uint8_t *pageCount) {
    return addr_getItem_internal(&displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}
