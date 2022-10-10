#*******************************************************************************
#*   (c) 2019 Zondax GmbH
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

.PHONY: all deps build clean load delete check_python show_info_recovery_mode

TESTS_SPECULOS_DIR?=$(CURDIR)/tests_speculos
EXAMPLE_VUE_DIR?=$(CURDIR)/example_vue
TESTS_JS_PACKAGE?=
TESTS_JS_DIR?=
TESTS_GENERATE_DIR?=$(CURDIR)/tests/generate-transaction-tests
METADATA_GENERATE_DIR?=$(CURDIR)/transaction_metadata

LEDGER_SRC=$(CURDIR)/app
DOCKER_APP_SRC=/project
DOCKER_APP_SRC_NEW=/app
DOCKER_APP_BIN=$(DOCKER_APP_SRC)/app/bin/app.elf

SPECULOS_MODEL_SWITCH=nanos
NANO_ICON_GIF=nanos_icon.gif
BOLOS_SDK_DIRECTORY=/opt/nanos-secure-sdk
TARGET_NAME=TARGET_NANOS
TEST_DEVICE=nanos
SPECULOS_SDK=2.1
ifeq ($(TARGET_DEVICE), NANO_X)
    $(info Targeting NanoX)
    SPECULOS_MODEL_SWITCH=nanox
    NANO_ICON_GIF=nanox_icon.gif
    BOLOS_SDK_DIRECTORY=/opt/nanox-secure-sdk
    TARGET_NAME=TARGET_NANOX
    TEST_DEVICE=nanox
    SPECULOS_SDK=2.0.2
endif
ifeq ($(TARGET_DEVICE), NANO_SP)
    $(info Targeting NanoSPlus)
    SPECULOS_MODEL_SWITCH=nanosp
    NANO_ICON_GIF=nanox_icon.gif
    BOLOS_SDK_DIRECTORY=/opt/nanosplus-secure-sdk
    TARGET_NAME=TARGET_NANOS2
    TEST_DEVICE=nanosp
    SPECULOS_SDK=1.0.3
endif


# todo: figure out if we are running on MacOS or Linux and how to set this env var dynamically: MAKE_LINUX_DOCKER_OPTIONS=--network host

ifndef MAKE_NVM_SH_PATH
export MAKE_NVM_SH_PATH=~
endif

# Note: This is not an SSH key, and being public represents no risk
SCP_PUBKEY=049bc79d139c70c83a4b19e8922e5ee3e0080bb14a2e8b0752aa42cda90a1463f689b0fa68c1c0246845c2074787b649d0d8a6c0b97d4607065eee3057bdf16b83
SCP_PRIVKEY=ff701d781f43ce106f72dc26a46b6a83e053b5d07bb3d4ceab79c91ca822a66b

INTERACTIVE:=$(shell [ -t 0 ] && echo 1)
USERID:=$(shell id -u)
ifndef RECURSE_QUIETLY
export RECURSE_QUIETLY=1
$(info USERID                : $(USERID))
$(info TESTS_ZEMU_DIR        : $(TESTS_ZEMU_DIR))
$(info EXAMPLE_VUE_DIR       : $(EXAMPLE_VUE_DIR))
$(info TESTS_JS_DIR          : $(TESTS_JS_DIR))
$(info TESTS_JS_PACKAGE      : $(TESTS_JS_PACKAGE))
$(info MAKE_NVM_SH_PATH      : $(MAKE_NVM_SH_PATH))
endif

ifdef INTERACTIVE
INTERACTIVE_SETTING:="-i"
TTY_SETTING:="-t"
else
INTERACTIVE_SETTING:=
TTY_SETTING:=
endif

define run_docker
	@echo "docker host: id -u: `id -u`"
	@echo "docker host: whoami: `whoami`"
	docker run $(TTY_SETTING) $(INTERACTIVE_SETTING) $(MAKE_LINUX_DOCKER_OPTIONS) --rm \
	-e SCP_PRIVKEY=$(SCP_PRIVKEY) \
	-e BOLOS_ENV_IGNORE=/opt/bolos \
	-e COIN=$(COIN) \
	-e APP_TESTING=$(APP_TESTING) \
	-e BOLOS_SDK=$(BOLOS_SDK_DIRECTORY) \
	-e TARGET_NAME=$(TARGET_NAME) \
	-u $(USERID):$(USERID) \
	-v $(shell pwd)/app:/app \
	-v $(shell pwd)/deps:/deps \
	$(1) \
	ledger-app-builder \
	$(2)
endef

all: build

.PHONY: check_todo
check_todo:
	git ls-files 2>&1 | xargs egrep -i "\b[t]odo:" 2> /dev/null || true

.PHONY: check_fixme
check_fixme:
	git ls-files 2>&1 | xargs egrep -i "\b[f]ixme:" 2> /dev/null

.PHONY: check_python
check_python:
	@python -c 'import sys; sys.exit(3-sys.version_info.major)' || (echo "The python command does not point to Python 3"; exit 1)

.PHONY: deps
deps: check_python
	@echo "Install dependencies"
	$(CURDIR)/deps/ledger-zxlib/scripts/install_deps.sh

.PHONY: convert_icon
convert_icon:
	@convert $(LEDGER_SRC)/tmp.gif -monochrome -size 16x16 -depth 1 $(LEDGER_SRC)/nanos_icon.gif
	@convert $(LEDGER_SRC)/nanos_icon.gif -crop 14x14+1+1 +repage -negate $(LEDGER_SRC)/nanox_icon.gif

.PHONY: pull_build_container
pull_build_container:
	docker version
	docker pull ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:sha-d864950
	docker image tag ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:sha-d864950 ledger-app-builder

.PHONY: build
build: pull_build_container
	$(info calling make inside docker)
	$(call run_docker, , make -j `nproc`)

.PHONY: clean
clean: pull_build_container
	$(call run_docker, ,make clean)

.PHONY: listvariants
listvariants: pull_build_container
	$(call run_docker, ,make listvariants)

.PHONY: shell
shell: pull_build_container
	$(call run_docker, -ti, bash)

.PHONY: load
load:
	${LEDGER_SRC}/pkg/zxtool.sh load

.PHONY: delete
delete:
	${LEDGER_SRC}/pkg/zxtool.sh delete

.PHONY: show_info_recovery_mode
show_info_recovery_mode:
	@echo "This command requires a Ledger Nano S in recovery mode. To go into recovery mode, follow:"
	@echo " 1. Settings -> Device -> Reset all and confirm"
	@echo " 2. Unplug device, press and hold the right button, plug-in again"
	@echo " 3. Navigate to the main menu"
	@echo "If everything was correct, no PIN needs to be entered."

# This target will initialize the device with the integration testing mnemonic
.PHONY: dev_init
dev_init: show_info_recovery_mode
	@echo "Initializing device with test mnemonic! WARNING TAKES 2 MINUTES AND REQUIRES RECOVERY MODE"
	@python -m ledgerblue.hostOnboard --apdu --id 0 --prefix "" --passphrase "" --pin 5555 --words "equip will roof matter pink blind book anxiety banner elbow sun young"

# This target will initialize the device with the secondary integration testing mnemonic (Bob)
.PHONY: dev_init_secondary
dev_init_secondary: check_python show_info_recovery_mode
	@echo "Initializing device with secondary test mnemonic! WARNING TAKES 2 MINUTES AND REQUIRES RECOVERY MODE"
	@python -m ledgerblue.hostOnboard --apdu --id 0 --prefix "" --passphrase "" --pin 5555 --words "elite vote proof agree february step sibling sand grocery axis false cup"

# This target will setup a custom developer certificate
.PHONY: dev_ca
dev_ca: check_python
	@python -m ledgerblue.setupCustomCA --targetId 0x31100004 --public $(SCP_PUBKEY) --name zondax

# This target will setup a custom developer certificate
.PHONY: dev_caX
dev_caX: check_python
	@python -m ledgerblue.setupCustomCA --targetId 0x33000004 --public $(SCP_PUBKEY) --name zondax

.PHONY: dev_ca_delete
dev_ca_delete: check_python
	@python -m ledgerblue.resetCustomCA --targetId 0x31100004

# This target will setup a custom developer certificate
.PHONY: dev_ca2
dev_ca2: check_python
	@python -m ledgerblue.setupCustomCA --targetId 0x33000004 --public $(SCP_PUBKEY) --name zondax

.PHONY: dev_ca_delete2
dev_ca_delete2: check_python
	@python -m ledgerblue.resetCustomCA --targetId 0x33000004

########################## VUE Section ###############################

.PHONY: vue_install_js_link
ifeq ($(TESTS_JS_DIR),)
vue_install_js_link:
	@echo "No local package defined"
else
vue_install_js_link:
	# First unlink everything
	cd $(TESTS_JS_DIR) && yarn unlink || true
	cd $(EXAMPLE_VUE_DIR) && yarn unlink $(TESTS_JS_PACKAGE) || true
#	# Now build and link
	cd $(TESTS_JS_DIR) && yarn install && yarn build && yarn link || true
	cd $(EXAMPLE_VUE_DIR) && yarn link $(TESTS_JS_PACKAGE) && yarn install || true
	@echo
	# List linked packages
	@echo
	@cd $(EXAMPLE_VUE_DIR) && ( ls -l node_modules ; ls -l node_modules/@* ) | grep ^l || true
	@echo
endif

.PHONY: vue
vue: vue_install_js_link
	cd $(EXAMPLE_VUE_DIR) && yarn install && yarn serve

########################## VUE Section ###############################
.PHONY: speculos_install_js_link
ifeq ($(TESTS_JS_DIR),)
speculos_install_js_link:
	@echo "No local package defined"
else
speculos_install_js_link:
	$(call run_announce,$@)
	# First unlink everything
	@# todo: workaround to avoid this log output? 'error No registered package found called "@onflow/ledger"'
	cd $(TESTS_JS_DIR) && yarn unlink || true
	cd $(TESTS_SPECULOS_DIR) && yarn unlink $(TESTS_JS_PACKAGE) || true
	# Now build and link
	cd $(TESTS_JS_DIR) && yarn install && yarn build && yarn link || true
	cd $(TESTS_SPECULOS_DIR) && yarn link $(TESTS_JS_PACKAGE) && yarn install || true
	@echo
	# List linked packages
	@echo
	@cd $(TESTS_SPECULOS_DIR) && ( ls -l node_modules ; ls -l node_modules/@* ) | grep ^l || true
	@echo
endif

.PHONY: speculos_pull_container
speculos_pull_container:
	docker pull ghcr.io/ledgerhq/speculos:sha-f430e51
	docker image tag ghcr.io/ledgerhq/speculos:sha-f430e51 speculos

.PHONY: speculos_install
speculos_install: speculos_install_js_link speculos_pull_container
	$(call run_announce,$@)
	# check for nvm, node, npm, and yarn
	@. $(MAKE_NVM_SH_PATH)/.nvm/nvm.sh ; nvm --version 2>&1 | perl -lane '$$nvm=$$_; chomp $$nvm; printf qq[# nvm --version: %s\n], $$nvm; if($$nvm!~m~^\d+\.\d+\.\d+$$~){ die qq[ERROR: nvm not installed? Please install, e.g. MacOS/Ubuntu: curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.0/install.sh | bash ; export NVM_DIR="$$HOME/.nvm" ; [ -s "$$NVM_DIR/nvm.sh" ] && \. "$$NVM_DIR/nvm.sh" ; [ -s "$$NVM_DIR/bash_completion" ] && \. "$$NVM_DIR/bash_completion" # see https://github.com/nvm-sh/nvm#installing-and-updating\n]; }'
	@node --version 2>&1 | perl -lane '$$node=$$_; chomp $$node; printf qq[# node --version: %s\n], $$node; if($$node!~m~^v16\.10\.0$$~){ die qq[ERROR: desired node version not installed? Please install, e.g. MacOS/Ubuntu: nvm install 16.10.0 ; nvm use 16.10.0]; }'
	@perl -e '$$yarn=`which yarn`; chomp $$yarn; printf qq[# which yarn: %s\n], $$yarn; if($$yarn=~m~^\s*$$~){ die qq[ERROR: yarn not installed? Please install, e.g. Ubuntu: sudo npm install --global yarn Linux: brew install yarn\n]; }'
	# delete node_modules
	cd $(TESTS_SPECULOS_DIR) && rm -rf node_modules
	# run yarn install
	@cd $(TESTS_SPECULOS_DIR) && yarn install

.PHONY: speculos_port_5001_start
speculos_port_5001_start: speculos_pull_container
	$(call run_announce,$@)
	$(call start_speculos_container,5001,40001,41001,/app/bin)

.PHONY: speculos_port_5002_start 
speculos_port_5002_start: speculos_pull_container
	$(call run_announce,$@)
	$(call start_speculos_container,5002,40002,41002,/app/bin)

.PHONY: speculos_port_5003_start 
speculos_port_5003_start: speculos_pull_container
	$(call run_announce,$@)
	$(call start_speculos_container,5003,40003,41003,/tests_speculos/backward_compatibility_test_biniaries/0-9-12/)

.PHONY: speculos_port_5001_stop 
speculos_port_5001_stop:
	$(call run_announce,$@)
	$(call stop_speculos_container,5001)

.PHONY: speculos_port_5002_stop
speculos_port_5002_stop:
	$(call run_announce,$@)
	$(call stop_speculos_container,5002)

.PHONY: speculos_port_5003_stop
speculos_port_5003_stop:
	$(call run_announce,$@)
	$(call stop_speculos_container,5003)

########################## TEST Section ###############################

.PHONY: generate_test_vectors
generate_test_vectors:
	cd $(TESTS_GENERATE_DIR) && yarn run generate

# Note: This error did occur once: docker: Error response from daemon: driver failed programming external connectivity on endpoint speculos-port-5002 (082ea92d039f260880bc264a2f6086e14c42d698f24ff5acbba422baa9c60b29): Error starting userland proxy: listen tcp 0.0.0.0:41002: bind: address already in use.
# Note: Since we do not need to use the VNC for the tests, then remove this option and hope the error never shows up again: --vnc-port $(3)
define start_speculos_container
	docker run --detach --name speculos-port-$(1) --rm -it -v $(CURDIR)$(4):/speculos/app/bin --publish $(1):$(1) --publish $(2):$(2) --publish $(3):$(3) speculos --model $(SPECULOS_MODEL_SWITCH) --sdk $(SPECULOS_SDK) --seed "equip will roof matter pink blind book anxiety banner elbow sun young" --display headless --apdu-port $(2) --api-port $(1) ./app/bin/app.elf ; rm -f $(TESTS_SPECULOS_DIR)/speculos-port-$(1).log ; docker logs --follow speculos-port-$(1) 2>&1 | tee -a $(TESTS_SPECULOS_DIR)/speculos-port-$(1).log > /dev/null 2>&1 &
	@perl -e 'use Time::HiRes; $$t1=Time::HiRes::time(); while(1){ $$o=`cat $(TESTS_SPECULOS_DIR)/speculos-port-$(1).log`; if($$o =~ m~Running on .*\:$(1)~s){ printf qq[# detected -- via log -- speculos listening after %f seconds; spy on emulated device via http://localhost:$(1)/\n], Time::HiRes::time() - $$t1; exit; } Time::HiRes::sleep(0.01); };'
endef

define stop_speculos_container
	# make: todo: using --time 0 because this stops the docker container faster; but it still takes ~ 2.4 seconds... how to stop faster?
	docker stop --time 0 speculos-port-$(1)
endef

define run_announce
	@perl -e 'use Time::HiRes; use POSIX; $$ts = sprintf qq[%f], Time::HiRes::time(); ($$f) = $$ts =~ m~(\....)~; printf qq[%s%s %s make: %s\n], POSIX::strftime("%H:%M:%S", gmtime), $$f, q[-] x 126, $$ARGV[0];' "$(1)"
endef

define run_nodejs_test
	@cd $(TESTS_SPECULOS_DIR) \
	&& { { { \
          TEST_SPECULOS_API_PORT=$(1) TEST_SPECULOS_APDU_PORT=$(2) TEST_DEVICE=$(TEST_DEVICE) node $(3) 2>&1; echo $$? >&3; \
        } | tee -a speculos-port-$(1).log >&4; } 3>&1 | { read xs; exit $$xs; } } 4>&1
endef

#{ { { { echo "node" 2>&1; sleep 5; false; } ; echo $? >&3; } | tee -a test.test >&4; } 3>&1 | { read xs;exit $xs; } } 4>&1


.PHONY: speculos_port_5001_test_internal
speculos_port_5001_test_internal:
	$(call run_announce,$@)
	$(call run_nodejs_test,5001,40001,test-basic-app-version.js)
	$(call run_nodejs_test,5001,40001,test-basic-slot-status-bad-net.js)
	$(call run_nodejs_test,5001,40001,test-basic-slot-status-full.js)
	$(call run_nodejs_test,5001,40001,test-basic-sign-basic-invalid.js)
	$(call run_nodejs_test,5001,40001,test-basic-get-address.js)
	$(call run_nodejs_test,5001,40001,test-basic-show-address-secp256k1.js)
	$(call run_nodejs_test,5001,40001,test-basic-show-address-secp256r1.js)
	$(call run_nodejs_test,5001,40001,test-basic-show-address-expert.js)
	$(call run_nodejs_test,5001,40001,test-menu.js)
	@echo "# ALL TESTS COMPLETED!" | tee -a $(TESTS_SPECULOS_DIR)/speculos-port-5001.log

.PHONY: speculos_port_5002_test_internal
speculos_port_5002_test_internal:
	$(call run_announce,$@)
	$(call run_nodejs_test,5002,40002,test-slot-transaction-interaction.js)	
	$(call run_nodejs_test,5002,40002,test-transaction-expert-mode.js)	
	$(call run_nodejs_test,5002,40002,test-transactions.js)
	@echo "# ALL TESTS COMPLETED!" | tee -a $(TESTS_SPECULOS_DIR)/speculos-port-5002.log

.PHONY: speculos_port_5003_test_internal
speculos_port_5003_test_internal:
	$(call run_announce,$@)
	$(call run_nodejs_test,5003,40003,test-backward-compatibility-0-9-12.js)	
	@echo "# ALL TESTS COMPLETED!" | tee -a $(TESTS_SPECULOS_DIR)/speculos-port-5003.log

.PHONY: speculos_port_5001_test
speculos_port_5001_test:
	$(call run_announce,$@)
	$(MAKE) --no-print-directory speculos_port_5001_start && ($(MAKE) --no-print-directory speculos_port_5001_test_internal; ret=$$?;$(MAKE) --no-print-directory speculos_port_5001_stop;$(call run_announce,note: logs: cat $(TESTS_SPECULOS_DIR)/speculos-port-5001.log);cat $(TESTS_SPECULOS_DIR)/speculos-port-5001.log; exit $$ret)

.PHONY: speculos_port_5002_test
speculos_port_5002_test:
	$(call run_announce,$@)
	$(MAKE) --no-print-directory speculos_port_5002_start && ($(MAKE) --no-print-directory speculos_port_5002_test_internal; ret=$$?;$(MAKE) --no-print-directory speculos_port_5002_stop;$(call run_announce,note: logs: cat $(TESTS_SPECULOS_DIR)/speculos-port-5002.log);cat $(TESTS_SPECULOS_DIR)/speculos-port-5002.log; exit $$ret)

.PHONY: speculos_port_5003_test
speculos_port_5003_test:
	$(call run_announce,$@)
	$(MAKE) --no-print-directory speculos_port_5003_start && ($(MAKE) --no-print-directory speculos_port_5003_test_internal; ret=$$?;$(MAKE) --no-print-directory speculos_port_5003_stop;$(call run_announce,note: logs: cat $(TESTS_SPECULOS_DIR)/speculos-port-5003.log);cat $(TESTS_SPECULOS_DIR)/speculos-port-5003.log; exit $$ret)

.PHONY: rust_test
rust_test:
	cd app/rust && cargo test

.PHONY: cpp_test
cpp_test:
	mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make
	cd build && GTEST_COLOR=1 ASAN_OPTIONS=detect_leaks=0 ctest -VV

########################## Ledger Test ###############################

.PHONY: ledger_test
ledger_test:
	$(call run_announce,$@)
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-app-version.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-slot-status-bad-net.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-slot-status-full.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-sign-basic-invalid.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-get-address.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-show-address-secp256k1.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-show-address-secp256r1.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-basic-show-address-expert.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-menu.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-slot-transaction-interaction.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-transaction-expert-mode.js
	@cd $(TESTS_SPECULOS_DIR) && TEST_ON_DEVICE=LEDGER TEST_DEVICE=$(TEST_DEVICE) node test-transactions.js
	@echo "# ALL TESTS COMPLETED!"

########################## TRANSACTION METADATA Section ###############################

.PHONY: generate_transaction_metadata
generate_transaction_metadata:
	cd $(METADATA_GENERATE_DIR) && yarn run generate

########################## FUZZING Section ###############################

.PHONY: fuzz_build
fuzz_build:
	cmake -B build -DCMAKE_C_COMPILER=clang-10 -DCMAKE_CXX_COMPILER=clang++-10 -DCMAKE_BUILD_TYPE=Debug -DENABLE_FUZZING=1 -DENABLE_SANITIZERS=1 .
	make -C build

.PHONY: fuzz
fuzz: fuzz_build
	./fuzz/run-fuzzers.py

.PHONY: fuzz_crash
fuzz_crash: fuzz_build
	./fuzz/run-fuzz-crashes.py
