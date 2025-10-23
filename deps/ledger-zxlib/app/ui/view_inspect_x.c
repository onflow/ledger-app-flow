/*******************************************************************************
 *   (c) 2018 - 2023 Zondax AG
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
#include "bolos_target.h"

#if defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#include "view_internal.h"
#include "view_nano.h"
#include "view_nano_inspect.h"
#include "view_templates.h"
#include "zxmacros.h"

extern const ux_flow_step_t *ux_review_flow[MAX_REVIEW_UX_SCREENS];

static void h_inspect_loop_start();
static void h_inspect_loop_inside();
static void h_inspect_loop_end();

static void h_inspect();
static void h_back();
static void h_rootTxn();

void run_root_txn_flow();

uint8_t flow_inside_inspect_loop;
extern unsigned int review_type;

// Menu to recursively inspect elements from a transaction
UX_STEP_VALID(ux_inspect_flow_root_step, pb, h_rootTxn(), {&C_icon_back, "Go to root"});

UX_STEP_INIT(ux_inspect_flow_start_step, NULL, NULL, { h_inspect_loop_start(); });
UX_STEP_CB_INIT(ux_inspect_flow_step, bnnn_paging, h_inspect_loop_inside(), h_inspect(),
                {
                    .title = viewdata.key,
                    .text = viewdata.value,
                });

UX_STEP_INIT(ux_inspect_flow_end_step, NULL, NULL, { h_inspect_loop_end(); });

UX_STEP_VALID(ux_inspect_flow_back_step, pb, h_back(), {&C_icon_back, "BACK"});

void view_inspect_show_impl() {
    // h_inspect_loop_inside();
    if (G_ux.stack_count == 0) {
        ux_stack_push();
    }

    uint8_t index = 0;
    ux_review_flow[index++] = &ux_inspect_flow_root_step;
    ux_review_flow[index++] = &ux_inspect_flow_start_step;
    ux_review_flow[index++] = &ux_inspect_flow_step;
    ux_review_flow[index++] = &ux_inspect_flow_end_step;
    ux_review_flow[index++] = &ux_inspect_flow_back_step;
    ux_review_flow[index++] = FLOW_END_STEP;

    ux_flow_init(0, ux_review_flow, &ux_inspect_flow_start_step);
}

void inspect_init() {
#ifdef HAVE_INSPECT
    h_inspect_init();
    if (!viewdata.viewfuncCanInspectItem(viewdata.innerField.level, viewdata.innerField.trace,
                                         viewdata.innerField.paging.itemIdx)) {
        h_rootTxn();
        return;
    }

    const zxerr_t err = h_inspect_update_data();
    if (err != zxerr_ok) {
        // Show error
        view_error_show();
        // h_rootTxn();
        return;
    }
    view_inspect_show_impl();
#endif
}

void h_inspect() {
    if (viewdata.viewfuncCanInspectItem == NULL) {
        view_error_show();
        return;
    }

    // Check if we can inspect this element
    if (viewdata.innerField.level >= MAX_DEPTH ||
        !viewdata.viewfuncCanInspectItem(viewdata.innerField.level + 1, viewdata.innerField.trace,
                                         viewdata.innerField.paging.itemIdx)) {
        flow_inside_inspect_loop = 0;
        h_inspect_update_data();
        view_inspect_show_impl();
        return;
    }

    // Proceed with inspection
    viewdata.innerField.level++;
    viewdata.innerField.trace[viewdata.innerField.level] = viewdata.innerField.paging.itemIdx;
    viewdata.innerField.paging.itemIdx = 0;
    viewdata.innerField.paging.pageCount = 1;
    viewdata.innerField.paging.itemCount = 0xFF;
    flow_inside_inspect_loop = 0;

    h_inspect_update_data();
    view_inspect_show_impl();
}

void h_rootTxn() {
    viewdata.innerField.level = 0;
    viewdata.itemIdx = viewdata.innerField.trace[viewdata.innerField.level];
    h_paging_increase();
    run_root_txn_flow();
}

void h_back() {
    if (viewdata.innerField.level > 1) {
        viewdata.innerField.paging.itemIdx = viewdata.innerField.trace[viewdata.innerField.level];
        viewdata.innerField.level--;
        viewdata.innerField.paging.pageCount = 1;
        viewdata.innerField.paging.itemCount = 0xFF;
        flow_inside_inspect_loop = 0;

        h_inspect_update_data();
        view_inspect_show_impl();
        return;
    }

    // Go back to root txn review
    h_rootTxn();
}

void h_inspect_loop_inside() {
    ZEMU_LOGF(50, "h_inspect_loop_inside\n")
    flow_inside_inspect_loop = 1;
}

void h_inspect_loop_start() {
    ZEMU_LOGF(50, "h_inspect_loop_start\n")
    if (flow_inside_inspect_loop) {
        // coming from right

        if (!h_can_decrease(&viewdata.innerField.paging)) {
            // exit to the left
            flow_inside_inspect_loop = 0;
            ux_flow_prev();
            return;
        }

        h_decrease(&viewdata.innerField.paging);
    } else {
        // coming from left
        flow_inside_inspect_loop = 1;
    }

    h_inspect_update_data();
    ux_flow_next();
}

void h_inspect_loop_end() {
    ZEMU_LOGF(50, "h_inspect_loop_end\n")
    if (flow_inside_inspect_loop) {
        // coming from left
        if (!h_can_increase(&viewdata.innerField.paging, 0)) {
            // exit to the right
            flow_inside_inspect_loop = 0;
            ux_flow_next();
            return;
        }
        h_increase(&viewdata.innerField.paging, 0);

    } else {
        // coming from right
        // make sure that we are getting the last itemIdx/pageIdx
        h_inspect_loop_inside();
    }

    const zxerr_t err = h_inspect_update_data();
    if (err != zxerr_ok && err != zxerr_no_data) {
        view_error_show();
        return;
    }

    ux_layout_bnnn_paging_reset();
    CUR_FLOW.prev_index = CUR_FLOW.index - 2;
    CUR_FLOW.index--;
    ux_flow_relayout();
}

#endif
