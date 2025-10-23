/*******************************************************************************
 *   (c) 2018 - 2024 Zondax AG
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
#pragma once
#include <stdint.h>

#include "bolos_target.h"

#if defined(TARGET_NANOS)
#define INCLUDE_ACTIONS_AS_ITEMS 2
#define INCLUDE_ACTIONS_COUNT (INCLUDE_ACTIONS_AS_ITEMS - 1)
typedef uint8_t max_char_display;
#else
#define INCLUDE_ACTIONS_COUNT 0
typedef int max_char_display;
#endif

#define MAX_REVIEW_UX_SCREENS 10

void splitValueField();
void splitValueAddress();
max_char_display get_max_char_per_line();

void h_initialize();

bool h_paging_can_increase();

void h_paging_increase();

bool h_paging_can_decrease();

void h_paging_decrease();

bool h_paging_intro_screen();
