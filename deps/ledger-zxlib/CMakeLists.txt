#*******************************************************************************
#*   (c) 2018 Zondax AG
#*
#*  Licensed under the Apache License, Version 2.0 (the "License");
#*  you may not use this file except in compliance with the License.
#*  You may obtain a copy of the License at
#*
#*      http://www.apache.org/licenses/LICENSE-2.0
#*
#*  Unless required by applicable law or agreed to in writing, software
#*  distributed under the License is distributed on an "AS IS" BASIS,
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#*  See the License for the specific language governing permissions and
#*  limitations under the License.
#********************************************************************************
cmake_minimum_required(VERSION 3.18)
include("cmake/HunterGate.cmake")
HunterGate(
        URL "https://github.com/cpp-pm/hunter/archive/v0.26.1.tar.gz"
        SHA1 "e41ac7a18c49b35ebac99ff2b5244317b2638a65"
        LOCAL
)

if(CMAKE_GENERATOR MATCHES "Ninja")
    message(FATAL_ERROR "This project does not support the Ninja generator. "
            "Please use Unix Makefiles or another supported generator. "
            "This error is typical in CLion. In this case, switch to generator Unix Makefiles.")
endif()

########################################################
project(ledger-zxlib)

set(CMAKE_CXX_STANDARD 11)
cmake_policy(SET CMP0025 NEW)
cmake_policy(SET CMP0144 NEW)

enable_testing()

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

###############

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

file(GLOB_RECURSE ZXLIB_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
        )

file(GLOB_RECURSE TESTS_SRC
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
        )

###############
set(BUILD_TESTS OFF CACHE BOOL "Enables tests")

add_library(zxlib STATIC ${ZXLIB_SRC})
target_include_directories(zxlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()

add_executable(zxlib_tests
        ${TESTS_SRC}
        )

target_include_directories(zxlib_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        )

target_link_libraries(zxlib_tests PRIVATE
        GTest::gtest_main
        zxlib)

add_test(ZXLIB_TESTS zxlib_tests)
