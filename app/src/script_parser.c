#include "script_parser.h"


static bool isElementChar(uint8_t scriptChar) {
    return ('a'<=scriptChar && scriptChar<='z') || 
           ('A'<=scriptChar && scriptChar<='Z') || 
           ('0'<=scriptChar && scriptChar<='9') ||
           ('_'==scriptChar);
}

//returns parsed element length, 0 indicates error
static size_t parseElement(size_t index, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize) {
    size_t elementLength = 0;
    while (index + elementLength < scriptToParseSize) {
        uint8_t scriptChar = scriptToParse[index + elementLength];
        if (!isElementChar(scriptChar)) {
            return elementLength;
        }
        elementLength += 1;
    }
    return elementLength;
}


bool parseScript(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize, 
                 const uint8_t *scriptTemplate, size_t scriptTemplateSize) {
    parsedElements->elements_count = 0;
    size_t scriptRead = 0;
    size_t templateRead = 0;
    while (templateRead < scriptTemplateSize) {
        uint8_t templateChar = scriptTemplate[templateRead];

        if (templateChar == 1) { //token to parse
            size_t elementLen = parseElement(scriptRead, scriptToParse, scriptToParseSize);
            if (elementLen == 0 || parsedElements->elements_count == MAX_SCRIPT_PARSED_ELEMENTS) {
                return false;
            }
            parsedElements->elements[parsedElements->elements_count].data = scriptToParse + scriptRead;
            parsedElements->elements[parsedElements->elements_count].length = elementLen;

            parsedElements->elements_count += 1;
            scriptRead += elementLen;
            templateRead += 1;

            continue;
        }

        if (templateChar != scriptToParse[scriptRead]) {
            return false;
        }
        scriptRead += 1;
        templateRead += 1;
    }

    if (scriptRead != scriptToParseSize || templateRead != scriptTemplateSize) {
        return false;
    }

    parsedElements->script_type = SCRIPT_TYPE_UNKNOWN;
    return true;
}


#define ELEMENTS_MUST_BE_EQUAL(pE, i, j) {                                                          \
    if ((i >= pE->elements_count) || (j >= pE->elements_count)) return false;                       \
    if (pE->elements[i].length != pE->elements[j].length) return false;                             \
    if (MEMCMP(pE->elements[i].data, pE->elements[j].data, pE->elements[i].length)) return false;   \
}              

#define ADDRESS_STRING_LENGTH 18
#define NONFUNGIBLETOKEN_METADATAVIEWS_TESTNET "0x631e88ae7f1d7c20"
#define NONFUNGIBLETOKEN_METADATAVIEWS_MAINNET "0x1d7e57aa55817448"

#define ELEMENT_MUST_BE_ADDRESS(pE, i) {                                                                              \
    STATIC_ASSERT(sizeof(NONFUNGIBLETOKEN_METADATAVIEWS_TESTNET) == ADDRESS_STRING_LENGTH + 1, "Incompatible types"); \
    STATIC_ASSERT(sizeof(NONFUNGIBLETOKEN_METADATAVIEWS_MAINNET) == ADDRESS_STRING_LENGTH + 1, "Incompatible types"); \
    if (i >= pE->elements_count) return false;                                                                        \
    if (pE->elements[i].length != ADDRESS_STRING_LENGTH) return false;                                                \
    if (MEMCMP(pE->elements[i].data, NONFUNGIBLETOKEN_METADATAVIEWS_TESTNET, pE->elements[i].length) &&               \
        MEMCMP(pE->elements[i].data, NONFUNGIBLETOKEN_METADATAVIEWS_MAINNET, pE->elements[i].length)) return false;   \
}                                                                                   


// Elements :
// 0 - NonFungibleToken address
// 1 - MetadataViews address
// 2 - contractName
// 3 - contractAddress
// 4 - storagePath
// 5 - contractName
// 6 - storagePath
// 7 - publicCollectionContractName
// 8 - publicCollectionName
// 9 - publicPath
// 10 - storagePath
bool parseNFT1(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize) {
    const char template[] = 
        "import NonFungibleToken from \001\n"
        "import MetadataViews from \001\n"
        "import \001 from \001\n"
        "transaction {\n"
        "  prepare(acct: AuthAccount) {\n"
        "    let collectionType = acct.type(at: /storage/\001)\n"
        "    // if there already is a collection stored, return\n"
        "    if (collectionType != nil) {\n"
        "      return\n"
        "    }\n"
        "    // create empty collection\n"
        "    let collection <- \001.createEmptyCollection()\n"
        "    // put the new Collection in storage\n"
        "    acct.save(<-collection, to: /storage/\001)\n"
        "    // create a public capability for the collection\n"
        "    acct.link<&{NonFungibleToken.CollectionPublic, NonFungibleToken.Receiver, \001.\001, MetadataViews.ResolverCollection}>(\n"
        "      /public/\001,\n"
        "      target: /storage/\001\n"
        "    )\n"
        "  }\n"
        "}\n";

    if(!parseScript(parsedElements, scriptToParse, scriptToParseSize, (const uint8_t *) template, sizeof(template)-1)) { // -1 to strip terminating 0
        return false;
    }

    ELEMENT_MUST_BE_ADDRESS(parsedElements, 0); 
    ELEMENT_MUST_BE_ADDRESS(parsedElements, 1);

    ELEMENTS_MUST_BE_EQUAL(parsedElements, 2, 5);
    ELEMENTS_MUST_BE_EQUAL(parsedElements, 4, 6);    
    ELEMENTS_MUST_BE_EQUAL(parsedElements, 4, 10);

    parsedElements->script_type = SCRIPT_TYPE_NFT_SETUP_COLLECTION;
    return true;
}


// Elements :
// 0 - NonFungibleToken address
// 1 - contractName
// 2 - contractAddress
// 3 - storagePath
// 4 - contractName
// 5 - contractName
// 6 - storagePath
// 7 - publicPath
bool parseNFT2(script_parsed_elements_t *parsedElements, const uint8_t NV_VOLATILE *scriptToParse, size_t scriptToParseSize) {
    const char template[] = 
        "import NonFungibleToken from \001\n"
        "import \001 from \001\n"
        "transaction(recipient: Address, withdrawID: UInt64) {\n"
        "  // local variable for storing the transferred nft\n"
        "  let transferToken: @NonFungibleToken.NFT\n"
        "  prepare(owner: AuthAccount) {\n"
        "      // check if collection exists\n"
        "      if (owner.type(at: /storage/\001) != Type<@\001.Collection>()) {\n"
        "        panic(\"Could not borrow a reference to the stored collection\")\n"
        "      }\n"
        "      // borrow a reference to the collection\n"
        "      let collectionRef = owner\n"
        "        .borrow<&\001.Collection>(from: /storage/\001)!\n"
        "      // withdraw the NFT\n"
        "      self.transferToken <- collectionRef.withdraw(withdrawID: withdrawID)\n"
        "  }\n"
        "  execute {\n"
        "      // get the recipient's public account object\n"
        "      let recipient = getAccount(recipient)\n"
        "      // get receivers capability\n"
        "      let nonFungibleTokenCapability = recipient\n"
        "        .getCapability<&{NonFungibleToken.CollectionPublic}>(/public/\001)\n"
        "      // check the recipient has a NonFungibleToken public capability\n"
        "      if (!nonFungibleTokenCapability.check()) {\n"
        "        panic(\"Could not borrow a reference to the receiver's collection\")\n"
        "      }\n"
        "      // deposit nft to recipients collection\n"
        "      nonFungibleTokenCapability\n"
        "        .borrow()!\n"
        "        .deposit(token: <-self.transferToken)\n"
        "  }\n"
        "}\n";
    if(!parseScript(parsedElements, scriptToParse, scriptToParseSize, (const uint8_t *) template, sizeof(template)-1)) { // -1 to strip terminating 0
        return false;
    }

    ELEMENT_MUST_BE_ADDRESS(parsedElements, 0); 

    ELEMENTS_MUST_BE_EQUAL(parsedElements, 1, 4);
    ELEMENTS_MUST_BE_EQUAL(parsedElements, 1, 5);    
    ELEMENTS_MUST_BE_EQUAL(parsedElements, 3, 6);

    parsedElements->script_type = SCRIPT_TYPE_NFT_TRANSFER;
    return true;
}

#undef ELEMENTS_MUST_BE_EQUAL
#undef ELEMENT_MUST_BE_ADDRESS