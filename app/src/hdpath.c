#include "hdpath.h"
#include "zxerror.h"
#include "account.h"

hd_path_t hdPath;
show_address_t show_address;
flow_account_t address_to_display;
uint8_t addressUsedInTx;

void loadAddressFromSlot(uint8_t hasHdPath) {
    account_slot_t slot;
    zxerr_t err = slot_getSlot(MAIN_SLOT, (uint8_t *) &slot, sizeof(slot));

    if (!(err == zxerr_no_data  || err == zxerr_ok)) {
        show_address = show_address_error;
        return;
    }

    //Case 1 Empty slot 0 
    if (err == zxerr_no_data) {
        show_address = show_address_empty_slot;
    } 
    else {
        //Case 2 Slot 0 derivation path is not the same as APDU derivation path
        STATIC_ASSERT(sizeof(slot.path.data) == sizeof(hdPath.data), "Incompatible derivation path types");
        if (hasHdPath && memcmp(slot.path.data, hdPath.data, sizeof(hdPath.data))) {
            show_address = show_address_hdpaths_not_equal;
        }
        else {
            //Case 3 Everything is OK
            STATIC_ASSERT(sizeof(address_to_display.data) == sizeof(slot.account.data), "Incompatible address types");
            memcpy(address_to_display.data, slot.account.data, sizeof(address_to_display.data));
            show_address = show_address_yes;

            //load hdPath from slot if necessary
            if (!hasHdPath) {
                STATIC_ASSERT(sizeof(hdPath.data) == sizeof(slot.path.data), "Incompatible derivation path types");
                memcpy(hdPath.data, slot.path.data, sizeof(hdPath.data));
            }
        }
    }
}

void loadHdPathAndAddressFromSlot() {
    loadAddressFromSlot(0);
}

void loadAddressCompareHdPathFromSlot() {
    loadAddressFromSlot(1);
}
