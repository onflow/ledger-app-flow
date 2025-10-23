#!/usr/bin/env python3
"""
Centralized path configuration for fuzzing infrastructure.
This module provides consistent path definitions used across all fuzzing tools.
"""


class FuzzPaths:
    """Centralized path configuration for fuzzing"""

    FUZZ_DIR = "fuzz"
    BUILD_DIR = "build"
    LOGS_DIR = "logs"
    CORPORA_DIR = "corpora"
    COVERAGE_DIR = "coverage"

    @classmethod
    def get_default_build_path(cls):
        """Get the default build path relative to fuzz directory"""
        return f"{cls.FUZZ_DIR}/{cls.BUILD_DIR}"

    @classmethod
    def get_logs_path(cls):
        """Get the default logs path relative to fuzz directory"""
        return f"{cls.FUZZ_DIR}/{cls.LOGS_DIR}"

    @classmethod
    def get_coverage_path(cls):
        """Get the default coverage path relative to fuzz directory"""
        return f"{cls.FUZZ_DIR}/{cls.COVERAGE_DIR}"

    @classmethod
    def get_corpora_path(cls):
        """Get the default corpora path relative to fuzz directory"""
        return f"{cls.FUZZ_DIR}/{cls.CORPORA_DIR}"
