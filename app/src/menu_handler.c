#include <string.h>
#include <os_io_seproxyhal.h>
#include <os.h>

#include "view.h"
#include "hdpath.h"
#include "addr.h"
#include "crypto.h"
#include "account.h"

__Z_INLINE void menuaddr_return() {
    view_idle_show(0, NULL);
}

void handleMenuShowAddress() {
    hasPubkey = false;
    show_address = show_address_none;

    loadHdPathAndAddressFromSlot();

    if (show_address == show_address_yes) {
        //extract pubkey to pubkey_to_display global variable
        MEMZERO(pubkey_to_display, sizeof(pubkey_to_display));
        zxerr_t err = crypto_extractPublicKey(hdPath, pubkey_to_display, sizeof(pubkey_to_display));
        if (err ==  zxerr_ok) {
            hasPubkey = true;
        }
        else {
            zemu_log_stack("Public key extraction error.");
        }
    }

    view_review_init(addr_getItem, addr_getNumItems, menuaddr_return);
    view_review_show();

    return;
}
