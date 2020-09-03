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

#include "zxerror.h"
#include "coin.h"

#define SLOT_ACCOUNT_SIZE       8
#define SLOT_COUNT              64

typedef struct {
    uint8_t data[SLOT_ACCOUNT_SIZE];
} flow_account_t;

typedef struct {
    uint32_t data[HDPATH_LEN_DEFAULT];
} flow_path_t;

typedef struct {
    flow_account_t account;
    flow_path_t path;
} account_slot_t;

typedef struct {
    account_slot_t slot[SLOT_COUNT];
} slot_store_t;

zxerr_t slot_status(uint8_t *out, uint16_t outLen);

zxerr_t slot_getSlot(uint8_t slotIndex, uint8_t *out, uint16_t outLen);

zxerr_t slot_setSlot(uint8_t slotIndex, uint8_t *data, uint16_t dataLen);

#ifdef __cplusplus
}
#endif
