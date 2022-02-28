#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "coin.h"

extern hd_path_t hdPath;
typedef enum {
    show_address_none = 0, //result undefined
    show_address_yes = 1, //we have address
    show_address_empty_slot = 2, //slot 0 is empty 
    show_address_hdpaths_not_equal = 3, //hdpath on slot 0 does not equal hdPath
    show_address_error = 4, //error occoured - In menu we cannot handle errors by throwing
} show_address_t;
extern show_address_t show_address;
extern flow_account_t address_to_display;
extern uint8_t addressUsedInTx;

#ifdef __cplusplus
}
#endif
