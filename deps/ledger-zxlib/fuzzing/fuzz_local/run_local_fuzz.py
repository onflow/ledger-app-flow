#!/usr/bin/env python3
"""
Local convenience wrapper for running fuzzers in ledger-zxlib project
"""

import os
import sys

# Get the current directory
current_dir = os.path.dirname(os.path.abspath(__file__))
fuzzing_dir = os.path.dirname(current_dir)  # Parent fuzzing directory

# Add both paths to ensure we can import everything
sys.path.insert(0, current_dir)  # Add fuzz_local to path for fuzz_config
sys.path.insert(0, fuzzing_dir)  # Add fuzzing to path for run_fuzzers

try:
    from run_fuzzers import main
    from fuzz_config import MAX_SECONDS, FUZZER_JOBS

    # Override default arguments to point to this project root
    if "--fuzz-dir" not in sys.argv:
        sys.argv.extend(["--fuzz-dir", current_dir])

    # Override max-seconds if not provided
    if "--max-seconds" not in sys.argv:
        sys.argv.extend(["--max-seconds", str(MAX_SECONDS)])

    # Override jobs if not provided
    if "--jobs" not in sys.argv:
        sys.argv.extend(["--jobs", str(FUZZER_JOBS)])

    # Run the common fuzzer
    sys.exit(main())

except ImportError as e:
    print(f"Error: Cannot import required module: {e}")
    print("Make sure ../run_fuzzers.py and fuzz_config.py exist")
    sys.exit(1)
