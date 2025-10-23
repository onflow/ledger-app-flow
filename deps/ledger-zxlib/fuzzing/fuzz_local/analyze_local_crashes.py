#!/usr/bin/env python3
"""
Local convenience wrapper for analyzing crashes in ledger-zxlib project
"""

import os
import sys

# Get the current directory
current_dir = os.path.dirname(os.path.abspath(__file__))
fuzzing_dir = os.path.dirname(current_dir)  # Parent fuzzing directory

# Add paths to ensure we can import everything
sys.path.insert(0, current_dir)  # Add fuzz_local to path for fuzz_config
sys.path.insert(0, fuzzing_dir)  # Add fuzzing to path for analyze_crashes

try:
    # Import configuration from fuzz_config
    from fuzz_config import FUZZ_BUILD_DIR

    # Check if analyzer script exists
    analyzer_script = os.path.join(fuzzing_dir, "analyze_crashes.py")
    if not os.path.exists(analyzer_script):
        print(f"Error: Analyzer script not found at {analyzer_script}")
        sys.exit(1)

    # Override default arguments to point to this project root
    if "--fuzz-dir" not in sys.argv:
        sys.argv.extend(["--fuzz-dir", current_dir])

    # Import and run the common crash analyzer
    from analyze_crashes import main

    print("🔍 Analyzing ledger-zxlib fuzzer crashes...")
    print(f"Working directory: {current_dir}")
    print(f"Build directory: {os.path.join(current_dir, FUZZ_BUILD_DIR)}")

    sys.exit(main())

except ImportError as e:
    print(f"Error: Cannot import required module: {e}")
    print("Make sure ../analyze_crashes.py and fuzz_config.py exist")
    sys.exit(1)
