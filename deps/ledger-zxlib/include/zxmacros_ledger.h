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

#if defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2) || defined(TARGET_STAX) || \
    defined(TARGET_FLEX) || defined(TARGET_APEX_P)

#include "cx.h"
#include "os.h"
#include "os_io_seproxyhal.h"
#include "ux.h"
#include "zxerror.h"

#define MEMCPY_NV nvm_write
#define MEMZERO_NV nvm_erase

// This macros are kept for backwards compatibility
// the most recent SDK has unified implementations and deprecated the original os_***
#define MEMCPY memmove
#define MEMMOVE memmove
#define MEMSET memset
#define MEMCMP memcmp
#define MEMZERO explicit_bzero

#define IS_UX_ALLOWED (G_ux_params.len != BOLOS_UX_IGNORE && G_ux_params.len != BOLOS_UX_CONTINUE)

#if defined(TARGET_NANOS)
#define NV_CONST
#define NV_VOLATILE
#else
#define NV_CONST const
#define NV_VOLATILE volatile
#endif

#define CHECK_APP_CANARY() check_app_canary();
#define APP_STACK_CANARY_MAGIC 0xDEAD0031
extern unsigned int app_stack_canary;

#define WAIT_EVENT() io_seproxyhal_spi_recv(G_io_seproxyhal_spi_buffer, sizeof(G_io_seproxyhal_spi_buffer), 0)

#if defined(TARGET_NANOS) || defined(TARGET_NANOX) || defined(TARGET_NANOS2)
#define UX_WAIT()                   \
    while (!UX_DISPLAYED()) {       \
        WAIT_EVENT();               \
        UX_DISPLAY_NEXT_ELEMENT();  \
    }                               \
    WAIT_EVENT();                   \
    io_seproxyhal_general_status(); \
    WAIT_EVENT()
#else
#define UX_WAIT() \
    {}
#endif

// Macros for handling no-throw methods error check
#define CHECK_CXERROR(CALL)       \
    do {                          \
        cx_err_t __cx_err = CALL; \
        if (__cx_err != CX_OK) {  \
            return __cx_err;      \
        }                         \
    } while (0)

#define CATCH_CXERROR(CALL)       \
    do {                          \
        cx_err_t __cx_err = CALL; \
        if (__cx_err != CX_OK) {  \
            goto catch_cx_error;  \
        }                         \
    } while (0)

#define CHECK_CX_OK(CALL)         \
    do {                          \
        cx_err_t __cx_err = CALL; \
        if (__cx_err != CX_OK) {  \
            return zxerr_unknown; \
        }                         \
    } while (0)

#endif
