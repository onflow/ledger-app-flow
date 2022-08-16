/*******************************************************************************
*   (c) 2022 Vacuumlabs
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
#include "message.h"
#include "zxformat.h"
#include "app_mode.h"
#include "hdpath.h"
#include "crypto.h"
#include "buffering.h"

#define MAX_MESSAGE_SHOW_LENGTH 34*5
#define MAX_MESSAGE_LENGTH 0x7FFF

struct {
    uint8_t hash[CX_SHA256_SIZE];
    uint16_t length;
    uint8_t *message;
    bool canBeDisplayed;

} messageData;

zxerr_t message_parse() {
    messageData.message = buffering_get_buffer()->data;
    messageData.length = buffering_get_buffer()->pos;

    if (messageData.length > MAX_MESSAGE_LENGTH) {
        return zxerr_out_of_bounds;
    }

    sha256(messageData.message, messageData.length, messageData.hash);
    messageData.canBeDisplayed = false;

    if (messageData.length <= MAX_MESSAGE_SHOW_LENGTH) {
        messageData.canBeDisplayed = true;
        for(size_t j=0; j<messageData.length; j++) {
            if (messageData.message[j]<32 || messageData.message[j]>127) {
                messageData.canBeDisplayed = false;
                break;
            }
        }   
    }

    return zxerr_ok;
}

zxerr_t message_getNumItems(uint8_t *num_items){
    *num_items = app_mode_expert() ? 2: 1;
    return zxerr_ok;
}

// retrieves a readable output for each field / page
zxerr_t message_getItem(int8_t displayIdx,
                        char *outKey, uint16_t outKeyLen,
                        char *outVal, uint16_t outValLen,
                        uint8_t pageIdx, uint8_t *pageCount) {
    switch(displayIdx) {
    case 0:
        if (messageData.canBeDisplayed) {
            snprintf(outKey, outKeyLen, "Message");
            pageStringExt(outVal, outValLen, (char *)messageData.message, messageData.length, pageIdx, pageCount);
        }
        else {
            snprintf(outKey, outKeyLen, "Message hash");
            pageHexString(outVal, outValLen, messageData.hash, sizeof(messageData.hash), pageIdx, pageCount);
        }
        return zxerr_ok;                                
    case 1:
        if (app_mode_expert()) {
            snprintf(outKey, outKeyLen, "Your Path");
            char buffer[100];
            path_options_to_string(buffer, sizeof(buffer), hdPath.data, HDPATH_LEN_DEFAULT, cryptoOptions); 
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);
            return zxerr_ok;
        }
        return zxerr_no_data;
    }
    return zxerr_no_data;
}
