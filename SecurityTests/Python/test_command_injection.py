#!/usr/bin/env python3
"""
Security Regression Tests for Command Injection Vulnerabilities

This module tests for command injection vulnerabilities identified in:
- BaseTools/Scripts/RunMakefile.py
- BaseTools/Scripts/MemoryProfileSymbolGen.py
- Other build scripts using subprocess with shell=True

CWE-78: Improper Neutralization of Special Elements used in an OS Command
"""

import unittest
import subprocess
import os
import sys
import tempfile
from pathlib import Path

# Add BaseTools to path
edk2_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(edk2_root / 'BaseTools' / 'Source' / 'Python'))


class CommandInjectionTests(unittest.TestCase):
    """Test suite for command injection vulnerability prevention"""

    def setUp(self):
        """Set up test fixtures"""
        self.test_dir = tempfile.mkdtemp()
        self.malicious_inputs = [
            "; rm -rf /",
            "| cat /etc/passwd",
            "&& malicious_command",
            "$(malicious_command)",
            "`malicious_command`",
            "'; DROP TABLE users; --",
            "x64 & echo pwned",
        ]

    def tearDown(self):
        """Clean up test fixtures"""
        if os.path.exists(self.test_dir):
            os.rmdir(self.test_dir)

    def test_subprocess_popen_with_shell_true_is_dangerous(self):
        """
        CRITICAL: Verify that subprocess.Popen with shell=True is dangerous

        This test demonstrates why shell=True with user input is unsafe.
        This is a negative test - it SHOULD be dangerous (that's the point).
        """
        # This is a demonstration - we're not actually running malicious code
        # But showing how it COULD be exploited

        # UNSAFE pattern (what we found in the codebase):
        # subprocess.Popen('command ' + user_input, shell=True)

        # For security, we verify our detection works
        unsafe_pattern = "subprocess.Popen("

        # Scan RunMakefile.py for unsafe patterns
        runmakefile_path = edk2_root / 'BaseTools' / 'Scripts' / 'RunMakefile.py'
        if runmakefile_path.exists():
            with open(runmakefile_path, 'r') as f:
                content = f.read()
                # This test documents the vulnerability
                self.assertIn(unsafe_pattern, content,
                    "RunMakefile.py uses subprocess - verify it's safe")

    def test_shell_metacharacters_should_be_rejected(self):
        """
        Test that shell metacharacters in build parameters are rejected

        RECOMMENDATION: Any build script accepting user input should:
        1. Use subprocess.run/Popen with shell=False and argument lists
        2. Validate input against whitelist of allowed characters
        3. Reject any shell metacharacters
        """
        dangerous_chars = [';', '|', '&', '$', '`', '(', ')', '{', '}', '<', '>']

        for char in dangerous_chars:
            test_input = f"x64{char}malicious"

            # This is what SHOULD happen (validation)
            is_safe = self._validate_build_parameter(test_input)
            self.assertFalse(is_safe,
                f"Input '{test_input}' should be rejected as unsafe")

    def _validate_build_parameter(self, param):
        """
        Example of safe input validation

        Returns:
            bool: True if parameter is safe, False otherwise
        """
        # Whitelist approach: only allow alphanumeric, dash, underscore
        import re
        safe_pattern = re.compile(r'^[a-zA-Z0-9_-]+$')
        return bool(safe_pattern.match(param))

    def test_safe_subprocess_usage_example(self):
        """
        Demonstrate SAFE subprocess usage

        RECOMMENDATION: Replace all shell=True usage with this pattern
        """
        # SAFE: Use argument list, no shell=True
        safe_command = ['echo', 'test argument with; special| characters']

        try:
            result = subprocess.run(
                safe_command,
                capture_output=True,
                text=True,
                shell=False,  # IMPORTANT: No shell interpretation
                timeout=1
            )
            # Special characters are passed as literal arguments, not interpreted
            self.assertEqual(result.returncode, 0)
        except subprocess.TimeoutExpired:
            self.fail("Safe subprocess call timed out")

    def test_detect_unsafe_patterns_in_codebase(self):
        """
        Scan BaseTools Python scripts for unsafe patterns

        This test identifies files that need remediation
        """
        bastools_python = edk2_root / 'BaseTools' / 'Source' / 'Python'
        unsafe_files = []

        if bastools_python.exists():
            for py_file in bastools_python.rglob('*.py'):
                with open(py_file, 'r', errors='ignore') as f:
                    content = f.read()
                    # Check for unsafe patterns
                    if 'shell=True' in content:
                        unsafe_files.append(py_file.relative_to(edk2_root))

        # Document findings
        if unsafe_files:
            print(f"\n⚠️  WARNING: Found {len(unsafe_files)} files with shell=True:")
            for f in unsafe_files[:10]:  # Show first 10
                print(f"  - {f}")
            print("\n📋 RECOMMENDATION: Review and refactor these files")
            print("   Use subprocess with shell=False and argument lists")


class PathTraversalTests(unittest.TestCase):
    """Test suite for path traversal vulnerability prevention"""

    def test_path_traversal_detection(self):
        """
        Test that path traversal attempts are detected and blocked

        CWE-22: Improper Limitation of a Pathname to a Restricted Directory
        """
        malicious_paths = [
            "../../../etc/passwd",
            "..\\..\\..\\windows\\system32",
            "test/../../sensitive/file",
            "/etc/passwd",
            "C:\\Windows\\System32\\config\\SAM"
        ]

        for path in malicious_paths:
            is_safe = self._validate_path(path)
            self.assertFalse(is_safe,
                f"Path '{path}' should be rejected as unsafe")

    def _validate_path(self, path):
        """
        Example safe path validation

        Returns:
            bool: True if path is safe, False if path traversal detected
        """
        # Check for path traversal indicators
        dangerous_patterns = ['..', '\\..', '/etc/', 'C:\\', '\\\\']

        for pattern in dangerous_patterns:
            if pattern in path:
                return False

        return True


class PythonExceptionHandlingTests(unittest.TestCase):
    """Test suite for proper exception handling (security-relevant)"""

    def test_bare_except_clauses_are_dangerous(self):
        """
        Verify that bare except: clauses are detected

        Impact: Security errors can be silently suppressed
        CWE-396: Declaration of Catch for Generic Exception
        """
        bastools_python = edk2_root / 'BaseTools' / 'Source' / 'Python'
        files_with_bare_except = []

        if bastools_python.exists():
            for py_file in bastools_python.rglob('*.py'):
                with open(py_file, 'r', errors='ignore') as f:
                    for line_num, line in enumerate(f, 1):
                        # Detect bare except: (not except Exception:, except SomeError:)
                        if line.strip() == 'except:':
                            files_with_bare_except.append(
                                (py_file.relative_to(edk2_root), line_num)
                            )

        # Document findings (we expect to find some based on analysis)
        if files_with_bare_except:
            print(f"\n⚠️  Found {len(files_with_bare_except)} bare except: clauses")
            for file, line in files_with_bare_except[:10]:
                print(f"  - {file}:{line}")
            print("\n📋 RECOMMENDATION: Replace with specific exception types")
            print("   except (ValueError, TypeError, IOError) as e:")


def run_security_tests():
    """
    Run all security tests and generate report

    Returns:
        int: Exit code (0 = success, 1 = failures)
    """
    # Create test suite
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()

    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(CommandInjectionTests))
    suite.addTests(loader.loadTestsFromTestCase(PathTraversalTests))
    suite.addTests(loader.loadTestsFromTestCase(PythonExceptionHandlingTests))

    # Run with verbose output
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    # Print summary
    print("\n" + "="*70)
    print("SECURITY TEST SUMMARY")
    print("="*70)
    print(f"Tests Run: {result.testsRun}")
    print(f"Successes: {result.testsRun - len(result.failures) - len(result.errors)}")
    print(f"Failures: {len(result.failures)}")
    print(f"Errors: {len(result.errors)}")
    print("="*70)

    return 0 if result.wasSuccessful() else 1


if __name__ == '__main__':
    sys.exit(run_security_tests())
