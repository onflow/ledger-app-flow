#!/usr/bin/env python3
"""
Common crash analyzer for Zondax Ledger applications.
This script provides a standardized way to analyze fuzzer crashes across different projects.
"""

import os
import sys
import shlex
import subprocess
import time
import argparse
from typing import List, Optional
from fuzz_paths import FuzzPaths


class CrashAnalyzer:
    """Common crash analyzer for fuzzing artifacts"""

    def __init__(
        self,
        fuzz_dir: str,
        timeout: int = 30,
    ):
        self.fuzz_dir = os.path.abspath(fuzz_dir)
        self.timeout = timeout

        # Setup directories within fuzz_dir
        self.build_dir = os.path.join(self.fuzz_dir, FuzzPaths.BUILD_DIR)
        self.logs_dir = os.path.join(self.fuzz_dir, FuzzPaths.LOGS_DIR)

        # Ensure logs directory exists
        os.makedirs(self.logs_dir, exist_ok=True)

    def setup_environment(self) -> dict:
        """Setup environment variables for crash analysis"""
        env = os.environ.copy()

        # ASAN configuration for crash analysis
        env["ASAN_OPTIONS"] = (
            "halt_on_error=1:"
            "print_stacktrace=1:"
            "detect_stack_use_after_return=false:"
            "detect_stack_use_after_scope=true:"
            "symbolize=1:"
            "print_module_map=1:"
            "handle_segv=1:"
            "handle_sigbus=1:"
            "handle_abort=1:"
            "handle_sigfpe=1:"
            "allow_user_segv_handler=0:"
            "use_sigaltstack=1:"
            "detect_odr_violation=0:"
            "fast_unwind_on_malloc=1:"
            "detect_leaks=0:"
            "check_malloc_usable_size=0:"
            "detect_heap_use_after_free=0:"
            "quarantine_size_mb=0"
        )

        # UBSAN configuration for crash analysis
        env["UBSAN_OPTIONS"] = (
            "halt_on_error=1:" "print_stacktrace=1:" "symbolize=1:" "print_summary=1:" "silence_unsigned_overflow=0"
        )

        return env

    def analyze_single_crash(self, fuzzer_name: str, crash_file: str, fuzz_path: str) -> int:
        """Analyze a single crash file with detailed logging"""
        print(f"\n🔍 Analyzing crash: {os.path.basename(crash_file)}")

        # Prepare logging
        log_file = os.path.join(self.logs_dir, f"crash_{fuzzer_name}_{os.path.basename(crash_file)}.log")

        cmd = [fuzz_path, crash_file]
        print(f"Command: {' '.join(shlex.quote(c) for c in cmd)}")

        env = self.setup_environment()

        try:
            # Run with timeout and capture output
            with open(log_file, "w") as log:
                log.write(f"Crash analysis for: {crash_file}\n")
                log.write(f"Command: {' '.join(cmd)}\n")
                log.write(f"Timestamp: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
                log.write("=" * 50 + "\n\n")

                # Add crash input data dump
                try:
                    with open(crash_file, "rb") as crash_data:
                        data = crash_data.read()
                        hex_string = data.hex()
                        log.write(f"CRASH INPUT DATA ({len(data)} bytes):\n")
                        log.write(f"Hex string: {hex_string}\n")
                        log.write("=" * 50 + "\n\n")
                except Exception as e:
                    log.write(f"Error reading crash file: {e}\n\n")

                log.flush()

                result = subprocess.run(cmd, env=env, timeout=self.timeout, capture_output=True, text=True)

                log.write("STDOUT:\n")
                log.write(result.stdout)
                log.write("\nSTDERR:\n")
                log.write(result.stderr)
                log.write(f"\nReturn code: {result.returncode}\n")

            if result.returncode != 0:
                print(f"❌ Crash reproduced! Return code: {result.returncode}")
                print(f"📝 Details saved to: {log_file}")
                return result.returncode
            else:
                print("✅ No crash (return code: 0)")
                return 0

        except subprocess.TimeoutExpired:
            print(f"⏰ Timeout after {self.timeout}s - possible infinite loop")
            with open(log_file, "a") as log:
                log.write(f"\nTIMEOUT after {self.timeout} seconds\n")
            return -1
        except Exception as e:
            print(f"💥 Error running crash: {e}")
            return -1

    def analyze_fuzzer_crashes(self, fuzzer_name: str) -> tuple[int, int]:
        """Analyze all crashes for a specific fuzzer

        Returns:
            tuple: (total_crashes, reproduced_crashes)
        """
        print(f"\n######## Analyzing crashes for {fuzzer_name} ########")

        artifact_dir = os.path.join(self.fuzz_dir, FuzzPaths.CORPORA_DIR, f"{fuzzer_name}-artifacts")
        fuzz_path = os.path.join(self.build_dir, f"fuzz-{fuzzer_name}")

        # Check if directories and binaries exist
        if not os.path.exists(artifact_dir):
            print(f"No artifact directory found: {artifact_dir}")
            return 0, 0

        if not os.path.exists(fuzz_path):
            print(f"Fuzzer binary not found: {fuzz_path}")
            return 0, 0

        # Find crash files
        crash_files = [f for f in os.listdir(artifact_dir) if os.path.isfile(os.path.join(artifact_dir, f))]

        if not crash_files:
            print(f"✅ No crash files found in {artifact_dir}")
            return 0, 0

        print(f"Found {len(crash_files)} crash files")

        # Analyze each crash
        crashes_reproduced = 0
        for crash_file in crash_files:
            crash_path = os.path.join(artifact_dir, crash_file)
            error_code = self.analyze_single_crash(fuzzer_name, crash_path, fuzz_path)

            if error_code != 0:
                crashes_reproduced += 1

        return len(crash_files), crashes_reproduced

    def analyze_all_crashes(self, fuzzer_names: Optional[List[str]] = None) -> bool:
        """Analyze crashes for all or specified fuzzers

        Args:
            fuzzer_names: List of specific fuzzer names to analyze, or None for all

        Returns:
            bool: True if no crashes were found, False if crashes need attention
        """
        total_crashes = 0
        total_reproduced = 0

        # Auto-discover fuzzers if none specified
        if fuzzer_names is None:
            corpora_dir = os.path.join(self.fuzz_dir, FuzzPaths.CORPORA_DIR)
            if os.path.exists(corpora_dir):
                fuzzer_names = []
                for item in os.listdir(corpora_dir):
                    if item.endswith("-artifacts"):
                        fuzzer_name = item[:-10]  # Remove '-artifacts' suffix
                        fuzzer_names.append(fuzzer_name)
            else:
                print("No corpora directory found")
                return True

        # Analyze each fuzzer
        for fuzzer_name in fuzzer_names:
            crashes, reproduced = self.analyze_fuzzer_crashes(fuzzer_name)
            total_crashes += crashes
            total_reproduced += reproduced

            print(f"\n📊 Summary for {fuzzer_name}:")
            print(f"Total crash files: {crashes}")
            print(f"Crashes reproduced: {reproduced}")

        # Overall summary
        print("\n🔍 Overall Analysis Summary:")
        print(f"Total crash files: {total_crashes}")
        print(f"Total crashes reproduced: {total_reproduced}")
        print(f"Logs saved to: {self.logs_dir}")

        if total_reproduced > 0:
            print(f"\n🚨 {total_reproduced} crashes need attention!")
            return False
        else:
            print("\n✅ All crash analysis completed successfully!")
            return True


def main():
    parser = argparse.ArgumentParser(description="Analyze fuzzer crashes for Zondax Ledger applications")
    parser.add_argument("--fuzz-dir", required=True, help="Fuzz directory path (mandatory)")
    parser.add_argument(
        "--timeout", type=int, default=30, help="Timeout for each crash analysis in seconds (default: 30)"
    )
    parser.add_argument("--fuzzers", nargs="*", help="Specific fuzzers to analyze (default: analyze all found fuzzers)")

    args = parser.parse_args()

    analyzer = CrashAnalyzer(args.fuzz_dir, args.timeout)

    if analyzer.analyze_all_crashes(args.fuzzers):
        return 0
    else:
        return 1


if __name__ == "__main__":
    sys.exit(main())
