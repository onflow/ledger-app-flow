# Simple Makefile for zxlib tests
# Wrapper around CMake build system

.PHONY: all build test clean test-verbose test-filter fuzz fuzz_crash fuzz_clean build_fuzz fuzz_report fuzz_report_html fuzz_help format

# Default target
all: build

# Build the library and tests
build:
	mkdir -p build
	cd build && cmake -DCMAKE_BUILD_TYPE=Debug ..
	cd build && make

# Run C++ tests
test: build
	cd build && GTEST_COLOR=1 ASAN_OPTIONS=detect_leaks=0 ctest -VV

# Clean build artifacts
clean:
	rm -rf build

# Run tests with verbose output on failure
test-verbose: build
	cd build && GTEST_COLOR=1 ASAN_OPTIONS=detect_leaks=0 ctest -VV --output-on-failure

# Run specific test
test-filter: build
	cd build && ./zxlib_tests --gtest_filter="$(FILTER)"

format:
	find src evm tests include app fuzzing \( -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' -o -iname '*.hpp' \) | xargs clang-format -i
	
# === FUZZING TARGETS ===
# Proxy fuzzing targets to fuzzing/Makefile for convenience
build_fuzz:
	@cd fuzzing && $(MAKE) build_fuzz

fuzz:
	@cd fuzzing && $(MAKE) fuzz

fuzz_crash:
	@cd fuzzing && $(MAKE) fuzz_crash

fuzz_clean:
	@cd fuzzing && $(MAKE) fuzz_clean

fuzz_report:
	@cd fuzzing && $(MAKE) fuzz_report

fuzz_report_html:
	@cd fuzzing && $(MAKE) fuzz_report_html

fuzz_help:
	@echo "Fuzzing targets for ledger-zxlib:"
	@echo ""
	@echo "All fuzzing commands have been moved to fuzzing/Makefile"
	@echo "You can either:"
	@echo "  1. Use proxy commands from here: make fuzz, make fuzz_crash, etc."
	@echo "  2. Go to fuzzing directory: cd fuzzing && make help"
