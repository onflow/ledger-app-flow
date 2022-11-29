/*******************************************************************************
*   (c) 2022 Vacuumlabs
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "gmock/gmock.h"
#include <iostream>
#include <stdlib.h>
#include <hexutils.h>
#include <json/json_parser.h>
#include <script_parser.h>
#include <string.h>
#include <string>
#include <vector>

testing::AssertionResult PARSE_TEST(std::string script, std::string template_, bool expectedResult, 
                                    std::vector<std::string> expectedValues = {}) {
    script_parsed_elements_t parsed;
    bool result = parseScript(&parsed, (const uint8_t *) script.data(), script.size(),
                            (const uint8_t *) template_.data(), template_.size());
    if (result != expectedResult) {
        return testing::AssertionFailure() << "Result: " << result << ", Expected: " << expectedResult;
    }
    if (result) {
        if (parsed.elements_count != expectedValues.size()) {
            return testing::AssertionFailure() << "Parsing error, Size: " << parsed.elements_count << ", Expected: " << expectedValues.size();
        }

        for(size_t i=0; i<expectedValues.size(); i++) {
            std::string parsedString((char *)parsed.elements[i].data, parsed.elements[i].length);
            if (parsedString != expectedValues[i]) {
                return testing::AssertionFailure() << "Parsing error; Index:" << i << ", Result: " << parsedString << ", Expected: " << expectedValues[i];
            }
        }
    }
    return testing::AssertionSuccess();
}

template<typename NFTFunction>
testing::AssertionResult PARSE_NFT_TEST(NFTFunction parseNFTFunction, std::string script, bool expectedResult, 
                                        std::vector<std::string> expectedValues = {},
                                        script_parsed_type_t expectedScriptType = SCRIPT_TYPE_UNKNOWN) {
    script_parsed_elements_t parsed;
    bool result = parseNFTFunction(&parsed, (const uint8_t *) script.data(), script.size());
    if (result != expectedResult) {
        return testing::AssertionFailure() << "Result: " << result << ", Expected: " << expectedResult;
    }
    if (result) {
        if (parsed.script_type != expectedScriptType) {
            return testing::AssertionFailure() << "Parsing error, Script type: " << parsed.script_type << ", Expected: " << expectedScriptType;
        }

        if (parsed.elements_count != expectedValues.size()) {
            return testing::AssertionFailure() << "Parsing error, Size: " << parsed.elements_count << ", Expected: " << expectedValues.size();
        }

        for(size_t i=0; i<expectedValues.size(); i++) {
            std::string parsedString((char *)parsed.elements[i].data, parsed.elements[i].length);
            if (parsedString != expectedValues[i]) {
                return testing::AssertionFailure() << "Parsing error; Index:" << i << ", Result: " << parsedString << ", Expected: " << expectedValues[i];
            }
        }
    }
    return testing::AssertionSuccess();
}

//templates 3-5 should always fail
const char * TEMPLATE1 = "abb\001 aa \001";
const char * TEMPLATE2 = "abb\001 aa\001 a";
const char * TEMPLATE3 = "\001 \001 \001 \001 \001 \001 \001 \001 \001 \001 \001 \001";
const char * TEMPLATE4 = "\001\001";
const char * TEMPLATE5 = "\001a";
const char * TEMPLATE6 = "\001 a";
const char * TEMPLATE7 = "a\001^\001";

TEST(script_parse, parseScript) {
    EXPECT_TRUE(PARSE_TEST("abbxxy aa yy", TEMPLATE1, true, {"xxy", "yy"}));
    EXPECT_TRUE(PARSE_TEST("abbxxy aa y ", TEMPLATE1, false));
    EXPECT_TRUE(PARSE_TEST("abbxxy aayy", TEMPLATE1, false));
    EXPECT_TRUE(PARSE_TEST("abbxxy aa y;", TEMPLATE1, false));
    EXPECT_TRUE(PARSE_TEST("abbxxy aa y?", TEMPLATE1, false));
    EXPECT_TRUE(PARSE_TEST("abbxxy aa y a", TEMPLATE2, false));
    EXPECT_TRUE(PARSE_TEST("abbxxy aay a", TEMPLATE2, true, {"xxy", "y"}));
    EXPECT_TRUE(PARSE_TEST("a a a a a a a a a a a a", TEMPLATE3, false));
    EXPECT_TRUE(PARSE_TEST("aa", TEMPLATE4, false));
    EXPECT_TRUE(PARSE_TEST("a a", TEMPLATE4, false));
    EXPECT_TRUE(PARSE_TEST("ba", TEMPLATE5, false));
    EXPECT_TRUE(PARSE_TEST("Abb_xy a", TEMPLATE6, true, {"Abb_xy"}));
    EXPECT_TRUE(PARSE_TEST("aa^__", TEMPLATE7, true, {"a", "__"}));
}

TEST(script_parse, parseNFT1) {
    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT1, "", false));

    const char script1[] = 
        "import NonFungibleToken from 0x631e88ae7f1d7c20\n"
        "import MetadataViews from 0x631e88ae7f1d7c20\n"
        "import aaa from bbb\n"
        "transaction {\n"
        "  prepare(acct: AuthAccount) {\n"
        "    let collectionType = acct.type(at: /storage/c)\n"
        "    // if there already is a collection stored, return\n"
        "    if (collectionType != nil) {\n"
        "      return\n"
        "    }\n"
        "    // create empty collection\n"
        "    let collection <- aaa.createEmptyCollection()\n"
        "    // put the new Collection in storage\n"
        "    acct.save(<-collection, to: /storage/c)\n"
        "    // create a public capability for the collection\n"
        "    acct.link<&{NonFungibleToken.CollectionPublic, NonFungibleToken.Receiver, x._, MetadataViews.ResolverCollection}>(\n"
        "      /public/zzzzzzZ,\n"
        "      target: /storage/c\n"
        "    )\n"
        "  }\n"
        "}\n";

    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT1, script1, true, {"0x631e88ae7f1d7c20", "0x631e88ae7f1d7c20",
                               "aaa", "bbb", "c", "aaa", "c", "x", "_", "zzzzzzZ", "c"}, 
                               SCRIPT_TYPE_NFT_SETUP_COLLECTION));

    const char script2[] = 
        "import NonFungibleToken from 0x631e88ae7f1d7c20\n"
        "import MetadataViews from 0x631e88ae7f1d7c20\n"
        "import aaa from bbb\n"
        "transaction {\n"
        "  prepare(acct: AuthAccount) {\n"
        "    let collectionType = acct.type(at: /storage/c)\n"
        "    // if there already is a collection stored, return\n"
        "    if (collectionType != nil) {\n"
        "      return\n"
        "    }\n"
        "    // create empty collection\n"
        "    let collection <- aaa.createEmptyCollection()\n"
        "    // put the new Collection in storage\n"
        "    acct.save(<-collection, to: /storage/cc)\n"
        "    // create a public capability for the collection\n"
        "    acct.link<&{NonFungibleToken.CollectionPublic, NonFungibleToken.Receiver, x._, MetadataViews.ResolverCollection}>(\n"
        "      /public/zzzzzzZ,\n"
        "      target: /storage/c\n"
        "    )\n"
        "  }\n"
        "}\n";

    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT1, script2, false)); //storages do not match

    const char script3[] = 
        "import NonFungibleToken from 0x631e88ae7f1d7c20\n"
        "import MetadataViews from 0x631e88ae7f1d7c20\n"
        "import aaaa from bbb\n"
        "transaction {\n"
        "  prepare(acct: AuthAccount) {\n"
        "    let collectionType = acct.type(at: /storage/c)\n"
        "    // if there already is a collection stored, return\n"
        "    if (collectionType != nil) {\n"
        "      return\n"
        "    }\n"
        "    // create empty collection\n"
        "    let collection <- aaa.createEmptyCollection()\n"
        "    // put the new Collection in storage\n"
        "    acct.save(<-collection, to: /storage/c)\n"
        "    // create a public capability for the collection\n"
        "    acct.link<&{NonFungibleToken.CollectionPublic, NonFungibleToken.Receiver, x._, MetadataViews.ResolverCollection}>(\n"
        "      /public/zzzzzzZ,\n"
        "      target: /storage/c\n"
        "    )\n"
        "  }\n"
        "}\n";

    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT1, script3, false)); //contract names do not match
}

TEST(script_parse, parseNFT2) {
    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT2, "", false));

    const char script1[] = 
        "import NonFungibleToken from 0x1d7e57aa55817448\n"
        "import aaaa from bbb\n"
        "transaction(recipient: Address, withdrawID: UInt64) {\n"
        "  // local variable for storing the transferred nft\n"
        "  let transferToken: @NonFungibleToken.NFT\n"
        "  prepare(owner: AuthAccount) {\n"
        "      // check if collection exists\n"
        "      if (owner.type(at: /storage/ststst) != Type<@aaaa.Collection>()) {\n"
        "        panic(\"Could not borrow a reference to the stored collection\")\n"
        "      }\n"
        "      // borrow a reference to the collection\n"
        "      let collectionRef = owner\n"
        "        .borrow<&aaaa.Collection>(from: /storage/ststst)!\n"
        "      // withdraw the NFT\n"
        "      self.transferToken <- collectionRef.withdraw(withdrawID: withdrawID)\n"
        "  }\n"
        "  execute {\n"
        "      // get the recipient's public account object\n"
        "      let recipient = getAccount(recipient)\n"
        "      // get receivers capability\n"
        "      let nonFungibleTokenCapability = recipient\n"
        "        .getCapability<&{NonFungibleToken.CollectionPublic}>(/public/publicPath)\n"
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

    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT2, script1, true, {"0x1d7e57aa55817448", "aaaa", "bbb", "ststst", "aaaa", "aaaa", "ststst", "publicPath"},
                               SCRIPT_TYPE_NFT_TRANSFER));

    const char script2[] = 
        "import NonFungibleToken from 0x1d7e57aa55817448\n"
        "import aaaa from bbb\n"
        "transaction(recipient: Address, withdrawID: UInt64) {\n"
        "  // local variable for storing the transferred nft\n"
        "  let transferToken: @NonFungibleToken.NFT\n"
        "  prepare(owner: AuthAccount) {\n"
        "      // check if collection exists\n"
        "      if (owner.type(at: /storage/ststst) != Type<@aaaa.Collection>()) {\n"
        "        panic(\"Could not borrow a reference to the stored collection\")\n"
        "      }\n"
        "      // borrow a reference to the collection\n"
        "      let collectionRef = owner\n"
        "        .borrow<&aaaaa.Collection>(from: /storage/ststst)!\n"
        "      // withdraw the NFT\n"
        "      self.transferToken <- collectionRef.withdraw(withdrawID: withdrawID)\n"
        "  }\n"
        "  execute {\n"
        "      // get the recipient's public account object\n"
        "      let recipient = getAccount(recipient)\n"
        "      // get receivers capability\n"
        "      let nonFungibleTokenCapability = recipient\n"
        "        .getCapability<&{NonFungibleToken.CollectionPublic}>(/public/publicPath)\n"
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

    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT2, script2, false)); //contractName mismatch

    const char script3[] = 
        "import NonFungibleToken from 0x1d7e57aa55817448\n"
        "import aaaa from bbb\n"
        "transaction(recipient: Address, withdrawID: UInt64) {\n"
        "  // local variable for storing the transferred nft\n"
        "  let transferToken: @NonFungibleToken.NFT\n"
        "  prepare(owner: AuthAccount) {\n"
        "      // check if collection exists\n"
        "      if (owner.type(at: /storage/stst) != Type<@aaaa.Collection>()) {\n"
        "        panic(\"Could not borrow a reference to the stored collection\")\n"
        "      }\n"
        "      // borrow a reference to the collection\n"
        "      let collectionRef = owner\n"
        "        .borrow<&aaaa.Collection>(from: /storage/ststst)!\n"
        "      // withdraw the NFT\n"
        "      self.transferToken <- collectionRef.withdraw(withdrawID: withdrawID)\n"
        "  }\n"
        "  execute {\n"
        "      // get the recipient's public account object\n"
        "      let recipient = getAccount(recipient)\n"
        "      // get receivers capability\n"
        "      let nonFungibleTokenCapability = recipient\n"
        "        .getCapability<&{NonFungibleToken.CollectionPublic}>(/public/publicPath)\n"
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

    EXPECT_TRUE(PARSE_NFT_TEST(parseNFT2, script3, false)); // storage mismatch
}