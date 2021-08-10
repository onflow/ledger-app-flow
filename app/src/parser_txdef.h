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
    SCRIPT_UNKNOWN,
    SCRIPT_TOKEN_TRANSFER,
    SCRIPT_CREATE_ACCOUNT,
    SCRIPT_ADD_NEW_KEY,
    SCRIPT_TH01_WITHDRAW_UNLOCKED_TOKENS,
    SCRIPT_TH02_DEPOSIT_UNLOCKED_TOKENS,
    SCRIPT_TH06_REGISTER_NODE,
    SCRIPT_TH08_STAKE_NEW_TOKENS,
    SCRIPT_TH09_RESTAKE_UNSTAKED_TOKENS,
    SCRIPT_TH10_RESTAKE_REWARDED_TOKENS,
    SCRIPT_TH11_UNSTAKE_TOKENS,
    SCRIPT_TH12_UNSTAKE_ALL_TOKENS,
    SCRIPT_TH13_WITHDRAW_UNSTAKED_TOKENS,
    SCRIPT_TH14_WITHDRAW_REWARDED_TOKENS,
    SCRIPT_TH16_REGISTER_OPERATOR_NODE,
    SCRIPT_TH17_REGISTER_DELEGATOR,
    SCRIPT_TH19_DELEGATE_NEW_TOKENS,
    SCRIPT_TH20_RESTAKE_UNSTAKED_DELEGATED_TOKENS,
    SCRIPT_TH21_RESTAKE_REWARDED_DELEGATED_TOKENS,
    SCRIPT_TH22_UNSTAKE_DELEGATED_TOKENS,
    SCRIPT_TH23_WITHDRAW_UNSTAKED_DELEGATED_TOKENS,
    SCRIPT_TH24_WITHDRAW_REWARDED_DELEGATED_TOKENS,
    SCRIPT_SCO01_SETUP_STAKING_COLLECTION,
    SCRIPT_SCO02_REGISTER_DELEGATOR,
    SCRIPT_SCO03_REGISTER_NODE,
    SCRIPT_SCO04_CREATE_MACHINE_ACCOUNT,
    SCRIPT_SCO05_REQUEST_UNSTAKING,
    SCRIPT_SCO06_STAKE_NEW_TOKENS,
    SCRIPT_SCO07_STAKE_REWARD_TOKENS,
    SCRIPT_SCO08_STAKE_UNSTAKED_TOKENS,
    SCRIPT_SCO09_UNSTAKE_ALL,
    SCRIPT_SCO10_WITHDRAW_REWARD_TOKENS,
    SCRIPT_SCO11_WITHDRAW_UNSTAKED_TOKENS,
    SCRIPT_SCO12_CLOSE_STAKE,
    SCRIPT_SCO13_TRANSFER_NODE,
    SCRIPT_SCO14_TRANSFER_DELEGATOR,
    SCRIPT_SCO15_WITHDRAW_FROM_MACHINE_ACCOUNT,
    SCRIPT_SCO16_UPDATE_NETWORKING_ADDRESS,
} script_type_e;

typedef enum {
    CHAIN_ID_UNKNOWN,
    CHAIN_ID_EMULATOR,
    CHAIN_ID_TESTNET,
    CHAIN_ID_MAINNET,
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
