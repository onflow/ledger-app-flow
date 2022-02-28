#include "hdpath.h"
#include "zxerror.h"
#include "account.h"

hd_path_t hdPath;
show_address_t show_address;
flow_account_t address_to_display;
uint8_t addressUsedInTx;

//This variables are used by parser.c and thus are used in cpp unit tests. 
//Bear this in mind if you want to add some function here.