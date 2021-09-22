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
#include <os.h>
#include "coin.h"
#include "zxerror.h"
#include "zxmacros.h"
#include "app_mode.h"
#include "crypto.h"
#include "account.h"

zxerr_t addr_getNumItems(uint8_t *num_items) {
    zemu_log_stack("addr_getNumItems");
    if (app_mode_expert()) {
        *num_items = 4;
        return zxerr_ok;
    }
    *num_items = 3;
    return zxerr_ok;
}

//To add to zxlib after fork
__Z_INLINE void pageHexStringFromBuf(char *outValue, uint16_t outValueLen,
                           uint8_t *inValue, uint16_t inValueLen,
                           uint8_t pageIdx, uint8_t *pageCount) {
    char buf[2*inValueLen+1];
    uint_fast32_t len = array_to_hexstr(buf, sizeof(buf), inValue, inValueLen);
    if (len == 0) {
        return;
    }
    pageString(outValue, outValueLen, buf, pageIdx, pageCount);
}


zxerr_t addr_getItem(int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outVal, uint16_t outValLen,
                     uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("addr_getItem");
    switch (displayIdx) {
        case 0: {
            snprintf(outKey, outKeyLen, "Account");
            array_to_hexstr_with_0x(outVal, outValLen, G_io_apdu_buffer, sizeof(flow_account_t));
            return zxerr_ok;
        }
        case 1: {
            snprintf(outKey, outKeyLen, "Pub Key");
            pageHexStringFromBuf(outVal, outValLen, G_io_apdu_buffer + sizeof(flow_account_t), PUBLIC_KEY_LEN, pageIdx, pageCount);
            return zxerr_ok;
        }
        case 2: {
            snprintf(outKey, outKeyLen, "Warning");
            pageString(outVal, outValLen, "Ledger does not verify if account and pub key match!!!", pageIdx, pageCount);
            return zxerr_ok;        
        }
        case 3: {
            if (!app_mode_expert()) {
                return zxerr_no_data;
            }

            snprintf(outKey, outKeyLen, "Your Path");
            char buffer[300];
            bip32_to_str(buffer, sizeof(buffer), hdPath.data, HDPATH_LEN_DEFAULT);
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);
            return zxerr_ok;
        }
        default:
            return zxerr_no_data;
    }
}
