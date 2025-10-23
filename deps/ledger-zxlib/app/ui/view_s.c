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
#include "bolos_target.h"

#if defined(TARGET_NANOS)

#include "apdu_codes.h"
#include "app_mode.h"
#include "bagl.h"
#include "ux.h"
#include "view.h"
#include "view_internal.h"
#include "view_nano.h"
#include "view_nano_inspect.h"
#include "view_templates.h"
#include "zxmacros.h"
#include "zxutils_ledger.h"

#define BAGL_WIDTH 128
#define BAGL_HEIGHT 32
#define BAGL_WIDTH_MARGIN 10

static bool exceed_pixel_in_display(const uint8_t length);

void account_enabled();
void shortcut_enabled();

static void h_expert_toggle();
static void h_expert_update();
static void h_review_button_left();
static void h_review_button_right();
static void h_review_button_both();

bool is_accept_item();
void set_accept_item();
bool is_reject_item();
bool should_show_skip_menu_right();
bool should_show_skip_menu_left();

#ifdef APP_SECRET_MODE_ENABLED
static void h_secret_click();
#endif

#ifdef APP_ACCOUNT_MODE_ENABLED
static void h_account_toggle();
static void h_account_update();
#endif

#ifdef SHORTCUT_MODE_ENABLED
static void h_shortcut_toggle();
static void h_shortcut_update();
#endif

#ifdef APP_BLINDSIGN_MODE_ENABLED
static void h_blindsign_toggle();
static void h_blindsign_update();
#endif

// Keep track of whether we're in skip menu view
static bool is_in_skip_menu = false;

enum MAINMENU_SCREENS {
    SCREEN_HOME = 0,
    SCREEN_EXPERT,
#ifdef APP_ACCOUNT_MODE_ENABLED
    SCREEN_ACCOUNT,
#endif
#ifdef SHORTCUT_MODE_ENABLED
    SCREEN_SHORTCUT,
#endif
#ifdef APP_BLINDSIGN_MODE_ENABLED
    SCREEN_BLINDSIGN,
#endif
};

ux_state_t ux;
extern ux_menu_state_t ux_menu;
extern unsigned int review_type;

void os_exit(uint32_t id) {
    (void)id;
    os_sched_exit(0);
}
static unsigned int view_skip_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter);
const bagl_element_t *view_prepro(const bagl_element_t *element);

// Add new view state for skip screen
static const bagl_element_t view_skip[] = {
    UI_BACKGROUND_LEFT_RIGHT_ICONS,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, "Press right to read"),
    UI_LabelLine(UIID_LABEL + 1, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, "Double-press to skip"),
};

const ux_menu_entry_t menu_main[] = {
    {NULL, NULL, 0, &C_icon_app, MENU_MAIN_APP_LINE1, viewdata.key, 33, 12},
    {NULL, h_expert_toggle, 0, &C_icon_app, "Expert mode:", viewdata.value, 33, 12},

#ifdef APP_BLINDSIGN_MODE_ENABLED
    {NULL, h_blindsign_toggle, 0, &C_icon_app, "Blind sign:", viewdata.value, 33, 12},
#endif

#ifdef APP_ACCOUNT_MODE_ENABLED
    {NULL, h_account_toggle, 0, &C_icon_app, "Account:", viewdata.value, 33, 12},
#endif

#ifdef SHORTCUT_MODE_ENABLED
    {NULL, h_shortcut_toggle, 0, &C_icon_app, "Shortcut mode:", viewdata.value, 33, 12},
#endif

    {NULL, NULL, 0, &C_icon_app, APPVERSION_LINE1, APPVERSION_LINE2, 33, 12},

    {NULL,
#ifdef APP_SECRET_MODE_ENABLED
     h_secret_click,
#else
     NULL,
#endif
     0, &C_icon_app, "Developed by:", "Zondax.ch", 33, 12},

    {NULL, NULL, 0, &C_icon_app, "License: ", "Apache 2.0", 33, 12},
    {NULL, os_exit, 0, &C_icon_dashboard, "Quit", NULL, 50, 29},
    UX_MENU_END};

const ux_menu_entry_t menu_initialize[] = {{NULL, NULL, 0, &C_icon_app, MENU_MAIN_APP_LINE1, "Not Ready", 33, 12},
                                           {NULL, h_initialize, 0, &C_icon_app, "Click to", "Initialize", 33, 12},
                                           {NULL, NULL, 0, &C_icon_app, APPVERSION_LINE1, APPVERSION_LINE2, 33, 12},
                                           {NULL, NULL, 0, &C_icon_app, "Developed by:", "Zondax.ch", 33, 12},
                                           {NULL, NULL, 0, &C_icon_app, "License: ", "Apache 2.0", 33, 12},
                                           {NULL, os_exit, 0, &C_icon_dashboard, "Quit", NULL, 50, 29},
                                           UX_MENU_END};

const ux_menu_entry_t menu_custom_error[] = {{NULL, NULL, 0, &C_icon_warning, viewdata.key, viewdata.value, 33, 12},
                                             {NULL, h_error_accept, 0, &C_icon_validate_14, "Ok", NULL, 50, 29},
                                             UX_MENU_END};

const ux_menu_entry_t blindsign_error[] = {{NULL, NULL, 0, &C_icon_warning, "Blindsing Mode", " Required", 33, 12},
                                           {NULL, h_error_accept, 0, &C_icon_validate_14, "Exit", NULL, 50, 29},
                                           UX_MENU_END};

static const bagl_element_t view_message[] = {
    UI_BACKGROUND,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 1, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
};

static const bagl_element_t view_review[] = {
    UI_BACKGROUND_LEFT_RIGHT_ICONS,
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 1, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
    UI_LabelLine(UIID_LABEL + 2, 0, 30, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value2),
};

static const bagl_element_t view_error[] = {
    UI_FillRectangle(0, 0, 0, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT, 0x000000, 0xFFFFFF),
    UI_Icon(0, 128 - 7, 0, 7, 7, BAGL_GLYPH_ICON_CHECK),
    UI_LabelLine(UIID_LABEL + 0, 0, 8, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
    UI_LabelLine(UIID_LABEL + 0, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value),
    UI_LabelLineScrolling(UIID_LABELSCROLL, 0, 30, 128, UI_11PX, UI_WHITE, UI_BLACK, viewdata.value2),
};

static const bagl_element_t view_spinner[] = {
    UI_BACKGROUND,
    UI_LabelLine(UIID_LABEL, 0, 19, UI_SCREEN_WIDTH, UI_11PX, UI_WHITE, UI_BLACK, viewdata.key),
};

static unsigned int view_error_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            h_error_accept(0);
            break;
    }
    return 0;
}

static unsigned int view_message_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            break;
    }
    return 0;
}

static unsigned int view_spinner_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            break;
    }
    return 0;
}

// Helper to check if we've completed reviewing an item
bool should_show_skip_menu_right() {
    // When going forwards: we're at last page of current item
    // When going backwards: we're at first page of current item
    return viewdata.with_confirmation &&
           (review_type == REVIEW_TXN || review_type == REVIEW_GROUP_TXN || review_type == REVIEW_MSG) &&
           // To enable left arrow rendering
           viewdata.pageIdx > 0 &&
           // only if all item's pages has been rendered
           viewdata.pageIdx == viewdata.pageCount - 1 &&
           // Not in approve screen
           // Not in reject screen
           !is_accept_item() && !is_reject_item() &&
           // if we are not in the skip menu already
           !is_in_skip_menu;
}

// Helper to check if we should show skip menu
bool should_show_skip_menu_left() {
    return viewdata.with_confirmation &&
           (review_type == REVIEW_TXN || review_type == REVIEW_GROUP_TXN || review_type == REVIEW_MSG) &&
           viewdata.itemIdx > 0 &&  // Not the first item
           // if all pages have been rendered
           // Reached first page of current item
           viewdata.pageIdx == 0 &&
           // Not in approve screen
           // Not in reject screen
           !is_accept_item() && !is_reject_item() &&
           // if we are not in the skip menu already
           !is_in_skip_menu;
}

static unsigned int view_review_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Only handle double-click if we're in skip menu or approve/reject screens
            if (is_in_skip_menu || is_accept_item() || is_reject_item()) {
                h_review_button_both();
            }
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Check if we should show skip menu before moving back
            if (should_show_skip_menu_left()) {
                is_in_skip_menu = true;
                UX_DISPLAY(view_skip, view_prepro);
            } else {
                is_in_skip_menu = false;
                h_review_button_left();
            }
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            if (should_show_skip_menu_right()) {
                is_in_skip_menu = true;  // Entering skip menu
                UX_DISPLAY(view_skip, view_prepro);
            } else {
                is_in_skip_menu = false;
                h_review_button_right();
            }
            break;
    }
    return 0;
}

const bagl_element_t *idle_preprocessor(__Z_UNUSED const ux_menu_entry_t *entry, bagl_element_t *element) {
    switch (ux_menu.current_entry) {
        case SCREEN_HOME:
            break;
        case SCREEN_EXPERT:
            h_expert_update();
            break;
#ifdef APP_BLINDSIGN_MODE_ENABLED
        case SCREEN_BLINDSIGN:
            h_blindsign_update();
            break;
#endif
#ifdef APP_ACCOUNT_MODE_ENABLED
        case SCREEN_ACCOUNT:
            h_account_update();
            break;
#endif
#ifdef SHORTCUT_MODE_ENABLED
        case SCREEN_SHORTCUT:
            h_shortcut_update();
            break;
#endif
        default:
            break;
    }
    return element;
}

const bagl_element_t *view_prepro(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT:
            if (!h_paging_can_decrease() || h_paging_inspect_go_to_root_screen()) {
                return NULL;
            }
            UX_CALLBACK_SET_INTERVAL(2000)
            break;
        case UIID_ICONRIGHT:
            if (!h_paging_can_increase() || h_paging_inspect_back_screen()) {
                return NULL;
            }
            UX_CALLBACK_SET_INTERVAL(2000)
            break;
        case UIID_ICONREVIEW:
            if (!h_paging_intro_screen()) {
                return NULL;
            }
            UX_CALLBACK_SET_INTERVAL(2000)
            break;
        case UIID_LABELSCROLL:
            UX_CALLBACK_SET_INTERVAL(MAX(3000, 1000 + bagl_label_roundtrip_duration_ms(element, 7)));
            break;
    }
    return element;
}

const bagl_element_t *view_prepro_idle(const bagl_element_t *element) {
    switch (element->component.userid) {
        case UIID_ICONLEFT:
        case UIID_ICONRIGHT:
            return NULL;
    }
    return element;
}

void h_review_update() {
    zxerr_t err = h_review_update_data();
    switch (err) {
        case zxerr_ok:
            UX_DISPLAY(view_review, view_prepro)
            break;
        default:
            view_error_show();
            break;
    }
}

void h_review_button_left() {
    zemu_log_stack("h_review_button_left");
    h_paging_decrease();
    h_review_update();
}

void h_review_button_right() {
    zemu_log_stack("h_review_button_right");
    h_paging_increase();
    h_review_update();
}

static void h_review_action(unsigned int requireReply) {
    if (is_in_skip_menu) {
        // Force jump to approval screen
        set_accept_item();

        is_in_skip_menu = false;  // Reset the flag after handling
        h_review_update();

        return;
    }

    if (is_accept_item()) {
        zemu_log_stack("action_accept");
        h_approve(1);
        return;
    }

    if (is_reject_item()) {
        zemu_log_stack("action_reject");
        h_reject(requireReply);
        return;
    }

    zemu_log_stack("quick accept");
    if (app_mode_shortcut()) {
        set_accept_item();
        h_review_update();
        return;
    }

    inspect_init();
}

void h_review_button_both() {
    zemu_log_stack("h_review_button_both");

    // Handle double-click when in skip menu or approve/reject screens
    if (is_in_skip_menu || is_accept_item() || is_reject_item()) {
        is_in_skip_menu = false;
        h_review_action(review_type);
    }
}

//////////////////////////
//////////////////////////
//////////////////////////
//////////////////////////
//////////////////////////

void view_initialize_show_impl(uint8_t item_idx, const char *statusString) {
    if (statusString == NULL) {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", MENU_MAIN_APP_LINE2);
    } else {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", statusString);
    }
    UX_MENU_DISPLAY(item_idx, menu_initialize, NULL);
}

void view_idle_show_impl(uint8_t item_idx, const char *statusString) {
    if (statusString == NULL) {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", MENU_MAIN_APP_LINE2);
#ifdef APP_SECRET_MODE_ENABLED
        if (app_mode_secret()) {
            snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", MENU_MAIN_APP_LINE2_SECRET);
        }
#endif
    } else {
        snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", statusString);
    }
    UX_MENU_DISPLAY(item_idx, menu_main, idle_preprocessor);
}

void view_message_impl(const char *title, const char *message) {
    snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", title);
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "%s", message);
    UX_DISPLAY(view_message, view_prepro_idle)
}

void view_error_show_impl() { UX_DISPLAY(view_error, view_prepro) }

void view_custom_error_show_impl() { UX_MENU_DISPLAY(0, menu_custom_error, NULL); }

void view_blindsign_error_show_impl() { UX_MENU_DISPLAY(0, blindsign_error, NULL); }

void view_spinner_impl(const char *text) {
    snprintf(viewdata.key, MAX_CHARS_PER_VALUE_LINE, "%s", text);
    UX_DISPLAY(view_spinner, view_prepro_idle)
    UX_WAIT_DISPLAYED()
}

void h_expert_toggle() {
    app_mode_set_expert(!app_mode_expert());
    view_idle_show(1, NULL);
}

void h_expert_update() {
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "disabled");
    if (app_mode_expert()) {
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "enabled");
    }
}

#ifdef APP_BLINDSIGN_MODE_ENABLED
void h_blindsign_toggle() {
    app_mode_set_blindsign(!app_mode_blindsign());
    view_idle_show(SCREEN_BLINDSIGN, NULL);
}

void h_blindsign_update() {
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "disabled");
    if (app_mode_blindsign()) {
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE_LINE, "enabled");
    }
}
#endif

#ifdef APP_ACCOUNT_MODE_ENABLED
void h_account_toggle() {
    if (app_mode_expert()) {
        account_enabled();
    } else {
        view_idle_show(2, NULL);
    }
}

void h_account_update() {
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, ACCOUNT_DEFAULT);
    if (app_mode_account()) {
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, ACCOUNT_SECONDARY);
    }
}
#endif

#ifdef SHORTCUT_MODE_ENABLED
void h_shortcut_toggle() {
    if (app_mode_expert() && !app_mode_shortcut()) {
        shortcut_enabled();
        return;
    }
    app_mode_set_shortcut(0);
    view_idle_show(SCREEN_SHORTCUT, NULL);
}

void h_shortcut_update() {
    snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "disabled");
    if (app_mode_shortcut()) {
        snprintf(viewdata.value, MAX_CHARS_PER_VALUE1_LINE, "enabled");
    }
}
#endif

#ifdef APP_SECRET_MODE_ENABLED
void h_secret_click() {
    if (COIN_SECRET_REQUIRED_CLICKS == 0) {
        // There is no secret mode
        return;
    }

    viewdata.secret_click_count++;

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "secret click %d\n", viewdata.secret_click_count);
    zemu_log(buffer);

    if (viewdata.secret_click_count >= COIN_SECRET_REQUIRED_CLICKS) {
        secret_enabled();
        viewdata.secret_click_count = 0;
    }
}
#endif

void view_review_show_impl(unsigned int requireReply, const char *title, const char *validate) {
    UNUSED(title);
    UNUSED(validate);
    zemu_log_stack("view_review_show_impl");
    review_type = requireReply;

    h_paging_init();

    zxerr_t err = h_review_update_data();
    switch (err) {
        case zxerr_ok:
            UX_DISPLAY(view_review, view_prepro)
            break;
        default:
            view_error_show();
            break;
    }
}

void splitValueField() {
    print_value2("");
    const uint16_t vlen = (uint16_t)strnlen(viewdata.value, MAX_CHARS_PER_VALUE1_LINE);
    if (vlen > MAX_CHARS_PER_VALUE2_LINE - 1) {
        snprintf(viewdata.value2, MAX_CHARS_PER_VALUE2_LINE, "%s", viewdata.value + MAX_CHARS_PER_VALUE_LINE);
        viewdata.value[MAX_CHARS_PER_VALUE_LINE] = 0;
    }
}
void splitValueAddress() {
    uint8_t len = MAX_CHARS_PER_VALUE_LINE;
    bool exceeding_max = exceed_pixel_in_display(len);
    while (exceeding_max && len--) {
        exceeding_max = exceed_pixel_in_display(len);
    }
    print_value2("");
    const uint16_t vlen = (uint16_t)strnlen(viewdata.value, MAX_CHARS_PER_VALUE1_LINE);
    // if viewdata.value == NULL --> len = 0
    if (vlen > len && len > 0) {
        snprintf(viewdata.value2, MAX_CHARS_PER_VALUE2_LINE, "%s", viewdata.value + len);
        viewdata.value[len] = 0;
    }
}

max_char_display get_max_char_per_line() {
    uint8_t len = MAX_CHARS_PER_VALUE_LINE;
    bool exceeding_max = exceed_pixel_in_display(len);
    while (exceeding_max && len--) {
        exceeding_max = exceed_pixel_in_display(len);
    }
    // MAX_CHARS_PER_VALUE1_LINE is defined this way
    return (len > 0) ? (2 * len + 1) : len;
}

bool exceed_pixel_in_display(const uint8_t length) {
    const unsigned short strWidth = zx_compute_line_width_light(viewdata.value, length);
    return (strWidth >= (BAGL_WIDTH - BAGL_WIDTH_MARGIN));
}

static unsigned int view_skip_button(unsigned int button_mask, __Z_UNUSED unsigned int button_mask_counter) {
    switch (button_mask) {
        case BUTTON_EVT_RELEASED | BUTTON_LEFT | BUTTON_RIGHT:
            // Skip to approve
            h_review_action(review_type);
            break;
        case BUTTON_EVT_RELEASED | BUTTON_RIGHT:
            // Continue review
            is_in_skip_menu = false;
            h_review_button_right();
            break;
        case BUTTON_EVT_RELEASED | BUTTON_LEFT:
            // Go back
            is_in_skip_menu = false;
            h_review_button_left();
            break;
    }
    return 0;
}

void view_review_show_with_intent_impl(unsigned int requireReply, const char *intent) {
    // For Nano S, we don't have space to display the intent in the title
    // Just use the normal review flow
    UNUSED(intent);
    view_review_show_impl(requireReply, NULL, NULL);
}

#endif
