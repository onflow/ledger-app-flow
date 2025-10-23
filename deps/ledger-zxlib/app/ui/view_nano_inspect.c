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

#if defined(TARGET_NANOS) || defined(TARGET_NANOS2) || defined(TARGET_NANOX)
#include "view_internal.h"
#include "view_nano.h"
#include "view_nano_inspect.h"
#include "view_templates.h"
#include "zxmacros.h"

void h_inspect_init() {
    ZEMU_LOGF(50, "h_inspect_init\n")
    viewdata.innerField.level = 0;
    viewdata.innerField.trace[viewdata.innerField.level] = viewdata.itemIdx;

    viewdata.innerField.paging.itemIdx = 0;
    viewdata.innerField.paging.pageIdx = 0;
    viewdata.innerField.paging.pageCount = 1;
    viewdata.innerField.paging.itemCount = 0xFF;
}

void view_inspect_init(viewfunc_getInnerItem_t view_funcGetInnerItem, viewfunc_getNumItems_t view_funcGetInnerNumItems,
                       viewfunc_canInspectItem_t view_funcCanInspectItem) {
    viewdata.viewfuncGetInnerItem = view_funcGetInnerItem;
    viewdata.viewfuncGetInnerNumItems = view_funcGetInnerNumItems;
    viewdata.viewfuncCanInspectItem = view_funcCanInspectItem;
}

bool h_paging_inspect_go_to_root_screen() {
    return (viewdata.innerField.paging.itemIdx == 0) && (viewdata.innerField.trace[0] != 0);
}

bool h_paging_inspect_back_screen() {
    return (viewdata.innerField.paging.itemIdx == (viewdata.innerField.paging.itemCount - 1)) &&
           (viewdata.innerField.trace[0] != 0);
}

bool h_can_increase(paging_t *paging, uint8_t actionsCount) {
    if (paging->pageIdx + 1 < paging->pageCount) {
        ZEMU_LOGF(50, "h_inspect_paging_can_increase\n")
        return true;
    }

    // passed page count, go to next index
    if (paging->itemCount > 0 && paging->itemIdx < (paging->itemCount - 1 + actionsCount)) {
        ZEMU_LOGF(50, "h_inspect_paging_can_increase\n")
        return true;
    }

    ZEMU_LOGF(50, "h_inspect_paging_can_increase NO\n")
    return false;
}

void h_increase(paging_t *paging, uint8_t actionsCount) {
    if (paging == NULL) {
        return;
    }
    ZEMU_LOGF(50, "h_inspect_paging_increase\n")

    if (paging->pageIdx + 1 < paging->pageCount) {
        ZEMU_LOGF(50, "pageIdx++\n")
        // increase page
        paging->pageIdx++;
        return;
    }

    // passed page count, go to next index
    if (paging->itemCount > 0 && paging->itemIdx < (paging->itemCount - 1 + actionsCount)) {
        ZEMU_LOGF(50, "itemIdx++\n")
        paging->itemIdx++;
        paging->pageIdx = 0;
    }
}

bool h_can_decrease(paging_t *paging) {
    if (paging != NULL && (paging->pageIdx != 0 || paging->itemIdx > 0)) {
        ZEMU_LOGF(50, "h_inspect_paging_can_decrease\n")
        return true;
    }
    ZEMU_LOGF(50, "h_inspect_paging_can_decrease NO\n")
    return false;
}

void h_decrease(paging_t *paging) {
    if (paging == NULL) {
        return;
    }

    ZEMU_LOGF(50, "h_inspect_paging_decrease Idx %d\n", paging->itemIdx)

    if (paging->pageIdx != 0) {
        paging->pageIdx--;
        ZEMU_LOGF(50, "page-- : %d\n", paging->pageIdx)
        return;
    }

    if (paging->itemIdx > 0) {
        paging->itemIdx--;
        ZEMU_LOGF(50, "inner page--\n")
        paging->pageIdx = 0;
    }
}

zxerr_t h_inspect_update_data() {
    if (viewdata.viewfuncGetInnerNumItems == NULL) {
        zemu_log_stack("h_review_update_data - GetNumItems==NULL");
        return zxerr_no_data;
    }

    if (viewdata.viewfuncGetInnerItem == NULL) {
        zemu_log_stack("h_review_update_data - GetInnerItem==NULL");
        return zxerr_no_data;
    }

    // Make a copy cause we need to update the index for NanoS
    paging_t page = viewdata.innerField.paging;
    ZEMU_LOGF(50, "ItemsIdx: %d | %d\n", page.itemIdx, page.itemCount)
    CHECK_ZXERR(viewdata.viewfuncGetInnerNumItems(&viewdata.innerField.paging.itemCount))

#ifdef TARGET_NANOS
    viewdata.innerField.paging.itemCount += 2;
    viewdata.innerField.paging.pageCount = 1;
    if (h_paging_inspect_go_to_root_screen()) {
        snprintf(viewdata.key, MAX_CHARS_PER_KEY_LINE, "%s", "");
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "%s", "Go to root");
        splitValueField();
        page.pageIdx = 0;
        return zxerr_ok;
    }

    if (h_paging_inspect_back_screen()) {
        snprintf(viewdata.key, MAX_CHARS_PER_KEY_LINE, "%s", "");
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "%s", "BACK");
        splitValueField();
        page.pageIdx = 0;
        return zxerr_ok;
    }

    page.itemIdx--;
#endif

    ui_field_t thisItem = {
        .displayIdx = page.itemIdx,
        .outKey = viewdata.key,
        .outKeyLen = MAX_CHARS_PER_KEY_LINE,
        .outVal = viewdata.value,
        .outValLen = MAX_CHARS_PER_VALUE1_LINE,
        .pageIdx = viewdata.innerField.paging.pageIdx,
        .pageCount = &viewdata.innerField.paging.pageCount,
    };

    CHECK_ZXERR(viewdata.viewfuncGetInnerItem(viewdata.innerField.level, viewdata.innerField.trace, &thisItem))

    return zxerr_ok;
}
#endif
