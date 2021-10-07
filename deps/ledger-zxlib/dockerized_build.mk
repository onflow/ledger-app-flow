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

TESTS_ZEMU_DIR?=$(CURDIR)/tests_zemu
TESTS_SPECULOS_DIR?=$(CURDIR)/tests_speculos
EXAMPLE_VUE_DIR?=$(CURDIR)/example_vue
TESTS_JS_PACKAGE?=
TESTS_JS_DIR?=
TESTS_GENERATE_DIR?=$(CURDIR)/tests/generate-transaction-tests

LEDGER_SRC=$(CURDIR)/app
DOCKER_APP_SRC=/project
DOCKER_APP_BIN=$(DOCKER_APP_SRC)/app/bin/app.elf

DOCKER_BOLOS_SDK=/project/deps/nanos-secure-sdk
DOCKER_BOLOS_SDKX=/project/deps/nano2-sdk

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
endif

DOCKER_IMAGE=zondax-builder-bolos-2021-10-04

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
	docker version
	echo "TODO: this is all cached and fast on a local box, but takes over 4 minutes on CircleCI :-("
	cat deps/ledger-zxlib/Dockerfile.template | perl -lane 'sub BEGIN{ $$userid=`id -u`; chomp $$userid; } s~<USERID>~$$userid~g; print;' > deps/ledger-zxlib/Dockerfile
	docker build --tag zondax-builder-bolos-2021-10-04 deps/ledger-zxlib/
	docker run $(TTY_SETTING) $(INTERACTIVE_SETTING) --rm \
	-e SCP_PRIVKEY=$(SCP_PRIVKEY) \
	-e BOLOS_SDK=$(1) \
	-e BOLOS_ENV_IGNORE=/opt/bolos \
	-u $(USERID):$(USERID) \
	-v $(shell pwd):/project \
	$(DOCKER_IMAGE) \
	"COIN=$(COIN) APP_TESTING=$(APP_TESTING) $(2)"
endef

all: build

.PHONY: check_python
check_python:
	@python -c 'import sys; sys.exit(3-sys.version_info.major)' || (echo "The python command does not point to Python 3"; exit 1)

.PHONY: deps
deps: check_python
	@echo "Install dependencies"
	$(CURDIR)/deps/ledger-zxlib/scripts/install_deps.sh

.PHONY: pull
pull:
	docker pull $(DOCKER_IMAGE)

.PHONY: build_rust
build_rust:
	$(call run_docker,$(DOCKER_BOLOS_SDK),make -C $(DOCKER_APP_SRC) rust)

.PHONY: convert_icon
convert_icon:
	@convert $(LEDGER_SRC)/tmp.gif -monochrome -size 16x16 -depth 1 $(LEDGER_SRC)/nanos_icon.gif
	@convert $(LEDGER_SRC)/nanos_icon.gif -crop 14x14+1+1 +repage -negate $(LEDGER_SRC)/nanox_icon.gif

.PHONY: build
build:
	$(info Replacing app icon)
	@cp $(LEDGER_SRC)/nanos_icon.gif $(LEDGER_SRC)/glyphs/icon_app.gif
	$(info calling make inside docker)
	$(call run_docker,$(DOCKER_BOLOS_SDK),make -j `nproc` -C $(DOCKER_APP_SRC))

.PHONY: buildX
buildX: build_rust
	@cp $(LEDGER_SRC)/nanos_icon.gif $(LEDGER_SRC)/glyphs/icon_app.gif
	@convert $(LEDGER_SRC)/nanos_icon.gif -crop 14x14+1+1 +repage -negate $(LEDGER_SRC)/nanox_icon.gif
	$(call run_docker,$(DOCKER_BOLOS_SDKX),make -j `nproc` -C $(DOCKER_APP_SRC))

.PHONY: clean
clean:
	$(call run_docker,$(DOCKER_BOLOS_SDK),make -C $(DOCKER_APP_SRC) clean)

.PHONY: clean_rust
clean_rust:
	$(call run_docker,$(DOCKER_BOLOS_SDK),make -C $(DOCKER_APP_SRC) rust_clean)

.PHONY: listvariants
listvariants:
	$(call run_docker,$(DOCKER_BOLOS_SDK),make -C $(DOCKER_APP_SRC) listvariants)

.PHONY: shell
shell:
	$(call run_docker,$(DOCKER_BOLOS_SDK) -t,bash)

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

.PHONY: zemu_install_js_link
ifeq ($(TESTS_JS_DIR),)
zemu_install_js_link:
	@echo "No local package defined"
else
zemu_install_js_link:
	# First unlink everything
	cd $(TESTS_JS_DIR) && yarn unlink || true
	cd $(TESTS_ZEMU_DIR) && yarn unlink $(TESTS_JS_PACKAGE) || true
	# Now build and link
	cd $(TESTS_JS_DIR) && yarn install && yarn build && yarn link || true
	cd $(TESTS_ZEMU_DIR) && yarn link $(TESTS_JS_PACKAGE) && yarn install || true
	@echo
	# List linked packages
	@echo
	@cd $(TESTS_ZEMU_DIR) && ( ls -l node_modules ; ls -l node_modules/@* ) | grep ^l || true
	@echo
endif

.PHONY: zemu_install
zemu_install: zemu_install_js_link
	# and now install everything
	cd $(TESTS_ZEMU_DIR) && yarn install

.PHONY: speculos_install
speculos_install:
	# pull speculos container
	docker pull ghcr.io/ledgerhq/speculos
	docker image tag ghcr.io/ledgerhq/speculos speculos
	# check for nvm, node, npm, and yarn
	@. ~/.nvm/nvm.sh ; nvm --version 2>&1 | perl -lane '$$nvm=$$_; chomp $$nvm; printf qq[# nvm --version: %s\n], $$nvm; if($$nvm!~m~^\d+\.\d+\.\d+$$~){ die qq[ERROR: nvm not installed? Please install, e.g. MacOS/Ubuntu: curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.0/install.sh | bash ; export NVM_DIR="$$HOME/.nvm" ; [ -s "$$NVM_DIR/nvm.sh" ] && \. "$$NVM_DIR/nvm.sh" ; [ -s "$$NVM_DIR/bash_completion" ] && \. "$$NVM_DIR/bash_completion" # see https://github.com/nvm-sh/nvm#installing-and-updating\n]; }'
	@node --version 2>&1 | perl -lane '$$node=$$_; chomp $$node; printf qq[# node --version: %s\n], $$node; if($$node!~m~^v16\.10\.0$$~){ die qq[ERROR: desired node version not installed? Please install, e.g. MacOS/Ubuntu: nvm install 16.10.0 ; nvm use 16.10.0]; }'
	@perl -e '$$yarn=`which yarn`; chomp $$yarn; printf qq[# which yarn: %s\n], $$yarn; if($$yarn=~m~^\s*$$~){ die qq[ERROR: yarn not installed? Please install, e.g. Ubuntu: sudo npm install --global yarn Linux: brew install yarn\n]; }'
	# run yarn install
	@cd $(TESTS_SPECULOS_DIR) && yarn install

.PHONY: speculos_port_5001_start
speculos_port_5001_start:
	@perl -e 'use Time::HiRes; use POSIX; $$ts = sprintf qq[%f], Time::HiRes::time(); ($$f) = $$ts =~ m~(\....)~; printf qq[%s%s %s make: %s\n], POSIX::strftime("%H:%M:%S", gmtime), $$f, q[-] x 96, $$ARGV[0];' "$@"
	# make: todo: figure out how to use '--net host' because this starts the docker container faster: https://github.com/moby/moby/issues/38077
	#       but using '--net host' causes the speculos container NOT to listen on any ports... how to fix this? see https://github.com/LedgerHQ/speculos/issues/249
	docker run --detach --name speculos-port-5001 --rm -it -v $(CURDIR)/app:/speculos/app --publish 5001:5001 --publish 41001:41001 speculos --model nanos --sdk 2.0 --seed "secret" --display headless --apdu-port 40001 --vnc-port 41001 --api-port 5001 ./app/bin/app.elf ; rm -f ../speculos-port-5001.log ; docker logs --follow speculos-port-5001 2>&1 | tee -a ../speculos-port-5001.log > /dev/null 2>&1 &
	@perl -e 'use Time::HiRes; $$t1=Time::HiRes::time(); while(1){ $$o=`cat ../speculos-port-5001.log`; if($$o =~ m~Running on .*\:5001~s){ printf qq[# via log detected speculos listening in %f seconds\n], Time::HiRes::time() - $$t1; exit; } Time::HiRes::sleep(0.01); };'

.PHONY: speculos_port_5001_stop
speculos_port_5001_stop:
	@perl -e 'use Time::HiRes; use POSIX; $$ts = sprintf qq[%f], Time::HiRes::time(); ($$f) = $$ts =~ m~(\....)~; printf qq[%s%s %s make: %s\n], POSIX::strftime("%H:%M:%S", gmtime), $$f, q[-] x 96, $$ARGV[0];' "$@"
	# make: todo: using --time 0 because this stops the docker container faster; but it still takes ~ 2.4 seconds... how to stop faster?
	docker stop --time 0 speculos-port-5001

.PHONY: zemu
zemu:
	cd $(TESTS_ZEMU_DIR)/tools && node debug.mjs $(COIN)

.PHONY: zemu_val
zemu_val:
	cd $(TESTS_ZEMU_DIR)/tools && node debug_val.mjs

.PHONY: zemu_debug
zemu_debug:
	cd $(TESTS_ZEMU_DIR)/tools && node debug.mjs $(COIN) debug

########################## TEST Section ###############################

.PHONY: generate_test_vectors
generate_test_vectors:
	cd $(TESTS_GENERATE_DIR) && yarn run generate

.PHONY: zemu_test
zemu_test:
	cd $(TESTS_ZEMU_DIR) && yarn test$(COIN)

.PHONY: speculos_port_5001_test_internal
speculos_port_5001_test_internal:
	@perl -e 'use Time::HiRes; use POSIX; $$ts = sprintf qq[%f], Time::HiRes::time(); ($$f) = $$ts =~ m~(\....)~; printf qq[%s%s %s make: %s\n], POSIX::strftime("%H:%M:%S", gmtime), $$f, q[-] x 96, $$ARGV[0];' "$@"
	cd $(TESTS_SPECULOS_DIR) && TEST_SPECULOS_API_PORT=5001 TEST_DEBUG=1 node test-basic-slot-status-set.js 2>&1 | tee -a ../../speculos-port-5001.log 2>&1 | perl -lane '$$lines .= $$_ . "\n"; printf qq[%s\n], $$_ if(m~test_(start|end)~); sub END{ die qq[ERROR: test_end not detected; test failed?\n] if($$lines !~ m~test_end~s); }'
	@echo "# ALL TESTS COMPLETED!" | tee -a ../speculos-port-5001.log

.PHONY: speculos_port_5001_test
speculos_port_5001_test:
	@perl -e 'use Time::HiRes; use POSIX; $$ts = sprintf qq[%f], Time::HiRes::time(); ($$f) = $$ts =~ m~(\....)~; printf qq[%s%s %s make: %s\n], POSIX::strftime("%H:%M:%S", gmtime), $$f, q[-] x 96, $$ARGV[0];' "$@"
	@make --no-print-directory speculos_port_5001_start
	@-make --no-print-directory speculos_port_5001_test_internal
	@make --no-print-directory speculos_port_5001_stop
	@perl -e 'use Time::HiRes; use POSIX; $$ts = sprintf qq[%f], Time::HiRes::time(); ($$f) = $$ts =~ m~(\....)~; printf qq[%s%s %s make: %s\n], POSIX::strftime("%H:%M:%S", gmtime), $$f, q[-] x 96, $$ARGV[0];' "note: for detailed logs: cat ../speculos-port-5001.log"
	@cat ../speculos-port-5001.log 2>&1 | perl -lane '$$lines .= $$_ . "\n"; sub END{ die sprintf qq[ERROR: All tests did NOT complete! Dumping ../speculos-port-5001.log:\n%s\n], $$lines if($$lines !~ m~ALL TESTS COMPLETED~s); }'

.PHONY: rust_test
rust_test:
	cd app/rust && cargo test

.PHONY: cpp_test
cpp_test:
	mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make
	cd build && GTEST_COLOR=1 ASAN_OPTIONS=detect_leaks=0 ctest -VV

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
