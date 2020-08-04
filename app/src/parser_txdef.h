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

#include <coin.h>
#include <zxtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *ptr;
    uint16_t len;
} flow_script_t;

typedef struct {
    uint8_t *ptr;
    uint16_t len;
} flow_argument_t;

typedef struct {
    uint8_t *ptr;
    uint16_t len;
} flow_argument_list_t;

typedef uint64_t flow_gaslimit_t;

typedef struct {
    uint8_t *ptr;
    uint16_t len;
} flow_proposal_key_address_t;

typedef uint64_t flow_proposal_keyid_t;

typedef struct {
    uint8_t *ptr;
    uint16_t len;
} flow_payer_t;

typedef uint64_t flow_proposal_key_sequence_number_t;

typedef struct {
    uint8_t *ptr;
    uint16_t len;
} flow_proposal_authorizer_t;

typedef struct {
    uint8_t *ptr;
    uint16_t len;
} flow_proposal_authorizers_t;

typedef struct {
    flow_script_t script;
    flow_argument_list_t arguments;
    flow_gaslimit_t gasLimit;
    flow_proposal_key_address_t proposalKeyAddress;
    flow_proposal_keyid_t proposalKeyId;
    flow_proposal_key_sequence_number_t  proposalKeySequenceNumber;
    flow_payer_t payer;
    flow_proposal_authorizers_t authorizers;
} parser_tx_t;

#ifdef __cplusplus
}
#endif
