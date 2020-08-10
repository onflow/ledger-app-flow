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
#include <parser_common.h>
#include <parser_txdef.h>
#include <parser_impl.h>
#include "testcases.h"
#include "zxmacros.h"

bool TestcaseIsValid(const Json::Value &tc) {
    return true;
}

template<typename S, typename... Args>
void addTo(std::vector<std::string> &answer, const S &format_str, Args &&... args) {
    answer.push_back(fmt::format(format_str, args...));
}

std::string FormatHexString(const std::string &data, uint8_t idx, uint8_t *pageCount) {
    char outBuffer[40];
    pageString(outBuffer, sizeof(outBuffer), data.c_str(), idx, pageCount);
    return std::string(outBuffer);
}

std::vector<std::string> GenerateExpectedUIOutput(const testcaseData_t &tcd) {
    auto answer = std::vector<std::string>();

    if (!tcd.valid) {
        answer.emplace_back("Test case is not valid!");
        return answer;
    }

    uint8_t scriptHash[32];
    script_type_e scriptType = script_unknown;
    sha256((const uint8_t *) tcd.script.c_str(), tcd.script.length(), scriptHash);
    _matchScriptType(scriptHash, &scriptType);

    switch (scriptType) {
        case script_unknown:
            addTo(answer, "0 | Type :Unknown");
            break;
        case script_token_transfer:
            addTo(answer, "0 | Type : Token Transfer");
            break;
        case script_create_account:
            addTo(answer, "0 | Type : Create Account");
            break;
        default:
            addTo(answer, "0 | Type : ERROR");
            break;
    }

    uint8_t dummy;

    // TODO: Complete
    addTo(answer, "1 | Param : ?");
    addTo(answer, "2 | Ref Block Id : {}", FormatHexString(tcd.refBlock, 0, &dummy));
    addTo(answer, "2 | Ref Block Id : {}", FormatHexString(tcd.refBlock, 1, &dummy));
    addTo(answer, "3 | Gas Limit : {}", tcd.gasLimit);
    addTo(answer, "4 | Prop Key Addr : {}", FormatHexString(tcd.proposalKeyAddress, 0, &dummy));
    addTo(answer, "5 | Prop Key Id : {}", tcd.proposalKeyId);
    addTo(answer, "6 | Prop Key Seq Num : {}", tcd.proposalKeySequenceNumber);
    addTo(answer, "7 | Payer : {}", FormatHexString(tcd.payer, 0, &dummy));

    if (tcd.authorizers.size() > 1) {
        addTo(answer, "8 | Authorizer : ERR");      // TODO: is this valid?
        // FIXME: Missing test cases
//        uint16_t count = 0;
//        for (const auto& a: tcd.authorizers) {
//            addTo(answer, "8 | Authorizer {}/{} : {}", count + 1, tcd.authorizers.size(), a);
//            count++;
//        }
    } else {
        for (const auto& a: tcd.authorizers) {
            addTo(answer, "8 | Authorizer 1 : {}", a);
        }
    }

    return answer;
}
