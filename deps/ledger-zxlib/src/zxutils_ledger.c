// #*******************************************************************************
// #*   (c) 2018 - 2024 Zondax AG
// #*   (c) 2020 Ledger SAS
// #*
// #*  Licensed under the Apache License, Version 2.0 (the "License");
// #*  you may not use this file except in compliance with the License.
// #*  You may obtain a copy of the License at
// #*
// #*      http://www.apache.org/licenses/LICENSE-2.0
// #*
// #*  Unless required by applicable law or agreed to in writing, software
// #*  distributed under the License is distributed on an "AS IS" BASIS,
// #*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// #*  See the License for the specific language governing permissions and
// #*  limitations under the License.
// #********************************************************************************
#include "zxutils_ledger.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

// We implement a light mechanism in order to be able to retrieve the width of
// nano S characters, in the two possible fonts:
// - BAGL_FONT_OPEN_SANS_EXTRABOLD_11px,
// - BAGL_FONT_OPEN_SANS_REGULAR_11px.
#define NANOS_FIRST_CHAR 0x20
#define NANOS_LAST_CHAR 0x7F

// OPEN_SANS_REGULAR_11PX << 4 | OPEN_SANS_EXTRABOLD_11PX
extern const char nanos_characters_width[96];

unsigned short zx_compute_line_width_light(const char *text, unsigned char text_length) {
    char current_char;
    unsigned short line_width = 0;

    if (text == NULL) {
        return 0xFFFF;
    }

    // We parse the characters of the input text on all the input length.
    while (text_length--) {
        current_char = *text;

        if (current_char < NANOS_FIRST_CHAR || current_char > NANOS_LAST_CHAR) {
            if (current_char == '\n' || current_char == '\r') {
                break;
            }
        } else {
            // Regular.
            line_width += (nanos_characters_width[current_char - NANOS_FIRST_CHAR] >> 0x04) & 0x0F;
        }
        text++;
    }
    return line_width;
}
