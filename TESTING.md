# EDK2 Comprehensive Testing Guide

This document provides instructions for running the comprehensive test suite for EDK2.

## Quick Start

```bash
# Run all tests
python stuart_ci_build -c .pytool/CISettings.py

# Run security tests only
cd SecurityTests/Python && python test_command_injection.py

# Run specific package tests
python stuart_build -c .pytool/CISettings.py -p MdePkg/Test/MdePkgHostTest.dsc
```

---

## Table of Contents

1. [Test Types](#test-types)
2. [Prerequisites](#prerequisites)
3. [Running Tests](#running-tests)
4. [Coverage Reports](#coverage-reports)
5. [CI/CD Integration](#cicd-integration)
6. [Adding New Tests](#adding-new-tests)

---

## Test Types

### 1. Security Regression Tests

Location: `SecurityTests/`

These tests validate that identified security vulnerabilities are fixed and cannot regress:

- **Command Injection Tests** - Validates Python build script safety
- **Buffer Overflow Tests** - Validates network stack memory safety
- **Cryptographic Tests** - Validates crypto implementations

```bash
# Run Python security tests
cd SecurityTests/Python
python test_command_injection.py -v

# Expected output:
# test_subprocess_popen_with_shell_true_is_dangerous ... ok
# test_shell_metacharacters_should_be_rejected ... ok
# ...
# Ran 8 tests in 0.123s
# OK
```

### 2. Host-Based Unit Tests

Location: `<Package>/Test/`

Unit tests run on the host system (Linux/Windows) without UEFI environment:

**Supported Packages:**
- MdePkg
- MdeModulePkg
- SecurityPkg
- CryptoPkg
- NetworkPkg
- UefiCpuPkg
- ShellPkg

```bash
# Build and run all unit tests
python stuart_ci_build -c .pytool/CISettings.py

# Build specific package tests
python stuart_build -c .pytool/CISettings.py \
  -p MdePkg/Test/MdePkgHostTest.dsc

# Run tests manually
Build/MdePkg/HostTest/DEBUG_GCC5/X64/TestBaseSafeIntLib
```

### 3. Integration Tests

Integration tests validate interactions between modules (future expansion).

### 4. Platform Tests

Location: `EmulatorPkg/`, `OvmfPkg/`

Tests that run full UEFI boot in emulator or QEMU:

```bash
# Build OVMF
python stuart_build -c .pytool/CISettings.py \
  -p OvmfPkg/OvmfPkgX64.dsc

# Run in QEMU
qemu-system-x86_64 \
  -bios Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd \
  -serial stdio \
  -display none
```

---

## Prerequisites

### All Platforms

```bash
# Python 3.11+
python --version

# Install Python dependencies
pip install -r pip-requirements.txt
```

### Linux

```bash
# Ubuntu/Debian
sudo apt-get install gcc g++ make uuid-dev nasm iasl lcov

# Fedora/RHEL
sudo dnf install gcc gcc-c++ make libuuid-devel nasm acpica-tools lcov
```

### Windows

- Visual Studio 2022 (Community Edition or higher)
- NASM: https://www.nasm.us/
- Python 3.11+

---

## Running Tests

### Method 1: Stuart CI Build (Recommended)

This is the official EDK2 CI method:

```bash
# Setup (first time only)
python stuart_setup -c .pytool/CISettings.py

# Update dependencies
python stuart_update -c .pytool/CISettings.py

# Run all CI tests
python stuart_ci_build -c .pytool/CISettings.py
```

### Method 2: Package-Specific Tests

Build and run tests for a specific package:

```bash
# Example: Test MdePkg
python stuart_build -c .pytool/CISettings.py \
  TOOL_CHAIN_TAG=GCC5 \
  TARGET_ARCH=X64 \
  -p MdePkg/Test/MdePkgHostTest.dsc

# Find and run test executables
find Build/MdePkg -name "*Test" -type f -executable -exec {} \;
```

### Method 3: Manual Build

```bash
# Source the build environment
. edksetup.sh

# Build a test DSC
build -a X64 -t GCC5 -p MdePkg/Test/MdePkgHostTest.dsc

# Run tests
Build/MdePkg/HostTest/DEBUG_GCC5/X64/TestBaseSafeIntLib
```

### Method 4: Security Tests Only

```bash
# Python security tests
cd SecurityTests/Python
python test_command_injection.py -v

# Run with pytest
pip install pytest
pytest -v

# Static security analysis
pip install bandit
bandit -r BaseTools/Source/Python/ -ll
```

---

## Coverage Reports

### Generate Coverage Report

```bash
# Build with coverage enabled
python stuart_ci_build -c .pytool/CISettings.py --coverage

# Generate LCOV coverage
lcov --capture --directory Build --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/googletest/*' --output-file coverage.info

# Generate HTML report
genhtml coverage.info --output-directory coverage_html

# View report
firefox coverage_html/index.html  # Linux
start coverage_html/index.html    # Windows
```

### Check Coverage Threshold

```bash
# Get coverage percentage
lcov --summary coverage.info

# Example output:
# Overall coverage rate:
#   lines......: 78.3% (12543 of 16012 lines)
#   functions..: 82.1% (2134 of 2599 functions)
```

### Coverage Goals

| Package | Target | Current* |
|---------|--------|----------|
| MdePkg | 85% | ~45% |
| SecurityPkg | 80% | ~30% |
| CryptoPkg | 80% | ~40% |
| MdeModulePkg | 80% | ~25% |
| NetworkPkg | 75% | ~20% |
| **Overall** | **80%** | **~35%** |

\* Baseline estimates - see `test_coverage.md` for details

---

## CI/CD Integration

### GitHub Actions

The comprehensive testing workflow runs automatically on:
- Push to `master`, `stable/*`, `claude/*` branches
- Pull requests to `master`
- Manual workflow dispatch

**Workflow:** `.github/workflows/comprehensive-testing.yml`

**Jobs:**
1. **Security Tests** - Python vulnerability tests + Bandit
2. **Unit Tests** - Host-based tests with coverage
3. **Code Quality** - Pylint, Uncrustify, encoding checks
4. **Platform Tests** - OVMF QEMU boot tests
5. **CodeQL Analysis** - Static security analysis

### Viewing Results

1. Go to GitHub Actions tab
2. Select workflow run
3. View job results and artifacts
4. Download coverage reports

---

## Adding New Tests

### Adding a Unit Test (GoogleTest)

1. **Create test file:**

```cpp
// MdePkg/Test/GoogleTest/Library/MyLib/MyLibTest.cpp
#include <gtest/gtest.h>
extern "C" {
  #include <Library/MyLib.h>
}

TEST(MyLibTest, MyFunction_ValidInput_ReturnsSuccess) {
  EXPECT_EQ(MyFunction(10), 20);
}
```

2. **Add to DSC file:**

```ini
# MdePkg/Test/MdePkgHostTest.dsc
[Components]
  MdePkg/Test/GoogleTest/Library/MyLib/MyLibHostTest.inf
```

3. **Create INF file:**

```ini
# MdePkg/Test/GoogleTest/Library/MyLib/MyLibHostTest.inf
[Defines]
  INF_VERSION    = 0x00010005
  BASE_NAME      = MyLibHostTest
  FILE_GUID      = {generate-new-guid}
  MODULE_TYPE    = HOST_APPLICATION
  VERSION_STRING = 1.0

[Sources]
  MyLibTest.cpp

[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[LibraryClasses]
  GoogleTestLib
  MyLib
```

4. **Run the test:**

```bash
python stuart_build -c .pytool/CISettings.py \
  -p MdePkg/Test/MdePkgHostTest.dsc
```

### Adding a Security Test

1. **Add to `SecurityTests/Python/test_command_injection.py`:**

```python
def test_my_security_check(self):
    """Test description"""
    malicious_input = "; rm -rf /"
    result = validate_input(malicious_input)
    self.assertFalse(result, "Malicious input should be rejected")
```

2. **Run security tests:**

```bash
cd SecurityTests/Python
python test_command_injection.py -v
```

---

## Test Naming Conventions

**GoogleTest:**
```cpp
TEST(ComponentTest, Function_Condition_ExpectedResult)
```

Examples:
- `SafeInt32Add_ValidInputs_ReturnsSuccess`
- `SafeInt32Add_Overflow_ReturnsError`
- `BufferParse_NoNullTerminator_ReturnsInvalidParameter`

**Python:**
```python
def test_function_condition_expected_result(self):
```

Examples:
- `test_subprocess_rejects_shell_metacharacters`
- `test_path_validation_blocks_directory_traversal`

---

## Debugging Test Failures

### GoogleTest Failures

```bash
# Run single test
./Build/.../MyTest --gtest_filter="MyLibTest.MyFunction_*"

# Run with verbose output
./Build/.../MyTest --gtest_verbose=1

# Run under GDB
gdb --args ./Build/.../MyTest
```

### Python Test Failures

```bash
# Run with verbose output
python -m pytest -v -s

# Run single test
python -m pytest test_command_injection.py::test_specific_test -v

# Debug with pdb
python -m pytest --pdb
```

---

## Common Issues

### Issue: Tests not found

**Solution:** Ensure test DSC file includes your test in `[Components]` section

### Issue: Coverage below threshold

**Solution:** Add more test cases to cover uncovered lines. Use coverage report to identify gaps:

```bash
genhtml coverage.info -o coverage_html
# Open coverage_html/index.html to see uncovered lines
```

### Issue: Security test fails

**Solution:** This indicates a potential security vulnerability. Review:
1. `security_analysis.md` for identified issues
2. The specific test failure message
3. Fix the underlying code, don't modify the test

---

## Documentation

- **Testing Strategy:** `testing_strategy.md` - Comprehensive testing approach
- **Security Analysis:** `security_analysis.md` - Identified vulnerabilities
- **Code Quality:** `code_quality.md` - Code quality issues
- **Test Coverage:** `test_coverage.md` - Current coverage and goals
- **UnitTestFrameworkPkg:** `UnitTestFrameworkPkg/ReadMe.md` - Framework documentation

---

## Getting Help

- **Issues:** https://github.com/tianocore/edk2/issues
- **Discussions:** https://github.com/tianocore/edk2/discussions
- **Mailing List:** https://edk2.groups.io/g/devel
- **Documentation:** https://github.com/tianocore/tianocore.github.io/wiki

---

## Success Criteria

✅ **All tests passing**
✅ **Coverage ≥ 80%**
✅ **No security vulnerabilities**
✅ **Code quality checks pass**
✅ **CI/CD green**

---

**Last Updated:** 2025-10-21
**Maintained By:** EDK2 Quality Assurance Team
