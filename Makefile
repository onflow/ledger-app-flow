#*******************************************************************************
#   Ledger App
#   (c) 2020 Zondax GmbH
#   (c) 2017 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
$(error BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.defines

########################################
#        Mandatory configuration       #
########################################
# Application name
APPNAME = "Flow"

APPVERSION_M=0
APPVERSION_N=15
APPVERSION_P=0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# Application source files
APP_SOURCE_PATH += src
APP_SOURCE_PATH += deps/ledger-zxlib/include
APP_SOURCE_PATH += deps/ledger-zxlib/src
APP_SOURCE_PATH += deps/ledger-zxlib/app/common
APP_SOURCE_PATH += deps/ledger-zxlib/app/ui
APP_SOURCE_PATH += deps/jsmn/src
APP_SOURCE_FILES += ${BOLOS_SDK}/lib_standard_app/crypto_helpers.c

# Application icons following guidelines:
# https://developers.ledger.com/docs/embedded-app/design-requirements/#device-icon
ICON_NANOS = icons/nanos_icon.gif
ICON_NANOX = icons/nanox_icon.gif
ICON_NANOSP = icons/nanox_icon.gif
ICON_STAX = icons/stax_icon.gif
ICON_FLEX = icons/flex_icon.png

# Application allowed derivation curves.
# Possibles curves are: secp256k1, secp256r1, ed25519 and bls12381g1
# If your app needs it, you can specify multiple curves by using:
# `CURVE_APP_LOAD_PARAMS = <curve1> <curve2>`
CURVE_APP_LOAD_PARAMS = secp256r1 secp256k1

# Application allowed derivation paths.
# You should request a specific path for your app.
# This serve as an isolation mechanism.
# Most application will have to request a path according to the BIP-0044
# and SLIP-0044 standards.
# If your app needs it, you can specify multiple path by using:
# `PATH_APP_LOAD_PARAMS = "44'/1'" "45'/1'"`
PATH_APP_LOAD_PARAMS = "44'/539'" "44'/1'"


# Setting to allow building variant applications
# - <VARIANT_PARAM> is the name of the parameter which should be set
#   to specify the variant that should be build.
# - <VARIANT_VALUES> a list of variant that can be build using this app code.
#   * It must at least contains one value.
#   * Values can be the app ticker or anything else but should be unique.
VARIANT_PARAM = COIN
VARIANT_VALUES = FLOW

# Enabling DEBUG flag will enable PRINTF and disable optimizations
#DEBUG = 1

#######################################
#     Application custom permissions   #
########################################
# See SDK `include/appflags.h` for the purpose of each permission
#HAVE_APPLICATION_FLAG_DERIVE_MASTER = 1
#HAVE_APPLICATION_FLAG_GLOBAL_PIN = 1
#HAVE_APPLICATION_FLAG_BOLOS_SETTINGS = 1
#HAVE_APPLICATION_FLAG_LIBRARY = 1

########################################
# Application communication interfaces #
########################################
ENABLE_BLUETOOTH = 1
#ENABLE_NFC = 1

########################################
#         NBGL custom features         #
########################################
#ENABLE_NBGL_QRCODE = 1
#ENABLE_NBGL_KEYBOARD = 1
#ENABLE_NBGL_KEYPAD = 1

########################################
#          Features disablers          #
########################################
# These advanced settings allow to disable some feature that are by
# default enabled in the SDK `Makefile.standard_app`.
DISABLE_STANDARD_APP_FILES = 1
#DISABLE_DEFAULT_IO_SEPROXY_BUFFER_SIZE = 1 # To allow custom size declaration
#DISABLE_STANDARD_APP_DEFINES = 1 # Will set all the following disablers
#DISABLE_STANDARD_SNPRINTF = 1
#DISABLE_STANDARD_USB = 1
#DISABLE_STANDARD_WEBUSB = 1
ifeq ($(TARGET_NAME),TARGET_NANOS)
DISABLE_STANDARD_BAGL_UX_FLOW = 1
endif

########################################
#        Main app configuration        #
########################################

DEFINES   += HAVE_BOLOS_APP_STACK_CANARY
DEFINES   += LEDGER_SPECIFIC

ifeq ($(TARGET_NAME),TARGET_NANOS)
APP_STACK_MIN_SIZE:=2480
endif

########################################
#        Unit tests and misc           #
########################################

ifeq ($(DEBUG),1)
DEFINES += ZEMU_LOGGING
endif

PRODUCTION_BUILD ?= 1
ifeq ($(PRODUCTION_BUILD),1)
DEFINES += PRODUCTION_BUILD=1
endif

#########################

# Import generic rules from the SDK
include $(BOLOS_SDK)/Makefile.standard_app
