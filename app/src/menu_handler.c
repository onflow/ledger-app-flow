#include <string.h>
#include <os_io_seproxyhal.h>
#include <os.h>

#include "view.h"
#include "hdpath.h"
#include "addr.h"
#include "crypto.h"
#include "account.h"

__Z_INLINE void menuaddr_return() {
    view_idle_show(0, "Ready");
}

void handleMenuShowAddress() {
    hasPubkey = false;
    show_address = SHOW_ADDRESS_NONE;

    loadHdPathAndAddressFromSlot();

    if (show_address == SHOW_ADDRESS_YES) {
        //extract pubkey to pubkey_to_display global variable
        MEMZERO(pubkey_to_display, sizeof(pubkey_to_display));
        zxerr_t err = crypto_extractPublicKey(hdPath, cryptoOptions, pubkey_to_display, sizeof(pubkey_to_display));
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
