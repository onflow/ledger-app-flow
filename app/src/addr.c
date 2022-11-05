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

#include "screens.h"

bool hasPubkey;
uint8_t pubkey_to_display[SECP256_PK_LEN];

//this defines addr_internal function
BEGIN_SCREENS_FUNCTION(addr, zxerr_t, zxerr_ok, zxerr_no_data, zxerr_no_data)

    uint8_t show_address_yes = (show_address == SHOW_ADDRESS_YES || show_address == SHOW_ADDRESS_YES_HASH_MISMATCH);

    if (hasPubkey) {
        BEGIN_SCREEN
            snprintf(outKey, outKeyLen, "Pub Key");
            // +1 is to skip 0x04 prefix that indicates uncompresed key 
            pageHexString(outVal, outValLen, pubkey_to_display+1, sizeof(pubkey_to_display)-1, pageIdx, pageCount);
        END_SCREEN
    }
    
    if (!hasPubkey && show_address_yes) {
        BEGIN_SCREEN
            snprintf(outKey, outKeyLen, "Error");
            pageString(outVal, outValLen, " deriving public  key.", pageIdx, pageCount);
        END_SCREEN
    }

    //this indicates error in pubkey derivation (possible only when in menu_handler - in apdu_handler we throw)
    switch(show_address) {
        case SHOW_ADDRESS_ERROR:
            BEGIN_SCREEN
                snprintf(outKey, outKeyLen, "Error reading");
                pageString(outVal, outValLen, "account data.", pageIdx, pageCount);
            END_SCREEN
            break;
        case SHOW_ADDRESS_EMPTY_SLOT:
            BEGIN_SCREEN
                #if defined(TARGET_NANOS)
                    snprintf(outKey, outKeyLen, "Account data");
                    pageString(outVal, outValLen, "not saved on the device.", pageIdx, pageCount);
                #else
                    snprintf(outKey, outKeyLen, "Address:");
                    pageString(outVal, outValLen, "Account data not saved on the device.", pageIdx, pageCount);
                #endif
            END_SCREEN
            break;
        case SHOW_ADDRESS_HDPATHS_NOT_EQUAL:
            BEGIN_SCREEN
                snprintf(outKey, outKeyLen, "Address:");
                pageString(outVal, outValLen, "Other path is saved on the device.", pageIdx, pageCount);
            END_SCREEN
            break;
        case SHOW_ADDRESS_YES:
        case SHOW_ADDRESS_YES_HASH_MISMATCH:
            BEGIN_SCREEN
                snprintf(outKey, outKeyLen, "Address:");
                pageHexString(outVal, outValLen, address_to_display.data, sizeof(address_to_display.data), pageIdx, pageCount);
                return zxerr_ok;
            END_SCREEN
            break;
        default:
            return zxerr_unknown;
    }

    if (show_address_yes) {
        BEGIN_SCREEN
            #if defined(TARGET_NANOS)
                snprintf(outKey, outKeyLen, "Verify if this");
                snprintf(outVal, outValLen, " public key was   added to");
            #else
                snprintf(outKey, outKeyLen, "Warning:");
                snprintf(outVal, outValLen, "Verify if this public key was added to");
            #endif
        END_SCREEN
    }

    if (show_address_yes) {
        BEGIN_SCREEN
            #if defined(TARGET_NANOS)
                array_to_hexstr(outKey, outKeyLen, address_to_display.data, sizeof(address_to_display.data)); 
                snprintf(outVal, outValLen, " using any Flow  blockch. explorer.");
            #else
                char buffer[2*sizeof(address_to_display.data)+1];
                array_to_hexstr(buffer, sizeof(buffer), address_to_display.data, sizeof(address_to_display.data)); 
                snprintf(outVal, outValLen, "%s using any Flow blockchain explorer.", buffer);
                snprintf(outKey, outKeyLen, ""); 
            #endif
        END_SCREEN
    }

    if (app_mode_expert() && hasPubkey) {
        BEGIN_SCREEN
            snprintf(outKey, outKeyLen, "Your Path");
            char buffer[100];
            path_options_to_string(buffer, sizeof(buffer), hdPath.data, HDPATH_LEN_DEFAULT, cryptoOptions & 0xFF00); //show curve only
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);
        END_SCREEN
    }

END_SCREENS_FUNCTION


zxerr_t addr_getNumItems(uint8_t *num_items) {
    return addr_internal(MAX_DISPLAYS, NULL, 0, NULL, 0, 0, num_items);
}

zxerr_t addr_getItem(uint8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outVal, uint16_t outValLen,
                     uint8_t pageIdx, uint8_t *pageCount) {
    return addr_internal(displayIdx, outKey, outKeyLen, outVal, outValLen, pageIdx, pageCount);
}
