#include <string.h>
#include <os_io_seproxyhal.h>
#include <os.h>

#include "view.h"
#include "actions.h"
#include "tx.h"
#include "addr.h"
#include "crypto.h"
#include "coin.h"
#include "account.h"
#include "zxmacros.h"

#define ASSERT(CONDITION) { if (!(CONDITION)) {THROW(APDU_CODE_UNKNOWN);}; }

__Z_INLINE void menuaddr_return() {
    view_idle_show(0, NULL);
}

void handleMenuShowAddress() {
    account_slot_t slot;
    zxerr_t err = slot_getSlot(MAIN_SLOT, (uint8_t *) &slot, sizeof(slot));
    if (err == zxerr_no_data) {
        show_address = show_address_no_pubkey;
    }
    else {
        show_address = show_address_yes;
        ASSERT(sizeof(hdPath) == sizeof(slot.path.data));
        memcpy(hdPath, slot.path.data, sizeof(hdPath));

        //extract pubkey to pubkey_to_display global variable
        MEMZERO(pubkey_to_display, sizeof(pubkey_to_display));
        zxerr_t err = crypto_extractPublicKey(hdPath, pubkey_to_display, sizeof(pubkey_to_display));
        if (err !=  zxerr_ok) {
            zemu_log_stack("Public key extraction erorr");
            THROW(APDU_CODE_UNKNOWN);
        }

        ASSERT(sizeof(address_to_display) == sizeof(slot.account.data));
        memcpy(address_to_display, slot.account.data, sizeof(address_to_display));
    }

    view_review_init(addr_getItem, addr_getNumItems, menuaddr_return);
    view_review_show();

    return;
}
