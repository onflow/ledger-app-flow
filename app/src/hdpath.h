#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "coin.h"

extern hd_path_t hdPath;
extern uint16_t cryptoOptions;

typedef enum {
    SHOW_ADDRESS_NONE = 0, //result undefined
    SHOW_ADDRESS_YES = 1, //we have address
    SHOW_ADDRESS_EMPTY_SLOT = 2, //slot 0 is empty 
    SHOW_ADDRESS_HDPATHS_NOT_EQUAL = 3, //hdpath on slot 0 does not equal hdPath
    SHOW_ADDRESS_ERROR = 4, //error occoured - In menu we cannot handle errors by throwing
} show_address_t;
extern show_address_t show_address;
extern flow_account_t address_to_display;
extern uint8_t addressUsedInTx;


#ifdef __cplusplus
}
#endif
