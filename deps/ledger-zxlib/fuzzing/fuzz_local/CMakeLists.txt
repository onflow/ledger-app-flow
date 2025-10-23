cmake_minimum_required(VERSION 3.16)
project(zxlib_fuzz)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress duplicate library warnings on macOS
if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_warn_duplicate_libraries")
endif()

# Include the common fuzzing configuration
include(../FuzzingCommon.cmake)

# Setup fuzzing directories manually to avoid path issues
if(ENABLE_FUZZING)
    # Create directories in the correct location (current source dir is now /fuzzing/fuzz_local)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/corpora")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/logs")
    message(STATUS "Fuzzing directories setup completed for ledger-zxlib")
endif()

# Find required source files
set(ZXLIB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")
set(ZXLIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include")

# Find all source files needed for bech32
file(GLOB BECH32_SOURCES
    "${ZXLIB_SRC_DIR}/bech32.c"
    "${ZXLIB_SRC_DIR}/bittools.c"
    "${ZXLIB_SRC_DIR}/segwit_addr.c"
)

# Create a library with the source files
add_library(zxlib_bech32 STATIC ${BECH32_SOURCES})

target_include_directories(zxlib_bech32 PUBLIC 
    ${ZXLIB_INCLUDE_DIR}
    ${ZXLIB_SRC_DIR}
)

# Add fuzz target for bech32
add_fuzz_target(bech32 bech32_fuzzer.cpp zxlib_bech32)

# Find source files for base58
file(GLOB BASE58_SOURCES
    "${ZXLIB_SRC_DIR}/base58.c"
)

# Create library for base58
add_library(zxlib_base58 STATIC ${BASE58_SOURCES})
target_include_directories(zxlib_base58 PUBLIC 
    ${ZXLIB_INCLUDE_DIR}
    ${ZXLIB_SRC_DIR}
)

# Add fuzz target for base58
add_fuzz_target(base58 base58_fuzzer.cpp zxlib_base58)

# Find source files for base64
file(GLOB BASE64_SOURCES
    "${ZXLIB_SRC_DIR}/base64.c"
)

# Create library for base64
add_library(zxlib_base64 STATIC ${BASE64_SOURCES})
target_include_directories(zxlib_base64 PUBLIC 
    ${ZXLIB_INCLUDE_DIR}
    ${ZXLIB_SRC_DIR}
)

# Add fuzz target for base64
add_fuzz_target(base64 base64_fuzzer.cpp zxlib_base64)

# Find source files for hexutils
file(GLOB HEXUTILS_SOURCES
    "${ZXLIB_SRC_DIR}/hexutils.c"
)

# Create library for hexutils
add_library(zxlib_hexutils STATIC ${HEXUTILS_SOURCES})
target_include_directories(zxlib_hexutils PUBLIC 
    ${ZXLIB_INCLUDE_DIR}
    ${ZXLIB_SRC_DIR}
)

# Add fuzz target for hexutils
add_fuzz_target(hexutils hexutils_fuzzer.cpp zxlib_hexutils)

# Create library for segwit_addr (reuse existing bech32 sources)
add_fuzz_target(segwit_addr segwit_addr_fuzzer.cpp zxlib_bech32)

# Find source files for bignum
file(GLOB BIGNUM_SOURCES
    "${ZXLIB_SRC_DIR}/bignum.c"
)

# Create library for bignum
add_library(zxlib_bignum STATIC ${BIGNUM_SOURCES})
target_include_directories(zxlib_bignum PUBLIC 
    ${ZXLIB_INCLUDE_DIR}
    ${ZXLIB_SRC_DIR}
)

# Add fuzz target for bignum
add_fuzz_target(bignum bignum_fuzzer.cpp zxlib_bignum)

# Find source files for zxformat
file(GLOB ZXFORMAT_SOURCES
    "${ZXLIB_SRC_DIR}/zxformat.c"
)

# Create library for zxformat
add_library(zxlib_zxformat STATIC ${ZXFORMAT_SOURCES})
target_include_directories(zxlib_zxformat PUBLIC 
    ${ZXLIB_INCLUDE_DIR}
    ${ZXLIB_SRC_DIR}
)

# Add fuzz target for zxformat
add_fuzz_target(zxformat zxformat_fuzzer.cpp zxlib_zxformat)

# Find source files for timeutils
file(GLOB TIMEUTILS_SOURCES
    "${ZXLIB_SRC_DIR}/timeutils.c"
)

# Create library for timeutils
add_library(zxlib_timeutils STATIC ${TIMEUTILS_SOURCES})
target_include_directories(zxlib_timeutils PUBLIC 
    ${ZXLIB_INCLUDE_DIR}
    ${ZXLIB_SRC_DIR}
)

# Add fuzz target for timeutils
add_fuzz_target(timeutils timeutils_fuzzer.cpp zxlib_timeutils)