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


zxerr_t menuaddr_getNumItems(uint8_t *num_items) {
    zemu_log_stack("menuaddr_getNumItems");
    *num_items = 2;
    return zxerr_ok;
}

zxerr_t menuaddr_getItem(int8_t displayIdx,
                     char *outKey, uint16_t outKeyLen,
                     char *outVal, uint16_t outValLen,
                     uint8_t pageIdx, uint8_t *pageCount) {
    zemu_log_stack("addr_getItem");
    switch (displayIdx) {
        case 0:
            snprintf(outKey, outKeyLen, "Pub Key");
            snprintf(outVal, outValLen, "Pub key here");
//            pageString(outVal, outValLen, (char *) (G_io_apdu_buffer + VIEW_ADDRESS_OFFSET_SECP256K1 + 2), pageIdx, pageCount);
            return zxerr_ok;
        case 1: {
            snprintf(outKey, outKeyLen, "Address");
            snprintf(outVal, outValLen, "Address here");
/*            snprintf(outKey, outKeyLen, "Your Path");
            char buffer[300];
            bip32_to_str(buffer, sizeof(buffer), hdPath, HDPATH_LEN_DEFAULT);
            pageString(outVal, outValLen, buffer, pageIdx, pageCount);*/
            return zxerr_ok;
        }
        default:
            return zxerr_no_data;
    }
}

__Z_INLINE void menuaddr_return() {
    view_idle_show(3, NULL);
}

void handleMenuShowAddress() {
    app_fill_address();

    view_review_init(menuaddr_getItem, menuaddr_getNumItems, menuaddr_return);
    view_review_show();

    return;
}
