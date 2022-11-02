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

#include <tx_metadata.h>
#include <stdint.h>
#include <stddef.h>
#include "crypto.h"

typedef enum {
    CHAIN_ID_UNKNOWN,
    CHAIN_ID_EMULATOR,
    CHAIN_ID_TESTNET,
    CHAIN_ID_MAINNET,
} chain_id_e;

typedef struct {
    uint8_t digest[CX_SHA256_SIZE];
} flow_script_hash_t;

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
    bool metadataInitialized;
    parsed_tx_metadata_t metadata;
    flow_script_hash_t hash;
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
