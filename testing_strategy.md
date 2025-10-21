# EDK2 Comprehensive Testing Strategy

**Document Version:** 1.0
**Date:** 2025-10-21
**Status:** Active
**Applicable To:** EDK2 Repository - All Packages

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Testing Framework Overview](#2-testing-framework-overview)
3. [Test Types and Coverage](#3-test-types-and-coverage)
4. [Host-Based Unit Testing](#4-host-based-unit-testing)
5. [Integration Testing](#5-integration-testing)
6. [Security Testing](#6-security-testing)
7. [Platform Testing](#7-platform-testing)
8. [Test Implementation Plan](#8-test-implementation-plan)
9. [CI/CD Integration](#9-cicd-integration)
10. [Coverage Goals and Metrics](#10-coverage-goals-and-metrics)

---

## 1. Executive Summary

### Purpose

This document defines a comprehensive testing strategy for the EDK2 UEFI firmware development environment, addressing:
- Security vulnerabilities identified in security analysis
- Code quality issues requiring validation
- Functional correctness across all packages
- Platform compatibility verification

### Testing Approach

**EDK2 is UEFI firmware, NOT a web application.** Therefore, our testing strategy focuses on:
- **Host-based unit tests** using GoogleTest and CMocka
- **Integration tests** for module interactions
- **Security-focused tests** for vulnerability validation
- **Platform tests** using EmulatorPkg and OvmfPkg
- **NO Playwright/browser testing** (not applicable to firmware)

### Goals

1. **80% code coverage** minimum across all packages
2. **100% security vulnerability coverage** with regression tests
3. **Zero tolerance** for test failures in CI/CD
4. **Automated testing** integrated into GitHub Actions

---

## 2. Testing Framework Overview

### 2.1 Existing Infrastructure

EDK2 provides robust testing infrastructure:

#### UnitTestFrameworkPkg

**Framework (C-based):**
- Host-based and target-based testing
- CMocka for mocking
- JUNIT XML output
- Supports PEI, DXE, SMM, UEFI Shell

**GoogleTest (C++-based):**
- Host-based only
- gMock for mocking
- Rich assertion library
- VS Code integration (C++ TestMate)
- Recommended for new tests

### 2.2 Test Execution Environments

1. **Host Environment** (Linux/Windows)
   - Fastest execution
   - Best for unit tests
   - Requires host-specific libraries

2. **Emulator Environment** (EmulatorPkg)
   - Simulates UEFI boot
   - Tests platform interactions
   - IA32, X64 support

3. **Virtual Machine Environment** (OvmfPkg)
   - QEMU-based testing
   - Full UEFI environment
   - Multi-architecture (IA32, X64, AARCH64)

4. **Target Hardware** (Platform-specific)
   - Final validation
   - Performance testing
   - Hardware-specific features

---

## 3. Test Types and Coverage

### 3.1 Test Pyramid

```
         /\
        /  \  E2E Platform Tests (5%)
       /    \
      /------\  Integration Tests (15%)
     /        \
    /----------\  Unit Tests (80%)
   /______________\
```

**Distribution:**
- **80% Unit Tests:** Fast, isolated, comprehensive
- **15% Integration Tests:** Module interactions
- **5% End-to-End Tests:** Full platform validation

### 3.2 Test Categories

| Category | Framework | Environment | Coverage Target |
|----------|-----------|-------------|-----------------|
| Unit Tests | GoogleTest | Host | 80% |
| Integration Tests | GoogleTest | Host/Emulator | Critical paths |
| Security Tests | GoogleTest | Host | 100% vulns |
| Platform Tests | Framework | Emulator/QEMU | Boot scenarios |
| Regression Tests | Both | All | Known bugs |

---

## 4. Host-Based Unit Testing

### 4.1 Scope

Unit tests validate individual functions and modules in isolation.

**Target Packages:**
- MdePkg (base libraries)
- MdeModulePkg (core modules)
- NetworkPkg (network stack)
- SecurityPkg (security features)
- CryptoPkg (cryptography)

### 4.2 GoogleTest Implementation

#### Test Structure

```cpp
// File: MdePkg/Test/GoogleTest/Library/BaseLib/SafeIntLibTest.cpp

#include <gtest/gtest.h>
extern "C" {
  #include <Library/SafeIntLib.h>
}

class SafeIntLibTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Test setup
  }

  void TearDown() override {
    // Test cleanup
  }
};

TEST_F(SafeIntLibTest, SafeInt32Add_ValidInputs_ReturnsSuccess) {
  INT32 Result;
  RETURN_STATUS Status;

  Status = SafeInt32Add(100, 200, &Result);

  EXPECT_EQ(Status, RETURN_SUCCESS);
  EXPECT_EQ(Result, 300);
}

TEST_F(SafeIntLibTest, SafeInt32Add_Overflow_ReturnsError) {
  INT32 Result;
  RETURN_STATUS Status;

  Status = SafeInt32Add(INT32_MAX, 1, &Result);

  EXPECT_EQ(Status, RETURN_BUFFER_TOO_SMALL);
}
```

### 4.3 Test Coverage Requirements

#### Minimum Coverage per Function

1. **Happy Path:** Valid inputs, expected outputs
2. **Boundary Conditions:**
   - Minimum values
   - Maximum values
   - Zero values
   - NULL pointers
3. **Error Conditions:**
   - Invalid parameters
   - Out-of-resources
   - Buffer overflows
4. **Edge Cases:**
   - Empty inputs
   - Large inputs
   - Concurrent access (if applicable)

### 4.4 Mocking Strategy

Use gMock for external dependencies:

```cpp
// Mock UEFI Runtime Services
class MockUefiRuntimeServices {
 public:
  MOCK_METHOD(EFI_STATUS, GetVariable,
    (CHAR16* VariableName, EFI_GUID* VendorGuid,
     UINT32* Attributes, UINTN* DataSize, VOID* Data));
};
```

---

## 5. Integration Testing

### 5.1 Scope

Integration tests validate interactions between modules:
- Protocol producer/consumer relationships
- Library dependencies
- Event handling
- Memory management across modules

### 5.2 Test Scenarios

#### Example: Variable Service Integration

```cpp
TEST(VariableServiceIntegrationTest, SetAndGetVariable_ValidData_Success) {
  // Setup: Initialize variable service
  InitializeVariableService();

  // Test: Set a variable
  EFI_STATUS Status;
  UINT32 Data = 0x12345678;
  Status = SetVariable(L"TestVar", &gTestGuid,
                       EFI_VARIABLE_BOOTSERVICE_ACCESS,
                       sizeof(Data), &Data);
  ASSERT_EQ(Status, EFI_SUCCESS);

  // Verify: Get the variable back
  UINT32 ReadData;
  UINTN DataSize = sizeof(ReadData);
  Status = GetVariable(L"TestVar", &gTestGuid, NULL, &DataSize, &ReadData);

  EXPECT_EQ(Status, EFI_SUCCESS);
  EXPECT_EQ(ReadData, Data);
  EXPECT_EQ(DataSize, sizeof(Data));
}
```

### 5.3 Integration Test Targets

1. **HII Database Integration**
   - Config routing
   - String database
   - Font database

2. **Network Stack Integration**
   - TCP/IP stack layers
   - DHCP + DNS integration
   - TLS + TCP integration

3. **Security Integration**
   - SecureBoot + Variable services
   - TPM + Measured boot
   - Crypto + Network (TLS)

---

## 6. Security Testing

### 6.1 Vulnerability Regression Tests

Every identified security vulnerability MUST have a corresponding test.

#### 6.1.1 Command Injection Tests

**Target:** Python build scripts (15 vulnerabilities)

```python
# File: BaseTools/Test/SecurityTests/test_command_injection.py

import unittest
import subprocess
from BaseTools.Scripts import RunMakefile

class CommandInjectionTests(unittest.TestCase):
    def test_makefile_runner_sanitizes_arch_parameter(self):
        """Verify that shell metacharacters in ARCH are sanitized"""
        malicious_arch = "x64; rm -rf /"

        # Should NOT execute rm -rf
        with self.assertRaises(ValueError):
            RunMakefile.run_with_args(arch=malicious_arch)

    def test_makefile_runner_rejects_pipe_characters(self):
        """Verify that pipe characters are rejected"""
        malicious_input = "x64 | cat /etc/passwd"

        with self.assertRaises(ValueError):
            RunMakefile.run_with_args(arch=malicious_input)
```

#### 6.1.2 Memory Safety Tests

**Target:** Buffer overflows, integer overflows

```cpp
// File: NetworkPkg/Test/GoogleTest/Mtftp4OptionTest.cpp

TEST(Mtftp4OptionTest, ParseOptions_NoNullTerminator_ReturnsError) {
  // Test for vulnerability in Mtftp4Option.c:180-205
  UINT8 MaliciousPacket[] = {
    0x00, 0x01,  // Opcode
    'f', 'i', 'l', 'e',  // No null terminator - ATTACK
    'b', 'l', 'k', 's', 'i', 'z', 'e'
  };

  EFI_MTFTP4_OPTION Options[10];
  UINT32 OptionCount = 10;

  EFI_STATUS Status = Mtftp4ParseOptions(
    (EFI_MTFTP4_PACKET*)MaliciousPacket,
    sizeof(MaliciousPacket),
    &OptionCount,
    Options
  );

  // Should detect invalid packet and return error
  EXPECT_EQ(Status, EFI_INVALID_PARAMETER);
  // Should NOT crash or read out of bounds
}

TEST(Mtftp4OptionTest, ParseOptions_ValidPacket_Success) {
  UINT8 ValidPacket[] = {
    0x00, 0x01,  // Opcode
    'f', 'i', 'l', 'e', 0x00,  // Filename (null-terminated)
    'b', 'l', 'k', 's', 'i', 'z', 'e', 0x00,  // Option name
    '5', '1', '2', 0x00  // Option value
  };

  EFI_MTFTP4_OPTION Options[10];
  UINT32 OptionCount = 10;

  EFI_STATUS Status = Mtftp4ParseOptions(
    (EFI_MTFTP4_PACKET*)ValidPacket,
    sizeof(ValidPacket),
    &OptionCount,
    Options
  );

  EXPECT_EQ(Status, EFI_SUCCESS);
  EXPECT_EQ(OptionCount, 1);
  EXPECT_STREQ((char*)Options[0].OptionStr, "blksize");
  EXPECT_STREQ((char*)Options[0].ValueStr, "512");
}
```

#### 6.1.3 Integer Overflow Tests

**Target:** AuthService.c integer underflow (CRITICAL)

```cpp
TEST(AuthServiceTest, VerifySignatureList_IntegerUnderflow_ReturnsError) {
  // Craft malicious signature list with underflow condition
  EFI_SIGNATURE_LIST MaliciousList;
  MaliciousList.SignatureListSize = sizeof(EFI_SIGNATURE_LIST) + 10;
  MaliciousList.SignatureHeaderSize = 1000;  // > (ListSize - sizeof)
  MaliciousList.SignatureSize = 100;

  // This would cause: (10 - 1000) underflow → large value
  EFI_STATUS Status = VerifySignatureList(&MaliciousList, ...);

  // MUST detect and reject
  EXPECT_EQ(Status, EFI_INVALID_PARAMETER);
}
```

#### 6.1.4 Cryptographic Tests

**Target:** Weak crypto, hardcoded seeds

```cpp
TEST(CryptRandTest, RandomSeed_NotHardcoded_Verified) {
  // Verify PRNG is not using hardcoded seed
  UINT8 Random1[32];
  UINT8 Random2[32];

  // Reset and reseed
  RandomSeed(NULL, 0);
  RandomBytes(Random1, sizeof(Random1));

  // Reset and reseed again
  RandomSeed(NULL, 0);
  RandomBytes(Random2, sizeof(Random2));

  // Two separate runs should produce different output
  // (not reproducible with hardcoded seed)
  EXPECT_NE(0, memcmp(Random1, Random2, sizeof(Random1)))
    << "PRNG appears to use hardcoded seed";
}

TEST(CryptX509Test, TlsMinimumVersion_IsTls12OrHigher) {
  // Verify TLS 1.0 and 1.1 are disabled
  TLS_CONFIG Config;
  GetDefaultTlsConfig(&Config);

  EXPECT_GE(Config.MinVersion, TLS_VERSION_1_2)
    << "TLS 1.0/1.1 must be disabled";
}
```

### 6.2 Fuzzing Tests

Use libFuzzer for input fuzzing:

```cpp
// File: NetworkPkg/Test/Fuzz/Dhcp4OptionFuzzer.cpp

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  if (Size < 2 || Size > 1500) {
    return 0;
  }

  // Fuzz DHCP option parsing
  EFI_DHCP4_PACKET Packet;
  ParseDhcp4Options((UINT8*)Data, Size, &Packet);

  // Should not crash regardless of input
  return 0;
}
```

Run with:
```bash
clang++ -fsanitize=fuzzer,address Dhcp4OptionFuzzer.cpp -o fuzzer
./fuzzer -max_total_time=300  # Run for 5 minutes
```

---

## 7. Platform Testing

### 7.1 EmulatorPkg Testing

Test full boot scenarios in emulator:

```c
// File: EmulatorPkg/Test/BootTest.c

UNIT_TEST_STATUS
EFIAPI
TestNormalBoot (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;

  // Boot emulator
  Status = LaunchEmulator(NORMAL_BOOT);
  UT_ASSERT_NOT_EFI_ERROR(Status);

  // Verify boot services available
  UT_ASSERT_NOT_NULL(gBS);

  // Verify runtime services available
  UT_ASSERT_NOT_NULL(gRT);

  return UNIT_TEST_PASSED;
}
```

### 7.2 OvmfPkg Testing

Test in QEMU virtual machine:

```bash
# Build OVMF
build -a X64 -p OvmfPkg/OvmfPkgX64.dsc -t GCC5

# Run in QEMU with test payload
qemu-system-x86_64 \
  -bios Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd \
  -serial stdio \
  -display none \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04

# Check exit code for success/failure
```

---

## 8. Test Implementation Plan

### 8.1 Priority-Based Implementation

#### Phase 1: Security Vulnerability Tests (Week 1-2)

**Objective:** 100% coverage of identified vulnerabilities

1. **Command Injection Tests**
   - RunMakefile.py sanitization
   - MemoryProfileSymbolGen.py sanitization
   - All subprocess.Popen() with shell=True

2. **Memory Safety Tests**
   - AuthService.c integer underflow
   - MTFTP option parsing buffer overflow
   - DHCP option parsing over-read
   - IPv6 fragment reassembly

3. **Cryptographic Tests**
   - PRNG seed validation
   - TLS version enforcement
   - SHA-1 deprecation checks

**Deliverables:**
- 50+ security regression tests
- All tests passing
- Integrated into CI/CD

---

#### Phase 2: Core Package Unit Tests (Week 3-6)

**Objective:** 80% coverage of critical packages

**Package Priority:**
1. **MdePkg** (Week 3)
   - BaseLib (string, math, CPU)
   - SafeIntLib (integer safety)
   - MemoryLib (memory operations)
   - Target: 85% coverage

2. **CryptoPkg** (Week 4)
   - Hash functions
   - Symmetric encryption
   - Asymmetric encryption
   - Random number generation
   - Target: 80% coverage

3. **SecurityPkg** (Week 5)
   - Variable authentication
   - SecureBoot verification
   - TPM operations
   - Target: 80% coverage

4. **NetworkPkg** (Week 6)
   - DHCP client
   - DNS resolution
   - TCP/IP stack
   - TLS implementation
   - Target: 75% coverage

**Deliverables:**
- 500+ unit tests
- 80%+ code coverage
- JUNIT XML reports

---

#### Phase 3: Integration Tests (Week 7-8)

**Objective:** Validate module interactions

**Test Scenarios:**
1. HII Database + Config Routing
2. Network Stack (DHCP → DNS → TCP → TLS)
3. SecureBoot + Variable Services + TPM
4. Boot Services + Runtime Services transitions

**Deliverables:**
- 50+ integration tests
- All critical paths covered

---

#### Phase 4: Platform Tests (Week 9-10)

**Objective:** End-to-end validation

**Platforms:**
- EmulatorPkg (IA32, X64)
- OvmfPkg (IA32, X64, AARCH64)

**Test Scenarios:**
1. Normal boot
2. S3 resume
3. Capsule update
4. SecureBoot enforcement
5. Network boot (PXE)

**Deliverables:**
- 20+ platform tests
- Automated QEMU testing

---

### 8.2 Test File Organization

```
<Package>/
├── Test/
│   ├── GoogleTest/              # GoogleTest-based tests
│   │   ├── Library/
│   │   │   ├── BaseLib/
│   │   │   │   ├── StringTest.cpp
│   │   │   │   ├── MathTest.cpp
│   │   │   │   └── CMakeLists.txt
│   │   │   └── SafeIntLib/
│   │   │       └── SafeIntLibTest.cpp
│   │   ├── Mock/               # Mock libraries
│   │   │   └── MockUefiRuntimeServices/
│   │   └── Security/           # Security tests
│   │       ├── BufferOverflowTest.cpp
│   │       └── IntegerOverflowTest.cpp
│   ├── UnitTest/               # Framework-based tests
│   │   └── Sample/
│   └── <Package>HostTest.dsc   # Host test build file
```

---

## 9. CI/CD Integration

### 9.1 GitHub Actions Workflow

Create `.github/workflows/comprehensive-testing.yml`:

```yaml
name: Comprehensive Testing Suite

on:
  push:
    branches: [ master, stable/* ]
  pull_request:
    branches: [ master ]

jobs:
  unit-tests:
    name: Unit Tests
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest]
        arch: [X64, IA32]

    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Setup Python
        uses: actions/setup-python@v5
        with:
          python-version: '3.11'

      - name: Install Dependencies
        run: pip install -r pip-requirements.txt

      - name: Build Host Tests
        run: |
          python stuart_setup -c .pytool/CISettings.py
          python stuart_update -c .pytool/CISettings.py
          python stuart_build -c .pytool/CISettings.py \
            TOOL_CHAIN_TAG=GCC5 \
            -p MdePkg/Test/MdePkgHostTest.dsc \
            -p MdeModulePkg/Test/MdeModulePkgHostTest.dsc \
            -p SecurityPkg/Test/SecurityPkgHostTest.dsc \
            -p CryptoPkg/Test/CryptoPkgHostUnitTest.dsc \
            -p NetworkPkg/Test/NetworkPkgHostTest.dsc

      - name: Run Unit Tests
        run: |
          find Build -name "*Test" -executable -exec {} \;

      - name: Generate Coverage Report
        run: |
          lcov --capture --directory Build --output-file coverage.info
          lcov --remove coverage.info '/usr/*' --output-file coverage.info
          lcov --list coverage.info

      - name: Upload Coverage
        uses: codecov/codecov-action@v3
        with:
          files: ./coverage.info
          fail_ci_if_error: true

      - name: Check Coverage Threshold
        run: |
          COVERAGE=$(lcov --summary coverage.info | grep lines | cut -d ' ' -f 4 | sed 's/%//')
          if (( $(echo "$COVERAGE < 80" | bc -l) )); then
            echo "Coverage $COVERAGE% is below 80% threshold"
            exit 1
          fi

  security-tests:
    name: Security Regression Tests
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Run Security Tests
        run: |
          # Command injection tests
          python -m pytest BaseTools/Test/SecurityTests/ -v

      - name: Security Scan
        run: |
          pip install bandit safety
          bandit -r BaseTools/Source/Python/ -ll
          safety check -r pip-requirements.txt

  integration-tests:
    name: Integration Tests
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Build and Run Integration Tests
        run: |
          # Build integration test suites
          python stuart_build -c .pytool/CISettings.py \
            -p MdeModulePkg/Test/IntegrationTest.dsc

  platform-tests:
    name: Platform Tests (OVMF)
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install QEMU
        run: sudo apt-get install -y qemu-system-x86

      - name: Build OVMF
        run: |
          python stuart_build -c .pytool/CISettings.py \
            -p OvmfPkg/OvmfPkgX64.dsc

      - name: Run Platform Tests
        run: |
          # Run automated boot tests in QEMU
          ./OvmfPkg/Test/run_platform_tests.sh

  code-quality:
    name: Code Quality Checks
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Run Pylint
        run: |
          pip install pylint
          pylint BaseTools/Source/Python/ --errors-only

      - name: Check for Bare Exceptions
        run: |
          ! grep -r "except:" BaseTools/Source/Python/ --include="*.py" || \
          (echo "Found bare except clauses!" && exit 1)

      - name: Run Uncrustify
        run: |
          python stuart_ci_build -c .pytool/CISettings.py \
            --plugin UncrustifyCheck

  # Fail the workflow if ANY test fails
  require-all-tests:
    name: All Tests Must Pass
    runs-on: ubuntu-latest
    needs: [unit-tests, security-tests, integration-tests, platform-tests, code-quality]
    steps:
      - run: echo "All tests passed!"
```

### 9.2 Failure Criteria

**CI/CD MUST FAIL if:**
1. Any unit test fails
2. Any security test fails
3. Code coverage < 80%
4. Any security scan finds HIGH/CRITICAL issues
5. Pylint finds errors
6. Any bare exception clauses exist
7. Code formatting violations

---

## 10. Coverage Goals and Metrics

### 10.1 Coverage Targets

| Package | Target | Priority |
|---------|--------|----------|
| MdePkg | 85% | CRITICAL |
| MdeModulePkg | 80% | HIGH |
| SecurityPkg | 80% | CRITICAL |
| CryptoPkg | 80% | CRITICAL |
| NetworkPkg | 75% | HIGH |
| All Others | 70% | MEDIUM |

### 10.2 Metrics Collection

**Tools:**
- **lcov/gcov** for C code coverage
- **Coverage.py** for Python coverage
- **JUNIT XML** for test results
- **Codecov** for tracking over time

**Reports Generated:**
- Line coverage
- Branch coverage
- Function coverage
- Uncovered lines report

### 10.3 Dashboard

**CodeCov Integration:**
- Track coverage trends
- Per-PR coverage changes
- Block PRs that reduce coverage

---

## 11. Test Maintenance

### 11.1 Adding New Tests

**Requirements for New Code:**
1. All new functions MUST have unit tests
2. All bug fixes MUST have regression tests
3. All security fixes MUST have security tests
4. Minimum 80% coverage for new code

### 11.2 Test Review Process

**PR Requirements:**
1. All tests passing
2. Coverage maintained or increased
3. Test code reviewed like production code
4. Performance impact assessed

---

## 12. Conclusion

This comprehensive testing strategy ensures:
- **Security:** All vulnerabilities validated and prevented
- **Quality:** 80%+ code coverage with automated checks
- **Reliability:** Comprehensive unit and integration testing
- **Maintainability:** Clear test structure and documentation
- **Automation:** Full CI/CD integration with strict failure criteria

**Success Criteria:**
- Zero test failures in CI/CD
- 80%+ code coverage maintained
- 100% security vulnerability coverage
- All new code has tests

---

**Document Owner:** EDK2 Quality Assurance Team
**Review Cycle:** Quarterly
**Last Updated:** 2025-10-21
