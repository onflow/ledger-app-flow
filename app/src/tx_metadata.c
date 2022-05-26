#include "tx_metadata.h"
#include "zxmacros.h"

//Known tx metadata

const uint8_t TX_METADATA_CREATE_ACCOUNT[] = {
    1, //number of hashes + hashes
    0xee, 0xf2, 0xd0, 0x49, 0x44, 0x48, 0x55, 0x41, 0x77, 0x61, 0x2e, 0x63, 0x02, 0x62, 0x56, 0x25, 0x83, 0x39, 0x23, 0x0c, 0xbc, 0x69, 0x31, 0xde, 0xd7, 0x8d, 0x61, 0x49, 0x44, 0x3c, 0x61, 0x73,
    'C', 'r', 'e', 'a', 't', 'e', ' ', 'A', 'c', 'c', 'o', 'u', 'n', 't', 0,  //tx name (to display)
    1,  //number of arguments

    //Argument 1
    ARGUMENT_TYPE_ARRAY,
    'P', 'u', 'b', 'k', 'e', 'y', 0, //arg name (to display)
    0, //argument index
    'S','t', 'r', 'i', 'n', 'g',  0, //expected value type
    JSMN_STRING, //expected value json token type
};

const uint8_t TX_METADATA_ADD_NEW_KEY[] = {
    1, //number of hashes + hashes
    0x59, 0x5c, 0x86, 0x56, 0x14, 0x41, 0xb3, 0x2b, 0x2b, 0x91, 0xee, 0x03, 0xf9, 0xe1, 0x0c, 0xa6, 0xef, 0xa7, 0xb4, 0x1b, 0xcc, 0x99, 0x4f, 0x51, 0x31, 0x7e, 0xc0, 0xaa, 0x9d, 0x8f, 0x8a, 0x42,
    'A', 'd', 'd', ' ', 'N', 'e', 'w', ' ', 'K', 'e', 'y', 0,  //tx name (to display)
    1,  //number of arguments

    //Argument 1
    ARGUMENT_TYPE_NORMAL,
    'P', 'u', 'b', ' ', 'k', 'e', 'y', 0, //arg name (to display)
    0, //argument index
    'S','t', 'r', 'i', 'n', 'g',  0, //expected value type
    JSMN_STRING, //expected value json token type
};

const uint8_t TX_METADATA_TOKEN_TRANSFER[] = {
    3, //number of hashes + hashes
    0xca, 0x80, 0xb6, 0x28, 0xd9, 0x85, 0xb3, 0x58, 0xae, 0x1c, 0xb1, 0x36, 0xbc, 0xd9, 0x76, 0x99, 0x7c, 0x94, 0x2f, 0xa1, 0x0d, 0xba, 0xbf, 0xea, 0xfb, 0x4e, 0x20, 0xfa, 0x66, 0xa5, 0xa5, 0xe2,
    0xd5, 0x6f, 0x4e, 0x1d, 0x23, 0x55, 0xcd, 0xcf, 0xac, 0xfd, 0x01, 0xe4, 0x71, 0x45, 0x9c, 0x6e, 0xf1, 0x68, 0xbf, 0xdf, 0x84, 0x37, 0x1a, 0x68, 0x5c, 0xcf, 0x31, 0xcf, 0x3c, 0xde, 0xdc, 0x2d,
    0x47, 0x85, 0x15, 0x86, 0xd9, 0x62, 0x33, 0x5e, 0x3f, 0x7d, 0x9e, 0x5d, 0x11, 0xa4, 0xc5, 0x27, 0xee, 0x4b, 0x5f, 0xd1, 0xc3, 0x89, 0x5e, 0x3c, 0xe1, 0xb9, 0xc2, 0x82, 0x1f, 0x60, 0xb1, 0x66,
    'T', 'o', 'k', 'e', 'n', ' ', 'T', 'r', 'a', 'n', 's', 'f', 'e', 'r', 0,  //tx name (to display)
    2,  //number of arguments

    //Argument 1
    ARGUMENT_TYPE_NORMAL,
    'A', 'm', 'o', 'u', 'n', 't', 0, //arg name (to display)
    0, //argument index
    'U','I', 'n', 't', '6', '4',  0, //expected value type
    JSMN_STRING, //expected value json token type

    //Argument 2
    ARGUMENT_TYPE_NORMAL,
    'D', 'e', 's', 't', 'i', 'n', 'a', 't', 'i', 'o', 'n', 0, //arg name (to display)
    1, //argument index
    'A','d', 'd', 'r', 'e', 's', 's', 0, //expected value type
    JSMN_STRING, //expected value json token type
};


//List of known compressed metadata
typedef struct {
    const uint8_t* metadata;
    uint16_t metadataLength;
} known_tx_metadata_entry_t;

const known_tx_metadata_entry_t KNOWN_TX_METADATA_[] = {
    {TX_METADATA_CREATE_ACCOUNT, sizeof(TX_METADATA_CREATE_ACCOUNT)},
    {TX_METADATA_ADD_NEW_KEY, sizeof(TX_METADATA_ADD_NEW_KEY)},
    {TX_METADATA_TOKEN_TRANSFER, sizeof(TX_METADATA_TOKEN_TRANSFER)},
    // sentinel, do not remove
    {NULL, 0}
};

parser_error_t _validateHash(uint8_t scriptHash[SCRIPT_HASH_SIZE], const NV_VOLATILE uint8_t *txMetadata, uint16_t txMetadataLength) {
    if (txMetadataLength < 1) {
        return PARSER_TEMPLATE_ERROR;
    }
    uint8_t numberOfHashes = txMetadata[0];
    if (numberOfHashes > MAX_TEMPLATE_NUMBER_OF_HASHES || txMetadataLength < 1+SCRIPT_HASH_SIZE*numberOfHashes) {
        return PARSER_TEMPLATE_ERROR;
    }

    for(size_t i=0; i<numberOfHashes; i++) {
        uint8_t thisHashMatches = 1;
        for(int j=0; j<SCRIPT_HASH_SIZE; j++) {
            uint8_t hashByte = txMetadata[1+i*SCRIPT_HASH_SIZE+j];
            if (hashByte != scriptHash[j]) {
                thisHashMatches = 0;
                break;
            }
        }
        if (thisHashMatches == 1) {
            return PARSER_OK;
        }
    }

    return PARSER_UNEXPECTED_SCRIPT;
}


parser_error_t parseTxMetadata(uint8_t scriptHash[SCRIPT_HASH_SIZE], const NV_VOLATILE uint8_t *txMetadata, uint16_t txMetadataLength, 
                               parsed_tx_metadata_t *parsedTxMetadata) {
    uint16_t parsed = 0;
        
    #define READ_CHAR(where) \
        { \
            if (!(parsed < txMetadataLength)) { \
                return PARSER_TEMPLATE_ERROR; \
            } \
            (*where) = txMetadata[parsed++]; \
        } 
    #define READ_STRING(dest_pointer, len) { \
        *(len) = 0; \
        *(dest_pointer) = (char *) txMetadata + parsed; \
        while (*(len) <= MAX_TEMPLATE_STRING_LENGTH) { \
            uint8_t byte = 0; \
            READ_CHAR(&byte); \
            if (byte == 0) { \
                break; \
            } \
            (*(len))++; \
        } \
        if (*(len) > MAX_TEMPLATE_STRING_LENGTH) { \
            return PARSER_TEMPLATE_ERROR; \
        } \
    }
    #define READ_SKIP(count) { \
        parsed += (count); \
    }

    //read number of hashes and validate script
    {
        uint8_t numberOfHashes = 0;
        READ_CHAR(&numberOfHashes)
        if (numberOfHashes > MAX_TEMPLATE_NUMBER_OF_HASHES) {
            return PARSER_TEMPLATE_TOO_MANY_HASHES;
        }
        parser_error_t err = _validateHash(scriptHash, txMetadata, txMetadataLength);
        if (err != PARSER_OK) {
            return err;
        }
        READ_SKIP(numberOfHashes*SCRIPT_HASH_SIZE)
    }

    //read tx name
    READ_STRING(&parsedTxMetadata->txName, &parsedTxMetadata->txNameLength)

    //read Arguments
    {
        READ_CHAR(&parsedTxMetadata->argCount)
        if (parsedTxMetadata->argCount > PARSER_MAX_ARGCOUNT) {
            return PARSER_TEMPLATE_TOO_MANY_ARGUMENTS;
        }
        STATIC_ASSERT(sizeof(parsedTxMetadata->arguments) >= PARSER_MAX_ARGCOUNT, "Too few arguments in parsed_tx_metadata_t.");
        for(int i=0; i<parsedTxMetadata->argCount; i++) {
            READ_CHAR(&parsedTxMetadata->arguments[i].argumentType);
            READ_STRING(&parsedTxMetadata->arguments[i].displayKey, &parsedTxMetadata->arguments[i].displayKeyLength)
            READ_CHAR(&parsedTxMetadata->arguments[i].argumentIndex);
            READ_STRING(&parsedTxMetadata->arguments[i].jsonExpectedType, &parsedTxMetadata->arguments[i].jsonExpectedTypeLength)
            READ_CHAR(&parsedTxMetadata->arguments[i].jsonExpectedKind);
        }
    }


    #undef READ_CHAR 
    #undef READ_STRING

    if (parsed != txMetadataLength) {
        return PARSER_TEMPLATE_ERROR;
    }

    return PARSER_OK;
}

parser_error_t matchStoredTxMetadata(uint8_t scriptHash[SCRIPT_HASH_SIZE], parsed_tx_metadata_t *parsedTxTempate) {
    size_t i=0;
    while(KNOWN_TX_METADATA_[i].metadata != NULL) {
        parser_error_t err = _validateHash(scriptHash, PIC(KNOWN_TX_METADATA_[i].metadata), KNOWN_TX_METADATA_[i].metadataLength);
        switch (err) {
            case PARSER_OK:
                return parseTxMetadata(scriptHash, PIC(KNOWN_TX_METADATA_[i].metadata), KNOWN_TX_METADATA_[i].metadataLength, parsedTxTempate);
            case PARSER_UNEXPECTED_SCRIPT:
                break;  //break switch
            default:
                return err;
        }
        i++;
    }
    
    return PARSER_UNEXPECTED_SCRIPT;
}
