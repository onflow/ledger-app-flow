/*******************************************************************************
 *   (c) 2018 - 2024 Zondax AG
 *   (c) 2016 Ledger
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

#include <stdbool.h>
#include <stdint.h>

#include "coin.h"
#include "view.h"
#include "zxerror.h"

#define CUR_FLOW G_ux.flow_stack[G_ux.stack_count - 1]

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define MAX_CHARS_PER_KEY_LINE 64
#ifdef ZXLIB_LIGHT_MODE
#define MAX_CHARS_PER_VALUE1_LINE 256
#else
#define MAX_CHARS_PER_VALUE1_LINE 4096
#endif
#define MAX_CHARS_SUBMSG_LINE 2048
#define MAX_CHARS_HEXMESSAGE 160
#elif defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX_P)
#include "nbgl_use_case.h"
#define MAX_LINES_PER_PAGE_REVIEW NB_MAX_LINES_IN_REVIEW
#define MAX_CHARS_PER_KEY_LINE 64
#define MAX_CHARS_PER_VALUE1_LINE 180
#define MAX_CHARS_SUBMSG_LINE 180
#define MAX_CHARS_HEXMESSAGE 160
#else
#ifndef MAX_CHARS_PER_VALUE_LINE
#define MAX_CHARS_PER_VALUE_LINE (17)
#endif
#define MAX_CHARS_PER_KEY_LINE (MAX_CHARS_PER_VALUE_LINE + 1)
#define MAX_CHARS_PER_VALUE1_LINE (2 * MAX_CHARS_PER_VALUE_LINE + 1)
#define MAX_CHARS_PER_SUBMSG_LINE (2 * MAX_CHARS_PER_VALUE_LINE + 1)
#define MAX_CHARS_PER_VALUE2_LINE (MAX_CHARS_PER_VALUE_LINE + 1)
#define MAX_CHARS_HEXMESSAGE 40
#endif

// This takes data from G_io_apdu_buffer that is prefilled with the address

#if defined(APP_BLINDSIGN_MODE_ENABLED)
#define APPROVE_LABEL_1 "ACCEPT RISK"
#define APPROVE_LABEL_2 "AND APPROVE"
#endif
#define APPROVE_LABEL "APPROVE"
#define REJECT_LABEL "REJECT"

#define SHORTCUT_TITLE "Skip"
#define SHORTCUT_VALUE "fields"
#define SHORTCUT_STR "Skip fields"

// Review string can be customizable in each app
#if !defined(REVIEW_SCREEN_TITLE) && !defined(REVIEW_SCREEN_TX_VALUE) && !defined(REVIEW_SCREEN_ADDR_VALUE)
#define REVIEW_SCREEN_TITLE "Please"
#define REVIEW_SCREEN_TXN_VALUE "review"
#define REVIEW_SCREEN_ADDR_VALUE "review"
#endif

#ifdef APP_BLINDSIGN_MODE_ENABLED
#define REVIEW_BLINDSIGN_MESSAGE_TITLE "Warning"
#define REVIEW_BLINDSIGN_MESSAGE_VALUE \
    "Tx details not verifiable. "      \
    "Could lose all assets"
#endif

static const char *review_key = REVIEW_SCREEN_TITLE;
static const char *review_txvalue = REVIEW_SCREEN_TXN_VALUE;
static const char *review_addrvalue = REVIEW_SCREEN_ADDR_VALUE;
static const char *review_keyconfig = "Review";
static const char *review_configvalue = "configuration";
static const char *review_skip_key = "Warning";
static const char *review_skip_value = "BlindSign";
static const char *review_skip_key_msg = "Tx details";
static const char *review_skip_value_msg = "not verifiable";
static const char *review_skip_key_msg_2 = "Could lose";
static const char *review_skip_value_msg_2 = "all assets";
static const char *review_msgvalue = "Review";
static const char *review_msgvalue_2 = "Message";

// Review msg string can be customizable in each app
#if !defined(REVIEW_MSG_TITLE) && !defined(REVIEW_MSG_VALUE)
#define REVIEW_MSG_TITLE "Review"
#define REVIEW_MSG_VALUE "Message"
#endif

static const char *review_msgKey = REVIEW_MSG_TITLE;
static const char *review_msgValue = REVIEW_MSG_VALUE;

static const char *shortcut_key = SHORTCUT_TITLE;
static const char *shortcut_value = SHORTCUT_VALUE;

#if defined(TARGET_NANOS)
#if defined(APP_BLINDSIGN_MODE_ENABLED)
#define INTRO_PAGES 3
#elif defined(REVIEW_SCREEN_ENABLED) && defined(SHORTCUT_MODE_ENABLED)
#define INTRO_PAGES 2
#elif defined(REVIEW_SCREEN_ENABLED) || defined(SHORTCUT_MODE_ENABLED)
#define INTRO_PAGES 1
#else
#define INTRO_PAGES 0
#endif
#else
#define INTRO_PAGES 0
#endif

// FIXME: Wait to be fixed on SDK:
// https://github.com/LedgerHQ/ledger-secure-sdk/blob/fe169b19c7445f2477c26035a827c22ba9f84964/lib_nbgl/include/nbgl_use_case.h#L59
#if defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX_P)
#ifdef NB_MAX_DISPLAYED_PAIRS_IN_REVIEW
#undef NB_MAX_DISPLAYED_PAIRS_IN_REVIEW
#define NB_MAX_DISPLAYED_PAIRS_IN_REVIEW 6
#endif
#endif

typedef struct {
    struct {
#if defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX_P)
        char *key;
        char *value;
        char keys[NB_MAX_DISPLAYED_PAIRS_IN_REVIEW][MAX_CHARS_PER_KEY_LINE];
        char values[NB_MAX_DISPLAYED_PAIRS_IN_REVIEW][MAX_CHARS_PER_VALUE1_LINE];
#else
        char key[MAX_CHARS_PER_KEY_LINE];
        char value[MAX_CHARS_PER_VALUE1_LINE];
#if defined(TARGET_NANOS)
        char value2[MAX_CHARS_PER_VALUE2_LINE];
#endif
#endif
    };
    viewfunc_getItem_t viewfuncGetItem;
    viewfunc_getNumItems_t viewfuncGetNumItems;
    viewfunc_accept_t viewfuncAccept;
    viewfunc_initialize_t viewfuncInitialize;

    viewfunc_getInnerItem_t viewfuncGetInnerItem;
    viewfunc_getNumItems_t viewfuncGetInnerNumItems;
    viewfunc_canInspectItem_t viewfuncCanInspectItem;

#ifdef APP_SECRET_MODE_ENABLED
    uint8_t secret_click_count;
#endif
    uint8_t itemIdx;
    uint8_t itemCount;
    uint8_t pageIdx;
    uint8_t pageCount;

    inner_state_t innerField;
#if defined(TARGET_NANOS) || defined(TARGET_NANOS2) || defined(TARGET_NANOX)
    /**
     * @brief Determines whether to prompt the user for confirmation during the review process.
     *
     * When `with_confirmation` is set to `true`, the engine will display a confirmation prompt
     * to the user after each item review. This allows the user to either continue reviewing items
     * or proceed to the final approval step.
     *
     * If `with_confirmation` is set to `false`, the engine will skip the confirmation prompts
     * forcing users to review all items until they get to the approval menu.
     *
     * **Applicable Targets:**
     * - Ledger Nano S (TARGET_NANOS)
     * - Ledger Nano S2 (TARGET_NANOS2)
     * - Ledger Nano X (TARGET_NANOX)
     *
     * **Excluded Targets:**
     * - Stax
     * - Flex
     */
    bool with_confirmation;
#endif
} view_t;

typedef enum {
    view_action_unknown,
    view_action_accept,
    view_action_reject,
} view_action_t;

extern view_t viewdata;

#define print_title(...) snprintf(viewdata.title, sizeof(viewdata.title), __VA_ARGS__)
#define print_key(...) snprintf(viewdata.key, sizeof(viewdata.key), __VA_ARGS__);
#define print_value(...) snprintf(viewdata.value, sizeof(viewdata.value), __VA_ARGS__);

#if defined(TARGET_NANOS)
#define print_value2(...) snprintf(viewdata.value2, sizeof(viewdata.value2), __VA_ARGS__);
#endif

///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////
///////////////////////////////////////////////

void view_idle_show_impl(uint8_t item_idx, const char *statusString);

void view_message_impl(const char *title, const char *message);

void view_error_show_impl();

void view_settings_show_impl();

void view_custom_error_show_impl();

void view_spinner_impl(const char *text);

void view_blindsign_error_show_impl();

void h_paging_init();

void h_inspect_init();

void view_review_show_impl(unsigned int requireReply, const char *title, const char *validate);

void view_review_show_with_intent_impl(unsigned int requireReply, const char *intent);

void view_inspect_show_impl();

void view_initialize_show_impl(uint8_t item_idx, const char *statusString);

void h_approve(unsigned int _);

void h_reject(unsigned int requireReply);

void h_review_update();

void h_error_accept(unsigned int _);

zxerr_t h_review_update_data();

zxerr_t h_inspect_update_data();
