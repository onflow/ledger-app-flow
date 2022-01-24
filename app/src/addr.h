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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//Enum to determine if we should show address
typedef enum {
    show_address_yes,
    show_address_empty_slot,
    show_address_hdpaths_not_equal,
} show_addres_t;

extern show_addres_t show_address;
extern uint8_t pubkey_to_display[SECP256_PK_LEN];
extern uint8_t address_to_display[ADDRESS_LENGTH];

/// Return the number of items in the address view
zxerr_t addr_getNumItems(uint8_t *num_items);

/// Gets an specific item from the address view (including paging)
zxerr_t addr_getItem(int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outValue, uint16_t outValueLen,
                     uint8_t pageIdx, uint8_t *pageCount);

#ifdef __cplusplus
}
#endif
