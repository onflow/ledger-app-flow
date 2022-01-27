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
        STATIC_ASSERT(sizeof(hdPath.data) == sizeof(slot.path.data), "Incompatible derivation path types");
        memcpy(hdPath.data, slot.path.data, sizeof(hdPath.data));

        //extract pubkey to pubkey_to_display global variable
        MEMZERO(pubkey_to_display, sizeof(pubkey_to_display));
        zxerr_t err = crypto_extractPublicKey(hdPath, pubkey_to_display, sizeof(pubkey_to_display));
        if (err ==  zxerr_ok) {
            STATIC_ASSERT(sizeof(address_to_display.data) == sizeof(slot.account.data),  "Incompatible derivation address types");
            memcpy(address_to_display.data, slot.account.data, sizeof(address_to_display.data));
        }
        {
            zemu_log_stack("Public key extraction errorr");
            show_address = show_address_error;
        }
    }

    view_review_init(addr_getItem, addr_getNumItems, menuaddr_return);
    view_review_show();

    return;
}
