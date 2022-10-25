/*******************************************************************************
*  (c) 2019 Zondax GmbH
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

#define CLA                             0x33

#include <stdint.h>
#include <stddef.h>

#define HDPATH_LEN_DEFAULT   5
typedef struct {
    uint32_t data[HDPATH_LEN_DEFAULT];
} hd_path_t;

#define ACCOUNT_SIZE       8
typedef struct {
    uint8_t data[ACCOUNT_SIZE];
} flow_account_t;

#define HDPATH_0_DEFAULT     (0x80000000u | 0x2cu)
#define HDPATH_1_DEFAULT     (0x80000000u | 0x21bu)
#define HDPATH_2_DEFAULT     (0x80000000u | 0u)
#define HDPATH_3_DEFAULT     (0u)
#define HDPATH_4_DEFAULT     (0u)

#define HDPATH_0_TESTNET     (0x80000000u | 0x2cu)
#define HDPATH_1_TESTNET     (0x80000000u | 0x1u)

#define SECP256_PK_LEN            65u

#define COIN_AMOUNT_DECIMAL_PLACES          0           // FIXME: Adjust this
#define COIN_SUPPORTED_TX_VERSION           0

#define MENU_MAIN_APP_LINE1 "Flow"
#define MENU_MAIN_APP_LINE2 "Ready"
#define APPVERSION_LINE1 "Version"
#define APPVERSION_LINE2 "v" APPVERSION

#define MAIN_SLOT 0

#define DOMAIN_TAG_LENGTH 32

#ifdef __cplusplus
}
#endif
