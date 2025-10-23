# Makefile for ledger-zxlib fuzzing
# This file contains both fuzzing targets and Python formatting rules

# === FUZZING CONFIGURATION ===
# All fuzzing configuration is now managed in fuzz_local/fuzz_config.py
FUZZ_DIR = fuzz_local
FUZZ_BUILD_DIR = $(FUZZ_DIR)/build
FUZZ_COVERAGE_DIR = $(FUZZ_DIR)/coverage
PYTHON ?= python3

# === PYTHON FORMATTING CONFIGURATION ===
BLACK_LINE_LENGTH := 120
FUZZING_DIR := .

# Default target
.PHONY: help
help:
	@echo "ledger-zxlib Fuzzing & Development Tools"
	@echo ""
	@echo "=== FUZZING TARGETS ==="
	@echo "  build_fuzz    Build fuzzer targets"
	@echo "  fuzz          Run all fuzzers (uses config from fuzz_config.py)"
	@echo "  fuzz_crash    Analyze any crashes found"
	@echo "  fuzz_clean    Clean fuzzing artifacts"
	@echo "  fuzz_report   Generate coverage report"
	@echo "  fuzz_report_html Generate HTML coverage report"
	@echo ""
	@echo "=== PYTHON FORMATTING ==="
	@echo "  format        Format Python files with Black"
	@echo "  check         Check if files need formatting"
	@echo "  format_clean  Remove Python cache files"
	@echo ""
	@echo "Configuration:"
	@echo "  Edit fuzz_local/fuzz_config.py to change fuzzing parameters"
	@echo ""
	@echo "Examples:"
	@echo "  make fuzz                 # Run with default config"
	@echo "  make fuzz_report          # Generate coverage report"

# === FUZZING TARGETS ===

# Check if clang is available
.PHONY: check_clang
check_clang:
	@command -v clang >/dev/null 2>&1 || { \
		echo "❌ Error: clang not found. Please install clang for fuzzing."; \
		echo "On macOS: brew install llvm"; \
		echo "On Ubuntu: apt install clang"; \
		exit 1; \
	}
	@command -v clang++ >/dev/null 2>&1 || { \
		echo "❌ Error: clang++ not found. Please install clang++ for fuzzing."; \
		echo "On macOS: brew install llvm"; \
		echo "On Ubuntu: apt install clang"; \
		exit 1; \
	}
	@echo "✅ Clang found: $$(clang --version | head -n1)"
	@echo "✅ Clang++ found: $$(clang++ --version | head -n1)"

# Build fuzzer targets
.PHONY: build_fuzz
build_fuzz: check_clang
	@echo "🔨 Building fuzzer targets..."
	@mkdir -p $(FUZZ_BUILD_DIR)
	@cd $(FUZZ_BUILD_DIR) && \
		cmake -DCMAKE_C_COMPILER=clang \
			  -DCMAKE_CXX_COMPILER=clang++ \
			  -DENABLE_FUZZING=ON \
			  -DENABLE_SANITIZERS=ON \
			  -DCMAKE_BUILD_TYPE=Debug \
			  .. && \
		make -j$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
	@echo "✅ Fuzzer build completed"

# Run fuzzers
.PHONY: fuzz
fuzz: build_fuzz
	@echo "🚀 Starting fuzzing session..."
	@echo "   Configuration from: $(FUZZ_DIR)/fuzz_config.py"
	@$(PYTHON) $(FUZZ_DIR)/run_local_fuzz.py
	@echo "🏁 Fuzzing session completed"

# Analyze crashes
.PHONY: fuzz_crash
fuzz_crash: build_fuzz
	@echo "🔍 Analyzing fuzzing crashes..."
	@$(PYTHON) $(FUZZ_DIR)/analyze_local_crashes.py

# Clean fuzzing artifacts
.PHONY: fuzz_clean
fuzz_clean:
	@echo "🧹 Cleaning fuzzing artifacts..."
	@rm -rf $(FUZZ_BUILD_DIR)
	@rm -rf $(FUZZ_DIR)/corpora
	@rm -rf $(FUZZ_DIR)/logs
	@rm -rf $(FUZZ_DIR)/coverage
	@echo "✅ Fuzzing artifacts cleaned"

# Generate coverage report
.PHONY: fuzz_report
fuzz_report:
	@if [ ! -d "$(FUZZ_COVERAGE_DIR)" ] || [ -z "$$(ls -A $(FUZZ_COVERAGE_DIR) 2>/dev/null)" ]; then \
		echo "Error: Coverage directory is empty. Please run the fuzzer with ENABLE_COVERAGE flag ON"; \
		exit 1; \
	fi
	@command -v llvm-profdata >/dev/null 2>&1 || { \
		echo "❌ Error: llvm-profdata not found. Please install LLVM tools."; \
		echo "On macOS: brew install llvm"; \
		echo "On Ubuntu: apt install llvm"; \
		exit 1; \
	}
	@if [ -z "$$(ls $(FUZZ_COVERAGE_DIR)/*.profraw 2>/dev/null)" ]; then \
		echo "❌ Error: No .profraw files found in coverage directory."; \
		echo "Please run fuzzer with coverage enabled first."; \
		exit 1; \
	fi
	@llvm-profdata merge -sparse $(FUZZ_COVERAGE_DIR)/*.profraw -o $(FUZZ_COVERAGE_DIR)/coverage.profdata
	@for fuzzer in $(FUZZ_BUILD_DIR)/fuzz-*; do \
		if [ -x "$$fuzzer" ]; then \
			echo "Coverage for $$(basename $$fuzzer):"; \
			llvm-cov report $$fuzzer -instr-profile=$(FUZZ_COVERAGE_DIR)/coverage.profdata; \
		fi; \
	done

# Generate unified HTML coverage report for all fuzzers
.PHONY: fuzz_report_html
fuzz_report_html:
	@# Check coverage directory exists and has data
	@if [ ! -d "$(FUZZ_COVERAGE_DIR)" ] || [ -z "$$(ls -A $(FUZZ_COVERAGE_DIR) 2>/dev/null)" ]; then \
		echo "Error: Coverage directory is empty. Please run the fuzzer with ENABLE_COVERAGE flag ON"; \
		exit 1; \
	fi
	@command -v llvm-profdata >/dev/null 2>&1 || { \
		echo "❌ Error: llvm-profdata not found. Please install LLVM tools."; \
		echo "On macOS: brew install llvm"; \
		echo "On Ubuntu: apt install llvm"; \
		exit 1; \
	}
	@command -v llvm-cov >/dev/null 2>&1 || { \
		echo "❌ Error: llvm-cov not found. Please install LLVM tools."; \
		echo "On macOS: brew install llvm"; \
		echo "On Ubuntu: apt install llvm"; \
		exit 1; \
	}
	@if [ -z "$$(ls $(FUZZ_COVERAGE_DIR)/*.profraw 2>/dev/null)" ]; then \
		echo "❌ Error: No .profraw files found in coverage directory."; \
		echo "Please run fuzzer with coverage enabled first."; \
		exit 1; \
	fi
	@# Merge all profraw files into a single profdata file
	@llvm-profdata merge -sparse $(FUZZ_COVERAGE_DIR)/*.profraw -o $(FUZZ_COVERAGE_DIR)/coverage.profdata
	@echo "Generating unified coverage report for all fuzzers..."
	@rm -rf $(FUZZ_COVERAGE_DIR)/report_html_unified
	@# Generate HTML report with all fuzzer binaries
	@FUZZ_BINARIES=($(FUZZ_BUILD_DIR)/fuzz-*); \
	if [ -e "$${FUZZ_BINARIES[0]}" ]; then \
		FIRST_BINARY="$${FUZZ_BINARIES[0]}"; \
		OBJECT_FLAGS=""; \
		for bin in "$${FUZZ_BINARIES[@]:1}"; do \
			OBJECT_FLAGS="$$OBJECT_FLAGS -object $$bin"; \
		done; \
		llvm-cov show $$FIRST_BINARY $$OBJECT_FLAGS \
			-instr-profile=$(FUZZ_COVERAGE_DIR)/coverage.profdata \
			-format=html \
			-output-dir=$(FUZZ_COVERAGE_DIR)/report_html_unified \
			-show-line-counts-or-regions \
			-show-instantiations \
			-show-expansions; \
	else \
		echo "No fuzz binaries found in $(FUZZ_BUILD_DIR)"; \
		exit 1; \
	fi
	@echo "Unified HTML coverage report generated in $(FUZZ_COVERAGE_DIR)/report_html_unified"
	@# Open report in default browser (cross-platform)
	@open $(FUZZ_COVERAGE_DIR)/report_html_unified/index.html 2>/dev/null || \
		xdg-open $(FUZZ_COVERAGE_DIR)/report_html_unified/index.html 2>/dev/null || \
		echo "Please open manually: $(FUZZ_COVERAGE_DIR)/report_html_unified/index.html"

# === PYTHON FORMATTING TARGETS ===

# Find all Python files in fuzzing directories
PYTHON_FILES := $(shell find $(FUZZING_DIR) -name "*.py" -type f -not -path "*/.*" 2>/dev/null)

# Install Black if not present
.PHONY: format_install
format_install:
	@echo "🔧 Installing Black Python formatter..."
	@command -v black >/dev/null 2>&1 || \
	  { echo "🔧 Installing Black Python formatter…"; \
	    $(PYTHON) -m pip install --user black || { echo "❌ Failed to install Black"; exit 1; }; }
	@echo "✅ Black installed successfully"

# Format all Python files
.PHONY: format
format: format_install
	@echo "🎨 Formatting Python files with Black..."
	@if [ -z "$(PYTHON_FILES)" ]; then \
		echo "⚠️  No Python files found"; \
	elif black --check --quiet --line-length $(BLACK_LINE_LENGTH) $(PYTHON_FILES); then \
		echo "✅ All Python files are already properly formatted - no changes needed"; \
	else \
		echo "⚙️  Applying formatting changes…"; \
		black --line-length $(BLACK_LINE_LENGTH) $(PYTHON_FILES); \
	fi

# Check formatting without making changes
.PHONY: check
check: format_install
	@echo "🔍 Checking Python file formatting..."
	@if [ -z "$(PYTHON_FILES)" ]; then \
		echo "⚠️  No Python files found"; \
	else \
		echo "📁 Checking $(shell echo $(PYTHON_FILES) | wc -w) Python files"; \
		black --check --line-length $(BLACK_LINE_LENGTH) $(PYTHON_FILES) && \
		echo "✅ All Python files are properly formatted" || \
		{ echo "❌ Some files need formatting - run 'make format'"; exit 1; }; \
	fi

# Clean Python cache files
.PHONY: format_clean
format_clean:
	@echo "🧹 Cleaning Python cache files..."
	@find $(FUZZING_DIR) -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	@find $(FUZZING_DIR) -name "*.pyc" -delete 2>/dev/null || true
	@find $(FUZZING_DIR) -name "*.pyo" -delete 2>/dev/null || true
	@echo "✅ Python cache files cleaned"

# List Python files
.PHONY: format_list
format_list:
	@echo "Python files in fuzzing directories:"
	@if [ -z "$(PYTHON_FILES)" ]; then \
		echo "  No Python files found in: $(FUZZING_DIR)"; \
	else \
		for file in $(PYTHON_FILES); do \
			echo "  $$file"; \
		done; \
	fi