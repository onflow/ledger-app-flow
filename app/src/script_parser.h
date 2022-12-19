#include <zxmacros.h>
#include <stdbool.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SCRIPT_PARSED_ELEMENTS 11

typedef enum {
    SCRIPT_TYPE_UNKNOWN = 0x00,
    SCRIPT_TYPE_NFT_SETUP_COLLECTION = 0x01,
    SCRIPT_TYPE_NFT_TRANSFER = 0x02,
} script_parsed_type_t;

typedef struct {
    const NV_VOLATILE uint8_t *data;
    size_t length;
} script_element_t;

typedef struct {
    script_parsed_type_t script_type;
    size_t elements_count;
    script_element_t elements[MAX_SCRIPT_PARSED_ELEMENTS];
} script_parsed_elements_t;

bool parseScript(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize, 
                 const uint8_t *scriptTemplate, size_t scriptTemplateSize);

typedef enum {
    PARSED_ELEMENTS_NFT1_NON_FUNGIBLE_TOKEN_ADDRESS = 0,
    PARSED_ELEMENTS_NFT1_METADATA_VIEWS_ADDRESS = 1,
    PARSED_ELEMENTS_NFT1_CONTRACT_NAME = 2,
    PARSED_ELEMENTS_NFT1_CONTRACT_ADDRESS = 3,
    PARSED_ELEMENTS_NFT1_STORAGE_PATH = 4,
    PARSED_ELEMENTS_NFT1_CONTRACT_NAME2 = 5,
    PARSED_ELEMENTS_NFT1_STORAGE_PATH2 = 6,
    PARSED_ELEMENTS_NFT1_PUBLIC_COLLECTION_CONTRACT_NAME = 7,
    PARSED_ELEMENTS_NFT1_PUBLIC_COLLECTION_NAME = 8,
    PARSED_ELEMENTS_NFT1_PUBLIC_PATH = 9,
    PARSED_ELEMENTS_NFT1_STORAGE_PATH3 = 10,

    PARSED_ELEMENTS_NFT1_COUNT = 11,
} parsed_elements_nft1_index_t;


bool parseNFT1(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize);

typedef enum {
    PARSED_ELEMENTS_NFT2_NON_FUNGIBLE_TOKEN_ADDRESS = 0,
    PARSED_ELEMENTS_NFT2_CONTRACT_NAME = 1,
    PARSED_ELEMENTS_NFT2_CONTRACT_ADDRESS = 2,
    PARSED_ELEMENTS_NFT2_STORAGE_PATH = 3,
    PARSED_ELEMENTS_NFT2_CONTRACT_NAME2 = 4,
    PARSED_ELEMENTS_NFT2_CONTRACT_NAME3 = 5,
    PARSED_ELEMENTS_NFT2_STORAGE_PATH2 = 6,
    PARSED_ELEMENTS_NFT2_PUBLIC_PATH = 7,

    PARSED_ELEMENTS_NFT2_COUNT = 8,
} parsed_elements_nft2_index_t;

bool parseNFT2(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize);


#ifdef __cplusplus
} //end extern C
#endif
