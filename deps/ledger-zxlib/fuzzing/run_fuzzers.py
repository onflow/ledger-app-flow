#!/usr/bin/env python3
"""
Common fuzzing runner for Zondax Ledger applications.
This script provides a standardized way to run fuzzers across different projects.

Configuration constants can be adjusted at the top of this file to tune:
- System resource reservation (CPU cores, memory ratio)
- RSS limits per fuzzer job
- Maximum number of parallel jobs
"""

import os
import sys
import random
import shlex
import subprocess
import argparse
import datetime
import time
from typing import List
from fuzz_paths import FuzzPaths

# System resource management constants
SYSTEM_RESERVED_CPU_CORES = 2  # Number of CPU cores reserved for system stability
MAX_FUZZER_MEMORY_PERCENT = 60  # Maximum percentage of total RAM for all fuzzer jobs (60% recommended for stability)
MIN_RSS_LIMIT_MB = 256  # Minimum RSS limit per job (MB)
MAX_RSS_LIMIT_MB = 16384  # Maximum RSS limit per job (MB) - increased for high-RAM systems
MAX_JOBS_LIMIT = 16  # Maximum number of parallel fuzzer jobs


class FuzzConfig:
    """Configuration for a single fuzzer target"""

    def __init__(self, name: str, max_len: int = 17000):
        self.name = name
        self.max_len = max_len


class FuzzRunner:
    """Common fuzzing runner"""

    def __init__(
        self,
        fuzz_dir: str,
        max_seconds: int = 600,
        jobs: int = None,
    ):
        self.fuzz_dir = os.path.abspath(fuzz_dir)
        self.max_seconds = max_seconds
        self.cpu_count = os.cpu_count() or 4
        # Ensure we never get negative jobs and always have at least 1 job
        available_cores = max(1, self.cpu_count - SYSTEM_RESERVED_CPU_CORES)
        self.jobs = jobs or min(MAX_JOBS_LIMIT, available_cores)
        self.mutate_depth = random.randint(1, 20)

        # Create directories within fuzz_dir
        self.build_dir = os.path.join(self.fuzz_dir, FuzzPaths.BUILD_DIR)
        self.coverage_dir = os.path.join(self.fuzz_dir, FuzzPaths.COVERAGE_DIR)
        self.logs_dir = os.path.join(self.fuzz_dir, FuzzPaths.LOGS_DIR)

        # Ensure directories exist
        os.makedirs(self.coverage_dir, exist_ok=True)
        os.makedirs(self.logs_dir, exist_ok=True)

        # System resource detection
        self.total_ram_mb = self._get_total_ram_mb()
        # Calculate total fuzzer memory budget from configured percentage
        self._total_fuzzer_budget_mb = int(self.total_ram_mb * (MAX_FUZZER_MEMORY_PERCENT / 100.0))

        # Calculate optimal RSS limit per job based on actual job count
        self._calculate_optimal_rss_limit()

    def _get_total_ram_mb(self) -> int:
        """Get total system RAM in MB with robust fallbacks"""
        # Try Linux /proc/meminfo first
        try:
            if os.path.exists("/proc/meminfo"):
                with open("/proc/meminfo", "r") as f:
                    for line in f:
                        if line.startswith("MemTotal:"):
                            ram_kb = int(line.split()[1])
                            return max(512, ram_kb // 1024)  # Minimum 512MB
        except (IOError, ValueError, IndexError):
            pass

        # Try psutil as fallback
        try:
            import psutil

            ram_mb = psutil.virtual_memory().total // (1024 * 1024)
            return max(512, ram_mb)  # Minimum 512MB
        except (ImportError, AttributeError):
            pass

        # Try platform-specific methods
        try:
            if sys.platform.startswith("darwin"):  # macOS
                result = subprocess.run(["sysctl", "-n", "hw.memsize"], capture_output=True, text=True, timeout=5)
                if result.returncode == 0:
                    return max(512, int(result.stdout.strip()) // (1024 * 1024))
        except (subprocess.SubprocessError, ValueError):
            pass

        # Final fallback - conservative estimate
        print("⚠️ Warning: Could not detect system RAM, using conservative 2GB estimate")
        return 2048

    def _calculate_optimal_rss_limit(self) -> None:
        """Calculate optimal RSS limit per job based on configured total budget and actual job count"""
        if self.total_ram_mb < MIN_RSS_LIMIT_MB:
            self.optimal_rss_limit = MIN_RSS_LIMIT_MB
            return

        # Calculate per-job limit: total budget divided by actual jobs
        per_job_budget = self._total_fuzzer_budget_mb // self.jobs if self.jobs > 0 else MIN_RSS_LIMIT_MB

        # Apply reasonable limits
        self.optimal_rss_limit = max(MIN_RSS_LIMIT_MB, min(MAX_RSS_LIMIT_MB, per_job_budget))

    def setup_environment(self) -> dict:
        """Setup environment variables for fuzzing"""
        env = os.environ.copy()

        # ASAN configuration optimized for embedded/static allocation apps
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

        # UBSAN configuration
        env["UBSAN_OPTIONS"] = (
            "halt_on_error=1:" "print_stacktrace=1:" "symbolize=1:" "print_summary=1:" "silence_unsigned_overflow=0"
        )

        # Coverage configuration
        env["LLVM_PROFILE_FILE"] = f"{self.coverage_dir}/%p.profraw"

        return env

    def run_fuzzer(self, config: FuzzConfig) -> bool:
        """Run a single fuzzer target"""
        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        print(f"######## Running fuzzer: {config.name} at {timestamp} ########")

        max_time = self.max_seconds

        # Setup paths
        artifact_dir = os.path.join(self.fuzz_dir, FuzzPaths.CORPORA_DIR, f"{config.name}-artifacts")
        corpus_dir = os.path.join(self.fuzz_dir, FuzzPaths.CORPORA_DIR, config.name)
        fuzz_path = os.path.join(self.build_dir, f"fuzz-{config.name}")

        # Create directories
        os.makedirs(artifact_dir, exist_ok=True)
        os.makedirs(corpus_dir, exist_ok=True)

        # Check if fuzzer binary exists
        if not os.path.exists(fuzz_path):
            print(f"ERROR: Fuzzer binary not found: {fuzz_path}")
            return False

        # Setup environment
        env = self.setup_environment()

        # Adaptive timeout based on system performance (more conservative)
        adaptive_timeout = min(60, max(15, (self.total_ram_mb // 500) + 10))

        # Build command with stable optimizations
        cmd = [
            fuzz_path,
            f"-max_total_time={max_time}",
            f"-timeout={adaptive_timeout}",
            f"-rss_limit_mb={self.optimal_rss_limit}",
            f"-jobs={self.jobs}",
            f"-max_len={config.max_len}",
            f"-mutate_depth={self.mutate_depth}",
            f"-artifact_prefix={artifact_dir}/",
            # Core coverage options (stable)
            "-print_pcs=1",
            "-print_funcs=1",
            "-print_final_stats=1",
            # Safe input optimizations
            "-shrink=1",
            "-reduce_inputs=1",
            # Advanced profiling (stable)
            "-use_value_profile=1",
            corpus_dir,
        ]

        print(f"Command: {' '.join(shlex.quote(c) for c in cmd)}")

        # Clean up old generic fuzz-*.log files for this fuzzer
        import glob

        old_logs = glob.glob(os.path.join(self.logs_dir, "fuzz-*.log"))
        for old_log in old_logs:
            try:
                os.remove(old_log)
            except OSError:
                pass  # Ignore if file doesn't exist or can't be removed

        # Run fuzzer from logs directory with robust error handling
        original_cwd = os.getcwd()
        process = None

        try:
            os.chdir(self.logs_dir)

            # Start fuzzer process with proper cleanup handling
            process = subprocess.Popen(cmd, env=env)

            # Wait for completion with timeout (max_time + 60s buffer)
            timeout_seconds = max_time + 60
            try:
                result = process.wait(timeout=timeout_seconds)
            except subprocess.TimeoutExpired:
                print(f"⚠️ Fuzzer {config.name} exceeded timeout ({timeout_seconds}s), terminating...")
                self._terminate_process_gracefully(process)
                return False

            # After execution, rename the generic fuzz-*.log files to job-specific names
            self._rename_job_logs(config.name)

            return result == 0

        except KeyboardInterrupt:
            print(f"\n⚠️ Fuzzer {config.name} interrupted by user")
            if process:
                self._terminate_process_gracefully(process)
            return False

        except Exception as e:
            print(f"❌ Error running fuzzer {config.name}: {e}")
            if process:
                self._terminate_process_gracefully(process)
            return False

        finally:
            os.chdir(original_cwd)

    def _terminate_process_gracefully(self, process: subprocess.Popen):
        """Terminate a process gracefully with timeout"""
        if process.poll() is not None:
            return  # Process already terminated

        try:
            # Try graceful termination first
            process.terminate()
            try:
                process.wait(timeout=10)  # Wait up to 10 seconds
                return  # Successfully terminated
            except subprocess.TimeoutExpired:
                print("⚠️ Process didn't respond to SIGTERM, sending SIGKILL...")

            # Force kill if graceful termination fails
            process.kill()
            try:
                process.wait(timeout=5)  # Wait up to 5 seconds for kill
            except subprocess.TimeoutExpired:
                print("❌ Process didn't respond to SIGKILL - may be zombie")

        except Exception as e:
            print(f"Warning: Failed to terminate process gracefully: {e}")

    def _rename_job_logs(self, fuzzer_name: str):
        """Rename generic fuzz-*.log files to fuzzer-specific job logs"""
        import glob

        # Find all fuzz-*.log files
        fuzz_logs = glob.glob(os.path.join(self.logs_dir, "fuzz-*.log"))

        for fuzz_log in fuzz_logs:
            # Extract job number from filename (e.g., fuzz-0.log -> 0)
            basename = os.path.basename(fuzz_log)
            if basename.startswith("fuzz-") and basename.endswith(".log"):
                job_num = basename[5:-4]  # Remove 'fuzz-' prefix and '.log' suffix

                # Create new filename with fuzzer name
                new_filename = f"{fuzzer_name}_{job_num}.log"
                new_path = os.path.join(self.logs_dir, new_filename)

                try:
                    os.rename(fuzz_log, new_path)
                    print(f"Renamed {basename} -> {new_filename}")
                except OSError as e:
                    print(f"Warning: Could not rename {basename} to {new_filename}: {e}")

    def validate_environment(self) -> bool:
        """Validate environment for fuzzing"""
        print("🔍 Validating environment...")

        # Check available disk space (need at least 1GB)
        try:
            stat = os.statvfs(self.fuzz_dir)
            free_space_gb = (stat.f_bavail * stat.f_frsize) / (1024**3)
            if free_space_gb < 1.0:
                print(f"❌ Insufficient disk space: {free_space_gb:.1f}GB available, need at least 1GB")
                return False
            print(f"✅ Disk space: {free_space_gb:.1f}GB available")
        except (OSError, AttributeError):
            print("⚠️ Could not check disk space")

        # System resources detected
        print("🖥️ System resources detected:")
        print(f"   • Total RAM: {self.total_ram_mb}MB")
        print(f"   • CPU cores: {self.cpu_count}")

        # Resource allocation after system integrity limits
        total_fuzzer_memory = self.jobs * self.optimal_rss_limit
        system_memory_reserved = self.total_ram_mb - total_fuzzer_memory

        # Critical check: ensure we don't exceed system memory
        if system_memory_reserved < 512:  # Need at least 512MB for system
            print(
                f"❌ CRITICAL: Fuzzer memory allocation ({total_fuzzer_memory}MB) would leave only {system_memory_reserved}MB for system"
            )
            print(f"   Reducing jobs from {self.jobs} to prevent system instability")
            # Calculate safe number of jobs
            safe_fuzzer_memory = self.total_ram_mb - 1024  # Leave 1GB for system
            safe_jobs = max(1, safe_fuzzer_memory // self.optimal_rss_limit)
            print(f"   Recommended: reduce FUZZER_JOBS to {safe_jobs}")
            if system_memory_reserved < 0:
                return False  # Abort if we would exceed total RAM

        # Calculate available vs used cores
        available_cores = self.cpu_count - SYSTEM_RESERVED_CPU_CORES
        unused_cores = available_cores - self.jobs

        print("🎯 Fuzzer resource allocation:")
        print(f"   • Jobs configured: {self.jobs}")
        print(
            f"   • Available cores: {available_cores} (total: {self.cpu_count}, system reserved: {SYSTEM_RESERVED_CPU_CORES})"
        )
        if unused_cores > 0:
            print(f"   • Unused cores: {unused_cores} (consider increasing jobs for better performance)")
        # Calculate actual percentages and budgets
        actual_fuzzer_percent = (total_fuzzer_memory / self.total_ram_mb) * 100
        budget_percent = MAX_FUZZER_MEMORY_PERCENT
        budget_mb = self._total_fuzzer_budget_mb

        print(
            f"   • Memory per job: {self.optimal_rss_limit}MB (calculated from {budget_mb}MB total budget ÷ {self.jobs} jobs)"
        )
        print(f"   • Total fuzzer memory: {total_fuzzer_memory}MB ({actual_fuzzer_percent:.1f}% of total RAM)")
        print(f"   • Configured budget: {budget_mb}MB ({budget_percent}% of total RAM)")
        if abs(total_fuzzer_memory - budget_mb) > 1:
            print(
                f"   ⚠️  Actual allocation ({total_fuzzer_memory}MB) differs from budget ({budget_mb}MB) due to per-job limits"
            )
        print(
            f"   • System memory reserved: {system_memory_reserved}MB ({(system_memory_reserved/self.total_ram_mb)*100:.1f}% of total RAM)"
        )

        # Show the actual calculations used
        optimal_jobs = min(MAX_JOBS_LIMIT, max(1, self.cpu_count - SYSTEM_RESERVED_CPU_CORES))

        print("📊 Resource calculation details:")
        print(f"   • Total fuzzer budget: {self.total_ram_mb}MB × {MAX_FUZZER_MEMORY_PERCENT}% = {budget_mb}MB")
        print(f"   • Configured jobs: {self.jobs}")
        print(
            f"   • Per-job calculation: {budget_mb}MB ÷ {self.jobs} = {budget_mb // self.jobs if self.jobs > 0 else 0}MB"
        )
        print(
            f"   • Applied limits: min({MAX_RSS_LIMIT_MB}MB, max({MIN_RSS_LIMIT_MB}MB, calculated)) = {self.optimal_rss_limit}MB"
        )
        print(
            f"   • Optimal jobs calculation: min({MAX_JOBS_LIMIT}, max(1, {self.cpu_count} - {SYSTEM_RESERVED_CPU_CORES})) = {optimal_jobs}"
        )
        if self.jobs != optimal_jobs:
            print(f"   • Jobs override: configured={self.jobs}, optimal={optimal_jobs}")

        # Warnings and suggestions for resource optimization
        if self.total_ram_mb < 2048:
            print("⚠️ Low system RAM detected, fuzzing may be slower")

        if self.cpu_count < 2:
            print("⚠️ Low CPU cores detected, consider increasing --jobs if system allows")

        if total_fuzzer_memory > (self.total_ram_mb * 0.75):
            print(f"❌ CRITICAL: High memory usage: {(total_fuzzer_memory/self.total_ram_mb)*100:.1f}% of total RAM")
            print("   Aborting to prevent system instability")
            print("   Please reduce FUZZER_JOBS or MAX_FUZZER_MEMORY_PERCENT")
            return False

        if unused_cores >= 3:
            efficiency_loss = (unused_cores / available_cores) * 100
            print(f"💡 Performance suggestion: {unused_cores} cores unused ({efficiency_loss:.1f}% of available cores)")
            print(f"   Consider increasing FUZZER_JOBS to {min(available_cores, 8)} for better performance")

        print("✅ ASAN configured for embedded apps: heap detection disabled, stack bounds enabled")
        return True

    def run_fuzzers(self, configs: List[FuzzConfig]) -> bool:
        """Run multiple fuzzer targets"""
        # Validate environment first
        if not self.validate_environment():
            print("❌ Environment validation failed")
            return False

        success = True

        print(f"🚀 Starting fuzzing session with {len(configs)} fuzzers...")
        print(f"⚙️ Configuration: {self.jobs} jobs, {self.max_seconds}s per fuzzer")

        start_time = time.time()

        for i, config in enumerate(configs, 1):
            print(f"\n📊 Running fuzzer {i}/{len(configs)}: {config.name}")
            if not self.run_fuzzer(config):
                success = False
                print(f"❌ Fuzzer {config.name} failed or found issues")
            else:
                print(f"✅ Fuzzer {config.name} completed successfully")

        total_time = time.time() - start_time
        print(f"\n🏁 Fuzzing session completed in {total_time:.1f}s")
        print(f"📈 Overall result: {'SUCCESS' if success else 'FAILURE'}")

        return success


def main():
    parser = argparse.ArgumentParser(description="Run fuzzers for Zondax Ledger applications")
    parser.add_argument("--fuzz-dir", required=True, help="Fuzz directory path (mandatory)")
    parser.add_argument("--max-seconds", type=int, default=600, help="Maximum seconds per fuzzer run (default: 600)")
    parser.add_argument("--jobs", type=int, help="Number of parallel jobs (default: from fuzz_config.py or 16)")
    parser.add_argument("--fuzzers", nargs="*", help="Specific fuzzers to run (default: run all configured fuzzers)")
    parser.add_argument("--config-dir", help="Directory containing fuzz_config.py (default: fuzz_dir)")

    args = parser.parse_args()

    # Check if project has its own fuzzer configuration
    config_dir = args.config_dir or args.fuzz_dir
    project_fuzz_config = os.path.join(config_dir, "fuzz_config.py")
    fuzzer_jobs = 16  # Default value

    if os.path.exists(project_fuzz_config):
        # Load project-specific configuration
        sys.path.insert(0, config_dir)
        try:
            import fuzz_config

            if hasattr(fuzz_config, "get_fuzzer_configs"):
                configs = fuzz_config.get_fuzzer_configs()
            if hasattr(fuzz_config, "FUZZER_JOBS"):
                fuzzer_jobs = fuzz_config.FUZZER_JOBS
        except ImportError as e:
            print(f"Warning: Could not load fuzz_config.py: {e}")
            print("Using default configuration")

    # Filter fuzzers if specific ones were requested
    if args.fuzzers:
        configs = [c for c in configs if c.name in args.fuzzers]
        if not configs:
            print(f"Error: No matching fuzzers found for: {args.fuzzers}")
            return 1

    # Use command line jobs argument if provided, otherwise use config value
    final_jobs = args.jobs if args.jobs is not None else fuzzer_jobs
    runner = FuzzRunner(args.fuzz_dir, args.max_seconds, final_jobs)

    if runner.run_fuzzers(configs):
        print("All fuzzers completed successfully!")
        return 0
    else:
        print("Some fuzzers failed or found issues!")
        return 1


if __name__ == "__main__":
    sys.exit(main())
