#!/usr/bin/env python3
"""
Project-specific fuzzing configuration for ledger-zxlib
"""

import sys
import os

# Configuration constants
MAX_SECONDS = 600  # 10 minutes for quick test
FUZZER_JOBS = 4  # Number of parallel jobs for fuzzing

# Directory configuration (relative to fuzz_local)
FUZZ_DIR = "."  # Current directory (fuzz_local)
FUZZ_BUILD_DIR = "build"  # Build directory relative to fuzz_local
FUZZ_COVERAGE_DIR = "coverage"  # Coverage directory relative to fuzz_local

# Add parent directory to path to import FuzzConfig
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from run_fuzzers import FuzzConfig


def get_fuzzer_configs():
    """Get project-specific fuzzer configurations"""
    return [
        FuzzConfig(name="bech32", max_len=256),  # bech32 fuzzer with appropriate max length
        FuzzConfig(name="base58", max_len=512),  # base58 fuzzer for encoding/decoding
        FuzzConfig(name="base64", max_len=1024),  # base64 fuzzer for encoding
        FuzzConfig(name="hexutils", max_len=512),  # hexutils fuzzer for hex string parsing
        FuzzConfig(name="segwit_addr", max_len=256),  # segwit address fuzzer
        FuzzConfig(name="bignum", max_len=256),  # bignum fuzzer for BCD conversion
        FuzzConfig(name="zxformat", max_len=512),  # zxformat fuzzer for string formatting
        FuzzConfig(name="timeutils", max_len=256),  # timeutils fuzzer for time operations
    ]


if __name__ == "__main__":
    # Show available configurations
    configs = get_fuzzer_configs()
    print("Available fuzzer configurations:")
    for config in configs:
        print(f"  - {config.name}: max_len={config.max_len}")
