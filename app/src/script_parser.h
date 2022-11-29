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

bool parseNFT1(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize);

bool parseNFT2(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize);


#ifdef __cplusplus
} //end extern C
#endif
