/*******************************************************************************
*   (c) 2019 Zondax GmbH
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
#include <fmt/core.h>
#include <coin.h>
#include <crypto.h>
#include <parser_txdef.h>
#include <parser_impl.h>
#include <iomanip>
#include "testcases.h"
#include "zxmacros.h"
#include "zxformat.h"
#include <iostream>

bool TestcaseIsValid(const Json::Value &) {
    return true;
}

std::string formatString(const std::string &data, uint8_t idx, uint8_t *pageCount) {
    char outBuffer[40];
    pageString(outBuffer, sizeof(outBuffer), data.c_str(), idx, pageCount);
    return std::string(outBuffer);
}

std::vector<std::string> formatStringParts(const Json::Value &v) {
    std::vector<std::string> answer;
    std::stringstream s;
    s << v.asString();

    uint8_t pageIdx = 0;
    uint8_t pageCount = 1;
    char outBuffer[40];

    while (pageIdx < pageCount) {
        pageString(outBuffer, sizeof(outBuffer), s.str().c_str(), pageIdx, &pageCount);
        answer.emplace_back(outBuffer);
        pageIdx++;
    }

    return answer;
}

template<typename S, typename... Args>
void addTo(std::vector<std::string> &answer, const S &format_str, Args &&... args) {
    answer.push_back(fmt::format(format_str, args...));
}

void addMultiStringArgumentTo(std::vector<std::string> &answer, const std::string &name, uint16_t item, const Json::Value &v) {
    auto chunks = formatStringParts(v);

    if (chunks.size() == 1) {
        addTo(answer, "{} | {} : {}", item, name, chunks[0]);
        return;
    }

    for (uint16_t j = 0; j < (uint16_t) chunks.size(); j++) {
        addTo(answer, "{} | {} [{}/{}] : {}", item, name, j + 1, chunks.size(), chunks[j]);
    }
}

std::vector<std::string> GenerateExpectedUIOutput(const testcaseData_t &tcd) {
    auto answer = std::vector<std::string>();

    if (!tcd.valid) {
        answer.emplace_back("Test case is not valid!");
        return answer;
    }

    uint8_t scriptHash[32];
    sha256((const uint8_t *) tcd.script.c_str(), tcd.script.length(), scriptHash);

    uint16_t item = 0;
    uint8_t dummy;

    parsed_tx_metadata_t m;
    _parseTxMetadata(scriptHash, tcd.metadata.data(), tcd.metadata.size(), &m);

    addTo(answer, "{} | Type : {}", item++, m.txName);
    addTo(answer, "{} | ChainID : {}", item++, tcd.chainID);
    for(int i=0; i<m.argCount; i++) {
        parsed_tx_metadata_argument_t *ma = &m.arguments[i];
        int argNo = ma->argumentIndex;

        switch(ma->argumentType) {
            case ARGUMENT_TYPE_NORMAL:
                addMultiStringArgumentTo(answer, ma->displayKey, item++, tcd.arguments[argNo]["value"]);
                break;
            case ARGUMENT_TYPE_OPTIONAL:
                if (tcd.arguments[1]["value"].isObject()) {
                    addMultiStringArgumentTo(answer, ma->displayKey, item++, tcd.arguments[argNo]["value"]["value"]);
                }
                else {
                    addTo(answer, "{} | {} : None", item++, ma->displayKey);
                }
                break;
            case ARGUMENT_TYPE_ARRAY:
                for (uint16_t i = 0; i < (uint16_t) tcd.arguments[argNo]["value"].size(); i++) {
                    addMultiStringArgumentTo(answer, fmt::format("{} {}", ma->displayKey, i + 1), item++, tcd.arguments[argNo]["value"][i]["value"]);
                }                
                break;
            case ARGUMENT_TYPE_OPTIONALARRAY:
                if (tcd.arguments[i]["value"].isObject()) {
                    for (uint16_t i = 0; i < (uint16_t) tcd.arguments[argNo]["value"]["value"].size(); i++) {
                        addMultiStringArgumentTo(answer, fmt::format("{} {}", ma->displayKey, i + 1), item++, tcd.arguments[argNo]["value"]["value"][i]["value"]);
                    }
                }
                else {
                    addTo(answer, "{} | {} 1 : None", item++, ma->displayKey);
                }
                break;
        }
    }

    addTo(answer, "{} | Ref Block [1/2] : {}", item, formatString(tcd.refBlock, 0, &dummy));
    addTo(answer, "{} | Ref Block [2/2] : {}", item++, formatString(tcd.refBlock, 1, &dummy));
    addTo(answer, "{} | Gas Limit : {}", item++, tcd.gasLimit);
    addTo(answer, "{} | Prop Key Addr : {}", item++, formatString(tcd.proposalKeyAddress, 0, &dummy));
    addTo(answer, "{} | Prop Key Id : {}", item++, tcd.proposalKeyId);
    addTo(answer, "{} | Prop Key Seq Num : {}", item++, tcd.proposalKeySequenceNumber);
    addTo(answer, "{} | Payer : {}", item++, formatString(tcd.payer, 0, &dummy));

    for (uint16_t i = 0; i < (uint16_t) tcd.authorizers.size(); i++) {
        addTo(answer, "{} | Authorizer {} : {}", item++, i + 1, tcd.authorizers[i]);
    }

    addTo(answer, "{} | Warning: : No address stored on the device.", item++);

    return answer;
}
