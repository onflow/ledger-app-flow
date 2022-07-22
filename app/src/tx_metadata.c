#include "tx_metadata.h"
#include "zxmacros.h"
#include "crypto.h"
#include "parser_common.h"

#define MAX_METADATA_NUMBER_OF_HASHES 10
#define MAX_METADATA_STRING_LENGTH 100
#define MAX_METADATA_MAX_ARRAY_ITEMS 20

#define METADATA_MERKLE_TREE_LEVELS 4

//Metadata
#define MAX_METADATA_LENGTH 255
struct {
    uint8_t metadataLength;
    uint8_t buffer[MAX_METADATA_LENGTH];
    uint8_t metadataMerkleTreeValidationLevel;
    uint8_t metadataMerkleTreeValidationHash[METADATA_HASH_SIZE];
} txMetadataState;

static const uint8_t merkleTreeRoot[METADATA_HASH_SIZE] = {
    0x71, 0xb3, 0xf3, 0x72, 0x5f, 0xbb, 0x0f, 0x2d, 0x59, 0x3d, 0xd6, 0x45, 0x70, 0xef, 0xe7, 0x5f, 0x55, 0x61, 0x87, 0x36, 0x6c, 0x19, 0xba, 0x94, 0x27, 0x24, 0x74, 0xe8, 0x0e, 0xea, 0x78, 0x5e,
};

parser_error_t _validateHash(const uint8_t scriptHash[METADATA_HASH_SIZE], const uint8_t *txMetadata, uint16_t txMetadataLength) {
    if (txMetadataLength < 1) {
        return PARSER_METADATA_ERROR;
    }
    uint8_t numberOfHashes = txMetadata[0];
    if (numberOfHashes > MAX_METADATA_NUMBER_OF_HASHES || txMetadataLength < 1+METADATA_HASH_SIZE*numberOfHashes) {
        return PARSER_METADATA_ERROR;
    }

    for(size_t i=0; i<numberOfHashes; i++) {
        uint8_t thisHashMatches = 1;
        for(int j=0; j<METADATA_HASH_SIZE; j++) {
            uint8_t hashByte = txMetadata[1 + i * METADATA_HASH_SIZE + j];
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

void initStoredTxMetadata() {
    explicit_bzero(&txMetadataState, sizeof(txMetadataState));
}

parser_error_t storeTxMetadata(const uint8_t *txMetadata, uint16_t txMetadataLength) {
    if (txMetadataLength > sizeof(txMetadataState.buffer)) {
        return PARSER_METADATA_ERROR;
    }

    memcpy(txMetadataState.buffer, txMetadata, txMetadataLength);
    txMetadataState.metadataLength = txMetadataLength;

    //Validate Merkle tree hash level
    if (txMetadataState.metadataMerkleTreeValidationLevel != 0) {
        return PARSER_METADATA_ERROR;
    }

    //calculate the 1st level hash
    sha256(txMetadataState.buffer, txMetadataState.metadataLength, txMetadataState.metadataMerkleTreeValidationHash);
    txMetadataState.metadataMerkleTreeValidationLevel = 1;
    
    return PARSER_OK;
}

parser_error_t validateStoredTxMetadataMerkleTreeLevel(const uint8_t* hashes, size_t hashesLen) {
    //Validate Merkle tree hash level
    if (txMetadataState.metadataMerkleTreeValidationLevel < 1 || txMetadataState.metadataMerkleTreeValidationLevel > METADATA_MERKLE_TREE_LEVELS) {
        return PARSER_METADATA_ERROR;
    }

    if (hashesLen % METADATA_HASH_SIZE != 0) {
        return PARSER_METADATA_ERROR;
    }

    //validate that current hash is in the list
    uint8_t currentHashFound = 0;
    for(size_t hashStart = 0; hashStart<hashesLen; hashStart+=METADATA_HASH_SIZE) {
        if (!memcmp(hashes+hashStart, txMetadataState.metadataMerkleTreeValidationHash, METADATA_HASH_SIZE)) {
            currentHashFound = 1;
        }
    }

    if (!currentHashFound) {
        return PARSER_METADATA_ERROR;
    }

    //calculate new hash
    sha256(hashes, hashesLen, txMetadataState.metadataMerkleTreeValidationHash);
    txMetadataState.metadataMerkleTreeValidationLevel += 1;
    return PARSER_OK;
}

static parser_error_t parseTxMetadataInternal(const uint8_t scriptHash[METADATA_HASH_SIZE], parsed_tx_metadata_t *parsedTxMetadata) {
    uint16_t parsed = 0;
   
    #define READ_CHAR(where) \
        { \
            if (!(parsed < txMetadataState.metadataLength)) { \
                return PARSER_METADATA_ERROR; \
            } \
            *(where) = txMetadataState.buffer[parsed++]; \
        } 
    #define READ_STRING(dest_pointer, len) { \
        *(len) = 0; \
        *(dest_pointer) = (char *) txMetadataState.buffer + parsed; \
        while (*(len) <= MAX_METADATA_STRING_LENGTH) { \
            uint8_t byte = 0; \
            READ_CHAR(&byte); \
            if (byte == 0) { \
                break; \
            } \
            (*(len))++; \
        } \
        if (*(len) > MAX_METADATA_STRING_LENGTH) { \
            return PARSER_METADATA_ERROR; \
        } \
    }
    #define READ_SKIP(count) { \
        parsed += (count); \
    }

    //read number of hashes and validate script
    {
        uint8_t numberOfHashes = 0;
        READ_CHAR(&numberOfHashes)
        if (numberOfHashes > MAX_METADATA_NUMBER_OF_HASHES) {
            return PARSER_METADATA_TOO_MANY_HASHES;
        }
        parser_error_t err = _validateHash(scriptHash, txMetadataState.buffer, txMetadataState.metadataLength);
        if (err != PARSER_OK) {
            return err;
        }
        READ_SKIP(numberOfHashes*METADATA_HASH_SIZE);
    }

    //read tx name
    READ_STRING(&parsedTxMetadata->txName, &parsedTxMetadata->txNameLength)

    //read arguments
    {
        READ_CHAR(&parsedTxMetadata->argCount)
        if (parsedTxMetadata->argCount > PARSER_MAX_ARGCOUNT) {
            return PARSER_METADATA_TOO_MANY_ARGUMENTS;
        }
        STATIC_ASSERT(sizeof(parsedTxMetadata->arguments) >= PARSER_MAX_ARGCOUNT, "Too few arguments in parsed_tx_metadata_t.");
        for(int i=0; i<parsedTxMetadata->argCount; i++) {
            READ_CHAR(&parsedTxMetadata->arguments[i].argumentType);
            argument_type_e argumentType = parsedTxMetadata->arguments[i].argumentType;
            if (argumentType == ARGUMENT_TYPE_ARRAY || argumentType == ARGUMENT_TYPE_OPTIONALARRAY) {
                READ_CHAR(&parsedTxMetadata->arguments[i].arrayMinElements);
                READ_CHAR(&parsedTxMetadata->arguments[i].arrayMaxElements);
                uint8_t min = parsedTxMetadata->arguments[i].arrayMinElements;
                uint8_t max = parsedTxMetadata->arguments[i].arrayMaxElements;
                if (min > max || max > MAX_METADATA_MAX_ARRAY_ITEMS) {
                    return PARSER_METADATA_ERROR;
                }
            }
            READ_STRING(&parsedTxMetadata->arguments[i].displayKey, &parsedTxMetadata->arguments[i].displayKeyLength)
            READ_CHAR(&parsedTxMetadata->arguments[i].argumentIndex);
            READ_STRING(&parsedTxMetadata->arguments[i].jsonExpectedType, &parsedTxMetadata->arguments[i].jsonExpectedTypeLength);
            READ_CHAR(&parsedTxMetadata->arguments[i].jsonExpectedKind);
        }
    }


    #undef READ_CHAR 
    #undef READ_STRING
    #undef READ_SKIP

    if (parsed != txMetadataState.metadataLength) {
        return PARSER_METADATA_ERROR;
    }

    return PARSER_OK;
}


parser_error_t parseTxMetadata(const uint8_t scriptHash[METADATA_HASH_SIZE], parsed_tx_metadata_t *parsedTxMetadata) {
    //validate that merkle tree metadata validation is finished
    if (txMetadataState.metadataMerkleTreeValidationLevel != METADATA_MERKLE_TREE_LEVELS + 1) {
        return PARSER_METADATA_ERROR;
    }
    if (memcmp(txMetadataState.metadataMerkleTreeValidationHash, merkleTreeRoot, METADATA_HASH_SIZE)) {
        return PARSER_METADATA_ERROR;
    }

    return parseTxMetadataInternal(scriptHash, parsedTxMetadata);
}

//For C++ testing purposes - we circumnavihate the hashing mechanism to test metadata parsing
parser_error_t _parseTxMetadata(const uint8_t scriptHash[METADATA_HASH_SIZE], const uint8_t *txMetadata, size_t txMetadataLength, 
                                parsed_tx_metadata_t *parsedTxMetadata) {
    memcpy(txMetadataState.buffer, txMetadata, txMetadataLength);
    txMetadataState.metadataLength = txMetadataLength;
    return parseTxMetadataInternal(scriptHash, parsedTxMetadata);
}
