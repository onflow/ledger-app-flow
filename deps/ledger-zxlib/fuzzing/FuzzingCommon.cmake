# Common fuzzing configuration for Zondax Ledger applications
# This file provides standardized fuzzing setup that can be included in any project

# Options
option(ENABLE_FUZZING "Build with fuzzing instrumentation and build fuzz targets" OFF)
option(ENABLE_COVERAGE "Build with source code coverage instrumentation" OFF)
option(ENABLE_SANITIZERS "Build with ASAN and UBSAN" OFF)

# Fuzzing configuration
if(ENABLE_FUZZING)
    message(STATUS "Fuzzing enabled")
    
    # Automatically enable sanitizers for fuzzing
    set(ENABLE_SANITIZERS ON CACHE BOOL "Sanitizers automatically enabled for fuzzing" FORCE)
    set(CMAKE_BUILD_TYPE Debug)
    
    # Define fuzzing build mode
    add_definitions(-DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1)
    
    # Optional fuzzing logging
    if(DEFINED ENV{FUZZ_LOGGING})
        add_definitions(-DFUZZING_LOGGING)
        message(STATUS "Fuzzing logging enabled")
    endif()
    
    # Set clang-tidy for fuzzing builds
    set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,bugprone-*,cert-*,clang-analyzer-*,-cert-err58-cpp,misc-*,-bugprone-suspicious-include)
    
    # Ensure Clang is being used
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
            message(FATAL_ERROR "Clang version must be at least 10.0!")
        endif()
    else()
        message(FATAL_ERROR
                "Unsupported compiler for fuzzing! Fuzzing requires Clang 10+.\n"
                "1. Install clang-10 or newer\n"
                "2. Use -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++")
    endif()
    
    # Add fuzzing instrumentation
    string(APPEND CMAKE_C_FLAGS " -fsanitize=fuzzer-no-link")
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=fuzzer-no-link")
    string(APPEND CMAKE_LINKER_FLAGS " -fsanitize=fuzzer-no-link")
endif()

# Coverage configuration
if(ENABLE_COVERAGE)
    message(STATUS "Coverage enabled")
    
    # Create coverage directory in project folder
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/coverage")
    
    # Add coverage instrumentation
    string(APPEND CMAKE_C_FLAGS " -fprofile-instr-generate=${CMAKE_CURRENT_SOURCE_DIR}/coverage/%p.profraw -fcoverage-mapping")
    string(APPEND CMAKE_CXX_FLAGS " -fprofile-instr-generate=${CMAKE_CURRENT_SOURCE_DIR}/coverage/%p.profraw -fcoverage-mapping")
    string(APPEND CMAKE_LINKER_FLAGS " -fprofile-instr-generate=${CMAKE_CURRENT_SOURCE_DIR}/coverage/%p.profraw -fcoverage-mapping")
endif()

# Sanitizers configuration
if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled")
    
    # Address and undefined behavior sanitizers
    string(APPEND CMAKE_C_FLAGS " -fsanitize=address,undefined")
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=address,undefined")
    string(APPEND CMAKE_LINKER_FLAGS " -fsanitize=address,undefined")
    
    # Allow recovery for continued fuzzing
    string(APPEND CMAKE_C_FLAGS " -fsanitize-recover=address,undefined")
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize-recover=address,undefined")
    string(APPEND CMAKE_LINKER_FLAGS " -fsanitize-recover=address,undefined")
    
    # Use-after-scope detection
    string(APPEND CMAKE_C_FLAGS " -fsanitize-address-use-after-scope")
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize-address-use-after-scope")
    string(APPEND CMAKE_LINKER_FLAGS " -fsanitize-address-use-after-scope")
    
    # Integer and bounds checking
    string(APPEND CMAKE_C_FLAGS " -fsanitize=integer -fsanitize=bounds")
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=integer -fsanitize=bounds")
    string(APPEND CMAKE_LINKER_FLAGS " -fsanitize=integer -fsanitize=bounds")
    
    # Stack protection
    string(APPEND CMAKE_C_FLAGS " -fstack-protector-strong")
    string(APPEND CMAKE_CXX_FLAGS " -fstack-protector-strong")
endif()

# Common debugging flags for all builds
string(APPEND CMAKE_C_FLAGS " -fno-omit-frame-pointer -g")
string(APPEND CMAKE_CXX_FLAGS " -fno-omit-frame-pointer -g")
string(APPEND CMAKE_LINKER_FLAGS " -fno-omit-frame-pointer -g")

# Macro to easily create fuzz targets
macro(add_fuzz_target TARGET_NAME SOURCE_FILE)
    if(ENABLE_FUZZING)
        # Create the fuzz target
        add_executable(fuzz-${TARGET_NAME} ${SOURCE_FILE})
        
        # Link with the main application library
        target_link_libraries(fuzz-${TARGET_NAME} PRIVATE ${ARGN})
        
        # Add fuzzer linking
        target_link_options(fuzz-${TARGET_NAME} PRIVATE "-fsanitize=fuzzer")
        
        # Set target properties
        set_target_properties(fuzz-${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
        
        message(STATUS "Created fuzz target: fuzz-${TARGET_NAME}")
    endif()
endmacro()

# Macro to setup project-specific fuzz directories
macro(setup_fuzz_directories)
    if(ENABLE_FUZZING)
        # Create standard fuzzing directories
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/corpora")
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/logs")
        
        message(STATUS "Fuzzing directories setup completed")
    endif()
endmacro()