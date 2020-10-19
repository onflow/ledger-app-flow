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
#include <json/json_parser.h>
#include "parser_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "crypto.h"
#include "template_hashes.h"

#define PARSER_MAX_ARGCOUNT 10

typedef enum {
    script_unknown,
    script_token_transfer,
    script_create_account,
    script_add_new_key,
    script_th01_withdraw_unlocked_tokens,
    script_th02_deposit_unlocked_tokens,
    script_th06_register_node,
    script_th08_stake_new_tokens,
    script_th09_restake_unstaked_tokens,
    script_th10_restake_rewarded_tokens,
    script_th11_unstake_tokens,
    script_th12_unstake_all_tokens,
    script_th13_withdraw_unstaked_tokens,
    script_th14_withdraw_rewarded_tokens,
    script_th17_register_delegator,
    script_th19_delegate_new_tokens,
    script_th20_restake_unstaked_delegated_tokens,
    script_th21_restake_rewarded_delegated_tokens,
    script_th22_unstake_delegated_tokens,
    script_th23_withdraw_unstaked_delegated_tokens,
    script_th24_withdraw_rewarded_delegated_tokens,
} script_type_e;

typedef enum {
    chain_id_unknown,
    chain_id_emulator,
    chain_id_testnet,
    chain_id_mainnet,
} chain_id_e;

typedef struct {
    parser_context_t ctx;
    uint8_t digest[CX_SHA256_SIZE];
    script_type_e type;
} flow_script_t;

typedef struct {
    parser_context_t ctx;
} flow_reference_block_id_t;

typedef struct {
    parser_context_t ctx;
    parser_context_t argCtx[PARSER_MAX_ARGCOUNT];
    uint16_t argCount;
} flow_argument_list_t;

typedef uint64_t flow_gaslimit_t;

typedef struct {
    parser_context_t ctx;
} flow_proposal_key_address_t;

typedef uint64_t flow_proposal_keyid_t;

typedef uint64_t flow_proposal_key_sequence_number_t;

typedef struct {
    parser_context_t ctx;
} flow_payer_t;

typedef struct {
    parser_context_t ctx;
} flow_proposal_authorizer_t;

typedef struct {
    parser_context_t ctx;
    uint16_t authorizer_count;
    flow_proposal_authorizer_t authorizer[16];
} flow_proposal_authorizers_t;

typedef struct {
    flow_script_t script;
    flow_argument_list_t arguments;
    flow_reference_block_id_t referenceBlockId;
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
