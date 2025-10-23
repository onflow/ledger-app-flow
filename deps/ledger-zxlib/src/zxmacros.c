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
#include "zxmacros.h"

#include "zxcanary.h"

#ifdef __cplusplus
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#endif

void handle_stack_overflow() {
    zemu_log("!!!!!!!!!!!!!!!!!!!!!! CANARY TRIGGERED!!! STACK OVERFLOW DETECTED\n");
#if defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2) || defined(TARGET_STAX) || \
    defined(TARGET_FLEX) || defined(TARGET_APEX_P)
    io_seproxyhal_se_reset();
#else
    while (1);
#endif
}

#ifdef __cplusplus
#pragma clang diagnostic pop
#endif

__Z_UNUSED void check_app_canary() {
#if defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2) || defined(TARGET_STAX) || \
    defined(TARGET_FLEX) || defined(TARGET_APEX_P)
    if (app_stack_canary != APP_STACK_CANARY_MAGIC) handle_stack_overflow();
    check_zondax_canary();
#endif
}

#if defined(ZEMU_LOGGING) && (defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2) || \
                              defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX_P))
void zemu_log_stack(const char *ctx) {
#define STACK_SHIFT 20
    void *p = NULL;
    char buf[70];
#if defined(HAVE_ZONDAX_CANARY)
    // When Zondax canary is enabled, we add a random canary just above `APP_STACK_CANARY_MAGIC 0xDEAD0031`
    const uint32_t availableStack =
        ((uint32_t)((void *)&p) + STACK_SHIFT - (uint32_t)&app_stack_canary) - sizeof(uint32_t);
#else
    const uint32_t availableStack = (uint32_t)((void *)&p) + STACK_SHIFT - (uint32_t)&app_stack_canary;
#endif

    snprintf(buf, sizeof(buf), "|SP| %p %p (%d) : %s\n", &app_stack_canary, ((void *)&p) + STACK_SHIFT, availableStack,
             ctx);
    zemu_log(buf);
    (void)ctx;
}
#else

void zemu_log_stack(__Z_UNUSED const char *ctx) {}

#endif

#if defined(ZEMU_LOGGING) && (defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2) || \
                              defined(TARGET_STAX) || defined(TARGET_FLEX) || defined(TARGET_APEX_P))
void zemu_trace(const char *file, uint32_t line) {
    char buf[200];
    snprintf(buf, sizeof(buf), "|TRACE| %s:%d\n", file, line);
    zemu_log(buf);
}
#else

void zemu_trace(__Z_UNUSED const char *file, __Z_UNUSED uint32_t line) {}

#endif
