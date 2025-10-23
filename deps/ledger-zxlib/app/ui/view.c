/*******************************************************************************
 *   (c) 2018 - 2022 Zondax AG
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

#include "actions.h"
#include "view_internal.h"
#include "zxmacros.h"

#define DEFAULT_SPINNER_TEXT "Processing..."

view_t viewdata;
unsigned int review_type = 0;

///////////////////////////////////
// Paging related

void h_paging_init() {
    zemu_log_stack("h_paging_init");

    viewdata.itemIdx = 0;
    viewdata.pageIdx = 0;
    viewdata.pageCount = 1;
    viewdata.itemCount = 0xFF;
}

///////////////////////////////////
// General
void view_init(void) {
    UX_INIT();
#ifdef APP_SECRET_MODE_ENABLED
    viewdata.secret_click_count = 0;
#endif
}

void view_idle_show(uint8_t item_idx, const char *statusString) { view_idle_show_impl(item_idx, statusString); }

void view_message_show(const char *title, const char *message) { view_message_impl(title, message); }

void view_spinner_show(const char *text) { view_spinner_impl(text ? text : DEFAULT_SPINNER_TEXT); }

void view_review_init(viewfunc_getItem_t viewfuncGetItem, viewfunc_getNumItems_t viewfuncGetNumItems,
                      viewfunc_accept_t viewfuncAccept) {
    viewdata.viewfuncGetItem = viewfuncGetItem;
    viewdata.viewfuncGetNumItems = viewfuncGetNumItems;
    viewdata.viewfuncAccept = viewfuncAccept;

#if defined(TARGET_NANOS) || defined(TARGET_NANOS2) || defined(TARGET_NANOX)
    viewdata.with_confirmation = false;
#endif
}

void view_review_init_progressive(viewfunc_getItem_t viewfuncGetItem, viewfunc_getNumItems_t viewfuncGetNumItems,
                                  viewfunc_accept_t viewfuncAccept) {
    view_review_init(viewfuncGetItem, viewfuncGetNumItems, viewfuncAccept);

#if defined(TARGET_NANOS) || defined(TARGET_NANOS2) || defined(TARGET_NANOX)
    viewdata.with_confirmation = true;
#endif
}

void view_initialize_init(viewfunc_initialize_t viewFuncInit) { viewdata.viewfuncInitialize = viewFuncInit; }

void view_review_show(review_type_e reviewKind) {
    // Set > 0 to reply apdu message
    view_review_show_impl((unsigned int)reviewKind, NULL, NULL);
}

void view_review_show_with_intent(review_type_e reviewKind, const char *intent) {
    // New function that explicitly handles intent
    view_review_show_with_intent_impl((unsigned int)reviewKind, intent);
}

void view_review_show_generic(review_type_e reviewKind, const char *title, const char *validate) {
    view_review_show_impl((unsigned int)reviewKind, title, validate);
}

void view_initialize_show(uint8_t item_idx, const char *statusString) {
    view_initialize_show_impl(item_idx, statusString);
}

void h_approve(__Z_UNUSED unsigned int _) {
    zemu_log_stack("h_approve");

    view_idle_show(0, NULL);
    UX_WAIT();
    if (viewdata.viewfuncAccept != NULL) {
        viewdata.viewfuncAccept();
    }
}

void h_reject(unsigned int requireReply) {
    zemu_log_stack("h_reject");

    view_idle_show(0, NULL);
    UX_WAIT();

    if (requireReply != REVIEW_UI) {
        app_reject();
    }
}

void h_error_accept(__Z_UNUSED unsigned int _) {
    view_idle_show(0, NULL);
    UX_WAIT();
    app_reply_error();
}
