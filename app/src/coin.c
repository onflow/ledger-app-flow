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

#include "crypto.h"
// based on Dapper provided code at https://github.com/onflow/flow-go-sdk/blob/96796f0cabc1847d7879a5230ab55fd3cdd41ae8/address.go#L286

const uint16_t linearCodeN = 64;

const uint32_t parityCheckMatrixColumns[] = {
        0x00001, 0x00002, 0x00004, 0x00008,
        0x00010, 0x00020, 0x00040, 0x00080,
        0x00100, 0x00200, 0x00400, 0x00800,
        0x01000, 0x02000, 0x04000, 0x08000,
        0x10000, 0x20000, 0x40000, 0x7328d,
        0x6689a, 0x6112f, 0x6084b, 0x433fd,
        0x42aab, 0x41951, 0x233ce, 0x22a81,
        0x21948, 0x1ef60, 0x1deca, 0x1c639,
        0x1bdd8, 0x1a535, 0x194ac, 0x18c46,
        0x1632b, 0x1529b, 0x14a43, 0x13184,
        0x12942, 0x118c1, 0x0f812, 0x0e027,
        0x0d00e, 0x0c83c, 0x0b01d, 0x0a831,
        0x0982b, 0x07034, 0x0682a, 0x05819,
        0x03807, 0x007d2, 0x00727, 0x0068e,
        0x0067c, 0x0059d, 0x004eb, 0x003b4,
        0x0036a, 0x002d9, 0x001c7, 0x0003f,
};

bool validateChainAddress(uint64_t chainCodeWord, uint64_t address) {
    uint64_t codeWord = address ^chainCodeWord;

    if (codeWord == 0) {
        return false;
    }

    uint64_t parity = 0;
    for (uint16_t i = 0; i < linearCodeN; i++) {
        if ((codeWord & 1) == 1) {
            parity ^= parityCheckMatrixColumns[i];
        }
        codeWord >>= 1;
    }

    return parity == 0;
}

