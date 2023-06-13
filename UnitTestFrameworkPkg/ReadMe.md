# Unit Test Framework Package

## About

This package provides unit test frameworks capable of building tests for multiple contexts including
the UEFI shell environment and host-based environments. It allows for unit test development to focus
on the tests and leave error logging, result formatting, context persistence, and test running to the framework.
The unit test framework works well for low level unit tests as well as system level tests and
fits easily in automation frameworks.

### Framework

The first unit test framework is called **Framework** and is implemented as a set of EDK II libraries.
The Framework supports both host-based unit tests and target-based unit tests that share the same
source style, macros, and APIs. In some scenarios, the same unit test case sources can be built
for both host-based unit test execution and target-based unit test execution. Host-based unit tests
that require mocked interfaces can use the mocking infrastructure provided by
[cmocka](https://api.cmocka.org/) that is included in the UnitTestFrameworkPkg as a submodule.

### GoogleTest

The second unit test framework supported by the UnitTestFrameworkPkg is
[GoogleTest](http://google.github.io/googletest/) and can be used to implement host-based unit tests.
[GoogleTest on GitHub](https://github.com/google/googletest) is included in the UnitTestFrameworkPkg
as a submodule. Use of GoogleTest for target-based unit tests of EDK II components is not supported.
Host-based unit tests that require mocked interfaces can use the mocking infrastructure included with
GoogleTest called [gMock](https://github.com/google/googletest/tree/main/googlemock). Note that the
gMock framework does not directly support mocking of free (C style) functions, so the FunctionMockLib
(containing a set of macros that wrap gMock's MOCK_METHOD macro) was created within the
UnitTestFrameworkPkg to enable this support. The details and usage of these macros in the
FunctionMockLib are described later.

GoogleTest requires less overhead to register test suites and test cases compared to the Framework.
There are also a number of tools that layer on top of GoogleTest that improve developer productivity.
One example is the VS Code extension
[C++ TestMate](https://marketplace.visualstudio.com/items?itemName=matepek.vscode-catch2-test-adapter)
that may be used to implement, run, and debug unit tests implemented using GoogleTest. The following
is an example of the C++ TestMate JSON configuration to find unit tests and configure the environment
for unit test execution.

```
"testMate.cpp.test.advancedExecutables": [
    {
        "pattern": "Build/**/*Test*",
        "cwd": "${absDirpath}",
        "env": {
            "GTEST_CATCH_EXCEPTIONS": "0",
            "ASAN_OPTIONS": "detect_leaks=0",
        }
    }
],
```

If a component can be tested with host-based unit tests, then *GoogleTest is recommended*. The MdePkg
contains a port of the BaseSafeIntLib unit tests in the GoogleTest style so the differences between
GoogleTest and Framework unit tests can be reviewed. The paths to the BaseSafeIntLib unit tests are:

* `MdePkg/Test/UnitTest/Library/BaseSafeIntLib`
* `MdePkg/Test/GoogleTest/Library/BaseSafeIntLib`

Furthermore, the SecurityPkg contains unit tests for the SecureBootVariableLib using mocks in both
the Framework/cmocka and GoogleTest/gMock style so the differences between cmocka and gMock can be
reviewed. The paths to the SecureBootVariableLib unit tests are:

* `SecurityPkg/Library/SecureBootVariableLib/UnitTest`
* `SecurityPkg/Library/SecureBootVariableLib/GoogleTest`

## Framework and GoogleTest Feature Comparison

| Feature                     | Framework | GoogleTest |
|:----------------------------|:---------:|:----------:|
| Host Based Unit Tests       |    YES    |    YES     |
| Target Based Unit Tests     |    YES    |     NO     |
| Unit Test Source Language   |     C     |    C++     |
| Register Test Suite         |    YES    |    Auto    |
| Register Test Case          |    YES    |    Auto    |
| Expected Assert Tests       |    YES    |    YES     |
| Setup/Teardown Hooks        |    YES    |    YES     |
| Value-Parameterized Tests   |    NO     |    YES     |
| Typed Tests                 |    NO     |    YES     |
| Type-Parameterized Tests    |    NO     |    YES     |
| Timeout Support             |    NO     |    YES     |
| Mocking Support             |   Cmocka  |   gMock    |
| JUNIT XML Reports           |    YES    |    YES     |
| Execute subset of tests     |    NO     |    YES     |
| VS Code Extensions          |    NO     |    YES     |
| Address Sanitizer           |   Cmocka  |    YES     |


Please see our separate documents for detailed instructions and sample usage of both frameworks:

=======
[Framework](./Framework.md)
[GoogleTest](./GoogleTest.md)

## Development

### Iterating on a Single Test

When using the EDK2 Pytools for CI testing, the host-based unit tests will be built and run on any build that includes
the `NOOPT` build target.

If you are trying to iterate on a single test, a convenient pattern is to build only that test module. For example,
the following command will build only the SafeIntLib host-based test from the MdePkg...

```bash
stuart_ci_build -c .pytool/CISettings.py TOOL_CHAIN_TAG=VS2022 -p MdePkg -t NOOPT BUILDMODULE=MdePkg/Test/UnitTest/Library/BaseSafeIntLib/TestBaseSafeIntLib.inf
```

## Building and Running Host-Based Tests

The EDK2 CI infrastructure provides a convenient way to run all host-based tests -- in the the entire tree or just
selected packages -- and aggregate all the reports, including highlighting any failures. This functionality is
provided through the Stuart build system (published by EDK2-PyTools) and the `NOOPT` build target. The sections that
follow use Framework examples. Unit tests based on GoogleTest are built and run the same way. The text output and
JUNIT XML output format have small differences.

### Building Locally

First, to make sure you're working with the latest PyTools, run the following command:

```bash
# Would recommend running this in a Python venv, but that's out of scope for this doc.
python -m pip install --upgrade -r ./pip-requirements.txt
```

After that, the following commands will set up the build and run the host-based tests.

```bash
# Setup repo for building
# stuart_setup -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=<GCC5, VS2022, etc.>
stuart_setup -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=VS2022

# Update all binary dependencies
# stuart_update -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=<GCC5, VS2022, etc.>
stuart_update -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=VS2022

# Build and run the tests
# stuart_ci_build -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=<GCC5, VS2022, etc.> -t NOOPT [-p <Package Name>]
stuart_ci_build -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=VS2022 -t NOOPT -p MdePkg
```

#### Disabling Address Sanitizer

By default, the address sanitizer feature is enabled for all host based unit test builds.  It can be disabled for
development/debug purposes by setting the DSC define `UNIT_TESTING_ADDRESS_SANITIZER_ENABLE` to `FALSE`.

```
stuart_ci_build -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=VS2022 -t NOOPT -p MdePkg BLD_*_UNIT_TESTING_ADDRESS_SANITIZER_ENABLE=FALSE
```

### Evaluating the Results

In your immediate output, any build failures will be highlighted. You can see these below as "WARNING" and "ERROR" messages.

```text
(edk_env) PS C:\_uefi\edk2> stuart_ci_build -c .\.pytool\CISettings.py TOOL_CHAIN_TAG=VS2022 -t NOOPT -p MdePkg

SECTION - Init SDE
SECTION - Loading Plugins
SECTION - Start Invocable Tool
SECTION - Getting Environment
SECTION - Loading plugins
SECTION - Building MdePkg Package
PROGRESS - --Running MdePkg: Host Unit Test Compiler Plugin NOOPT --
WARNING - Allowing Override for key TARGET_ARCH
PROGRESS - Start time: 2020-07-27 17:18:08.521672
PROGRESS - Setting up the Environment
PROGRESS - Running Pre Build
PROGRESS - Running Build NOOPT
PROGRESS - Running Post Build
SECTION - Run Host based Unit Tests
SUBSECTION - Testing for architecture: X64
WARNING - TestBaseSafeIntLibHost.exe Test Failed
WARNING -   Test SafeInt8ToUint8 - UT_ASSERT_EQUAL(0x5b:5b, Result:5c)
c:\_uefi\edk2\MdePkg\Test\UnitTest\Library\BaseSafeIntLib\TestBaseSafeIntLib.c:35: error: Failure!
ERROR - Plugin Failed: Host-Based Unit Test Runner returned 1
CRITICAL - Post Build failed
PROGRESS - End time: 2020-07-27 17:18:19.792313  Total time Elapsed: 0:00:11
ERROR - --->Test Failed: Host Unit Test Compiler Plugin NOOPT returned 1
ERROR - Overall Build Status: Error
PROGRESS - There were 1 failures out of 1 attempts
SECTION - Summary
ERROR - Error

(edk_env) PS C:\_uefi\edk2>
```

If a test fails, you can run it manually to get more details...

```text
(edk_env) PS C:\_uefi\edk2> .\Build\MdePkg\HostTest\NOOPT_VS2022\X64\TestBaseSafeIntLibHost.exe

Int Safe Lib Unit Test Application v0.1
---------------------------------------------------------
------------     RUNNING ALL TEST SUITES   --------------
---------------------------------------------------------
---------------------------------------------------------
RUNNING TEST SUITE: Int Safe Conversions Test Suite
---------------------------------------------------------
[==========] Running 71 test(s).
[ RUN      ] Test SafeInt8ToUint8
[  ERROR   ] --- UT_ASSERT_EQUAL(0x5b:5b, Result:5c)
[   LINE   ] --- c:\_uefi\edk2\MdePkg\Test\UnitTest\Library\BaseSafeIntLib\TestBaseSafeIntLib.c:35: error: Failure!
[  FAILED  ] Test SafeInt8ToUint8
[ RUN      ] Test SafeInt8ToUint16
[       OK ] Test SafeInt8ToUint16
[ RUN      ] Test SafeInt8ToUint32
[       OK ] Test SafeInt8ToUint32
[ RUN      ] Test SafeInt8ToUintn
[       OK ] Test SafeInt8ToUintn
...
```

You can also, if you are so inclined, read the output from the exact instance of the test that was run during
`stuart_ci_build`. The output file can be found on a path that looks like:

`Build/<Package>/HostTest/<Arch>/<TestName>.<TestSuiteName>.<Arch>.result.xml`

A sample of this output looks like:

```xml
<!--
  Excerpt taken from:
  Build\MdePkg\HostTest\NOOPT_VS2022\X64\TestBaseSafeIntLibHost.exe.Int Safe Conversions Test Suite.X64.result.xml
  -->
<?xml version="1.0" encoding="UTF-8" ?>
<testsuites>
  <testsuite name="Int Safe Conversions Test Suite" time="0.000" tests="71" failures="1" errors="0" skipped="0" >
    <testcase name="Test SafeInt8ToUint8" time="0.000" >
      <failure><![CDATA[UT_ASSERT_EQUAL(0x5c:5c, Result:5b)
c:\_uefi\MdePkg\Test\UnitTest\Library\BaseSafeIntLib\TestBaseSafeIntLib.c:35: error: Failure!]]></failure>
    </testcase>
    <testcase name="Test SafeInt8ToUint16" time="0.000" >
    </testcase>
    <testcase name="Test SafeInt8ToUint32" time="0.000" >
    </testcase>
    <testcase name="Test SafeInt8ToUintn" time="0.000" >
    </testcase>
```

### Manually Running Unit Test Executables

The host based unit test executed using `stuart_ci_build` sets up the environment to run host based unit tests
including environment variable settings. If host based unit test executable are run manually either from a
shell or using VS Code extensions such as `C++ TestMate`, then the environment must be setup correctly.

#### Windows Environment Variable Settings

```
set GTEST_CATCH_EXCEPTIONS=0
set ASAN_OPTIONS=detect_leaks=0
```

#### Linux Environment Variable Settings

```
export GTEST_CATCH_EXCEPTIONS=0
export ASAN_OPTIONS=detect_leaks=0
```

### XML Reporting Mode

Unit test applications using Framework are built using Cmocka that requires the
following environment variables to be set to generate structured XML output
rather than text:

```
CMOCKA_MESSAGE_OUTPUT=xml
CMOCKA_XML_FILE=<absolute or relative path to output file>
```

Unit test applications using GoogleTest require the following environment
variable to be set to generate structured XML output rather than text:

```
GTEST_OUTPUT=xml:<absolute or relative path to output file>
```

This mode is used by the test running plugin to aggregate the results for CI test status reporting in the web view.

### Code Coverage

Host based Unit Tests will automatically enable coverage data.

For Windows, this is primarily leveraged for pipeline builds, but this can be leveraged locally using the
OpenCppCoverage windows tool to parse coverage data to cobertura xml format.

- Windows Prerequisite
  ```bash
  Download and install https://github.com/OpenCppCoverage/OpenCppCoverage/releases
  python -m pip install --upgrade -r ./pip-requirements.txt
  stuart_ci_build -c .pytool/CISettings.py  -t NOOPT TOOL_CHAIN_TAG=VS2022 -p MdeModulePkg
  Open Build/coverage.xml
  ```

  - How to see code coverage data on IDE Visual Studio
    ```
    Open Visual Studio VS2022 or above version
    Click "Tools" -> "OpenCppCoverage Settings"
    Fill your execute file into "Program to run:"
    Click "Tools" -> "Run OpenCppCoverage"
    ```


For Linux, this is primarily leveraged for pipeline builds, but this can be leveraged locally using the
lcov linux tool, and parsed using the lcov_cobertura python tool to parse it to cobertura xml format.

- Linux Prerequisite
  ```bash
  sudo apt-get install -y lcov
  python -m pip install --upgrade -r ./pip-requirements.txt
  stuart_ci_build -c .pytool/CISettings.py  -t NOOPT TOOL_CHAIN_TAG=GCC5 -p MdeModulePkg
  Open Build/coverage.xml
  ```
  - How to see code coverage data on IDE Visual Studio Code
    ```
    Download plugin "Coverage Gutters"
    Press Hot Key "Ctrl + Shift + P" and click option "Coverage Gutters: Display Coverage"
    ```


### Important Note

This works on both Windows and Linux but is currently limited to x64 architectures. Working on getting others, but we
also welcome contributions.


## Unit Test Location/Layout Rules

Code/Test                                   | Location
---------                                   | --------
Host-Based Unit Tests for a Library/Protocol/PPI/GUID Interface   | If what's being tested is an interface (e.g. a library with a public header file, like DebugLib) and the test is agnostic to a specific implementation, then the test should be scoped to the parent package.<br/>Example: `MdePkg/Test/UnitTest/[Library/Protocol/Ppi/Guid]/`<br/><br/>A real-world example of this is the BaseSafeIntLib test in MdePkg.<br/>`MdePkg/Test/UnitTest/Library/BaseSafeIntLib/TestBaseSafeIntLibHost.inf`
Host-Based Unit Tests for a Library/Driver (PEI/DXE/SMM) implementation   | If what's being tested is a specific implementation (e.g. BaseDebugLibSerialPort for DebugLib), then the test should be scoped to the implementation directory itself, in a UnitTest (or GoogleTest) subdirectory.<br/><br/>Module Example: `MdeModulePkg/Universal/EsrtFmpDxe/UnitTest/`<br/>Library Example: `MdePkg/Library/BaseMemoryLib/UnitTest/`<br/>Library Example (GoogleTest): `SecurityPkg/Library/SecureBootVariableLib/GoogleTest/`
Host-Based Tests for a Functionality or Feature   | If you're writing a functional test that operates at the module level (i.e. if it's more than a single file or library), the test should be located in the package-level Tests directory under the HostFuncTest subdirectory.<br/>For example, if you were writing a test for the entire FMP Device Framework, you might put your test in:<br/>`FmpDevicePkg/Test/HostFuncTest/FmpDeviceFramework`<br/><br/>If the feature spans multiple packages, it's location should be determined by the package owners related to the feature.
Non-Host-Based (PEI/DXE/SMM/Shell) Tests for a Functionality or Feature   | Similar to Host-Based, if the feature is in one package, should be located in the `*Pkg/Test/[Shell/Dxe/Smm/Pei]Test` directory.<br/><br/>If the feature spans multiple packages, it's location should be determined by the package owners related to the feature.<br/><br/>USAGE EXAMPLES<br/>PEI Example: MP_SERVICE_PPI. Or check MTRR configuration in a notification function.<br/> SMM Example: a test in a protocol callback function. (It is different with the solution that SmmAgent+ShellApp)<br/>DXE Example: a test in a UEFI event call back to check SPI/SMRAM status. <br/> Shell Example: the SMM handler audit test has a shell-based app that interacts with an SMM handler to get information. The SMM paging audit test gathers information about both DXE and SMM. And the SMM paging functional test actually forces errors into SMM via a DXE driver.

### Example Directory Tree

```text
<PackageName>Pkg/
  ComponentY/
    ComponentY.inf
    ComponentY.c
    GoogleTest/
      ComponentYHostGoogleTest.inf    # Host-Based Test for Driver Module
      ComponentYGoogleTest.cpp
    UnitTest/
      ComponentYHostUnitTest.inf      # Host-Based Test for Driver Module
      ComponentYUnitTest.c

  Library/
    GeneralPurposeLibBase/
      ...

    GeneralPurposeLibSerial/
      ...

    SpecificLibDxe/
      SpecificLibDxe.c
      SpecificLibDxe.inf
      GoogleTest/                    # Host-Based Test for Specific Library Implementation
        SpecificLibDxeHostGoogleTest.cpp
        SpecificLibDxeHostGoogleTest.inf
      UnitTest/                      # Host-Based Test for Specific Library Implementation
        SpecificLibDxeHostUnitTest.c
        SpecificLibDxeHostUnitTest.inf
  Test/
    <Package>HostTest.dsc             # Host-Based Test Apps
    GoogleTest/
      InterfaceX
        InterfaceXHostGoogleTest.inf  # Host-Based App (should be in Test/<Package>HostTest.dsc)
        InterfaceXUnitTest.cpp        # Test Logic

      GeneralPurposeLib/              # Host-Based Test for any implementation of GeneralPurposeLib
        GeneralPurposeLibTest.cpp
        GeneralPurposeLibHostUnitTest.inf

    UnitTest/
      InterfaceX
        InterfaceXHostUnitTest.inf    # Host-Based App (should be in Test/<Package>HostTest.dsc)
        InterfaceXPeiUnitTest.inf     # PEIM Target-Based Test (if applicable)
        InterfaceXDxeUnitTest.inf     # DXE Target-Based Test (if applicable)
        InterfaceXSmmUnitTest.inf     # SMM Target-Based Test (if applicable)
        InterfaceXShellUnitTest.inf   # Shell App Target-Based Test (if applicable)
        InterfaceXUnitTest.c          # Test Logic

      GeneralPurposeLib/              # Host-Based Test for any implementation of GeneralPurposeLib
        GeneralPurposeLibTest.c
        GeneralPurposeLibHostUnitTest.inf

    Mock/
      Include/
        GoogleTest/
          Library/
            MockGeneralPurposeLib.h

      Library/
        GoogleTest/
          MockGeneralPurposeLib/
            MockGeneralPurposeLib.cpp
            MockGeneralPurposeLib.inf

  <Package>Pkg.dsc          # Standard Modules and any Target-Based Test Apps (including in Test/)
```

### Future Locations in Consideration

We don't know if these types will exist or be applicable yet, but if you write a support library or module that matches the following, please make sure they live in the correct place.

Code/Test                                   | Location
---------                                   | --------
Host-Based Library Implementations                 | Host-Based Implementations of common libraries (eg. MemoryAllocationLibHost) should live in the same package that declares the library interface in its .DEC file in the `*Pkg/HostLibrary` directory. Should have 'Host' in the name.
Host-Based Mocks and Stubs  | Mock and Stub libraries should live in the `UefiTestFrameworkPkg/StubLibrary` with either 'Mock' or 'Stub' in the library name.

### If still in doubt...

Ask a question in [edk2 discussions](https://github.com/tianocore/edk2/discussions) or reach out on the [EDK II development mailing list \<`devel@edk2.groups.io`\>](mailto:devel@edk2.groups.io).

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
