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
[GoogleTest](http://google.github.io/googletest/) that can be used to implement host-based unit tests.
Use of GoogleTest for target-based unit tests of EDK II components is not supported. If a
host-based unit test requires mocked interfaces, then the Framework with cmocka support should be
used instead. Enabling support for mocked interfaces with GoogleTest is being actively investigated.
[GoogleTest on GitHub](https://github.com/google/googletest) is included in the UnitTestFrameworkPkg
as a submodule.

GoogleTest requires less overhead to register test suites and test cases compared to the Framework.
There are also a number of tools that layer on top of GoogleTest that improve developer productivity.
One example is the VS Code extension
[C++ TestMate](https://marketplace.visualstudio.com/items?itemName=matepek.vscode-catch2-test-adapter)
that may be used to implement, run, and debug unit tests implemented using GoogleTest.

If a component can be tested with host-based unit tests without support for mocked interfaces,
then GoogleTest is recommended. The MdePkg contains a port of the BaseSafeIntLib unit tests in
the GoogleTest style so the differences between GoogleTest and Framework unit tests can be reviewed.
The paths to the BaseSafeIntLib unit tests are:

* MdePkg\Test\UnitTest\Library\BaseSafeIntLib
* MdePkg\Test\GoogleTest\Library\BaseSafeIntLib

## Framework and GoogleTest Feature Comparison

| Feature                     | Framework | GoogleTest |
|:----------------------------|:---------:|:----------:|
| Host Based Unit Tests       |    YES    |    YES     |
| Target Based Unit Tests     |    YES    |     NO     |
| Unit Test Source Language   |     C     |    C++     |
| Register Test Suite         |    YES    |    Auto    |
| Register Test Case          |    YES    |    Auto    |
| Death/Expected Assert Tests |    YES    |    YES     |
| Setup/Teardown Hooks        |    YES    |    YES     |
| Value-Parameterized Tests   |    NO     |    YES     |
| Typed Tests                 |    NO     |    YES     |
| Type-Parameterized Tests    |    NO     |    YES     |
| Timeout Support             |    NO     |    YES     |
| Mocking Support             |   Cmocka  |     NO     |
| JUNIT XML Reports           |    YES    |    YES     |
| Execute subset of tests     |    NO     |    YES     |
| VS Code Extensions          |    NO     |    YES     |

## Framework Libraries

### UnitTestLib

The main "framework" library. The core of the framework is the Framework object, which can have any number
of test cases and test suites registered with it. The Framework object is also what drives test execution.

The Framework also provides helper macros and functions for checking test conditions and
reporting errors. Status and error info will be logged into the test context. There are a number
of Assert macros that make the unit test code friendly to view and easy to understand.

Finally, the Framework also supports logging strings during the test execution. This data is logged
to the test context and will be available in the test reporting phase. This should be used for
logging test details and helpful messages to resolve test failures.

### UnitTestPersistenceLib

Persistence lib has the main job of saving and restoring test context to a storage medium so that for tests
that require exiting the active process and then resuming state can be maintained. This is critical
in supporting a system reboot in the middle of a test run.

### UnitTestResultReportLib

Library provides function to run at the end of a framework test run and handles formatting the report.
This is a common customization point and allows the unit test framework to fit its output reports into
other test infrastructure. In this package simple library instances have been supplied to output test
results to the console as plain text.

## Framework Samples

There is a sample unit test provided as both an example of how to write a unit test and leverage
many of the features of the framework. This sample can be found in the `Test/UnitTest/Sample/SampleUnitTest`
directory.

The sample is provided in PEI, SMM, DXE, and UEFI App flavors. It also has a flavor for the HOST_APPLICATION
build type, which can be run on a host system without needing a target.

## Framework Usage

This section is built a lot like a "Getting Started". We'll go through some of the components that are needed
when constructing a unit test and some of the decisions that are made by the test writer. We'll also describe
how to check for expected conditions in test cases and a bit of the logging characteristics.

Most of these examples will refer to the SampleUnitTestUefiShell app found in this package.

### Framework Requirements - INF

In our INF file, we'll need to bring in the `UnitTestLib` library. Conveniently, the interface
header for the `UnitTestLib` is located in `MdePkg`, so you shouldn't need to depend on any other
packages. As long as your DSC file knows where to find the lib implementation that you want to use,
you should be good to go.

See this example in 'SampleUnitTestUefiShell.inf'...

```
[Packages]
  MdePkg/MdePkg.dec

[LibraryClasses]
  UefiApplicationEntryPoint
  BaseLib
  DebugLib
  UnitTestLib
  PrintLib
```

Also, if you want you test to automatically be picked up by the Test Runner plugin, you will need
to make sure that the module `BASE_NAME` contains the word `Test`...

```
[Defines]
  BASE_NAME      = SampleUnitTestUefiShell
```

### Framework Requirements - Code

Not to state the obvious, but let's make sure we have the following include before getting too far along...

```c
#include <Library/UnitTestLib.h>
```

Now that we've got that squared away, let's look at our 'Main()'' routine (or DriverEntryPoint() or whatever).

### Framework Configuration

Everything in the UnitTestFrameworkPkg framework is built around an object called -- conveniently -- the Framework.
This Framework object will contain all the information about our test, the test suites and test cases associated
with it, the current location within the test pass, and any results that have been recorded so far.

To get started with a test, we must first create a Framework instance. The function for this is
`InitUnitTestFramework`. It takes in `CHAR8` strings for the long name, short name, and test version.
The long name and version strings are just for user presentation and relatively flexible. The short name
will be used to name any cache files and/or test results, so should be a name that makes sense in that context.
These strings are copied internally to the Framework, so using stack-allocated or literal strings is fine.

In the 'SampleUnitTestUefiShell' app, the module name is used as the short name, so the initialization looks like this.

```c
DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION ));

//
// Start setting up the test framework for running the tests.
//
Status = InitUnitTestFramework( &Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION );
```

The `&Framework` returned here is the handle to the Framework. If it's successfully returned, we can start adding
test suites and test cases.

Test suites exist purely to help organize test cases and to differentiate the results in reports. If you're writing
a small unit test, you can conceivably put all test cases into a single suite. However, if you end up with 20+ test
cases, it may be beneficial to organize them according to purpose. You _must_ have at least one test suite, even if
it's just a catch-all. The function to create a test suite is `CreateUnitTestSuite`. It takes in a handle to
the Framework object, a `CHAR8` string for the suite title and package name, and optional function pointers for
a setup function and a teardown function.

The suite title is for user presentation. The package name is for xUnit type reporting and uses a '.'-separated
hierarchical format (see 'SampleUnitTestApp' for example). If provided, the setup and teardown functions will be
called once at the start of the suite (before _any_ tests have run) and once at the end of the suite (after _all_
tests have run), respectively. If either or both of these are unneeded, pass `NULL`. The function prototypes are
`UNIT_TEST_SUITE_SETUP` and `UNIT_TEST_SUITE_TEARDOWN`.

Looking at 'SampleUnitTestUefiShell' app, you can see that the first test suite is created as below...

```c
//
// Populate the SimpleMathTests Unit Test Suite.
//
Status = CreateUnitTestSuite( &SimpleMathTests, Fw, "Simple Math Tests", "Sample.Math", NULL, NULL );
```

This test suite has no setup or teardown functions. The `&SimpleMathTests` returned here is a handle to the suite and
will be used when adding test cases.

Great! Now we've finished some of the cruft, red tape, and busy work. We're ready to add some tests. Adding a test
to a test suite is accomplished with the -- you guessed it -- `AddTestCase` function. It takes in the suite handle;
a `CHAR8` string for the description and class name; a function pointer for the test case itself; additional, optional
function pointers for prerequisite check and cleanup routines; and an optional pointer to a context structure.

Okay, that's a lot. Let's take it one piece at a time. The description and class name strings are very similar in
usage to the suite title and package name strings in the test suites. The former is for user presentation and the
latter is for xUnit parsing. The test case function pointer is what is executed as the "test" and the
prototype should be `UNIT_TEST_FUNCTION`. The last three parameters require a little bit more explaining.

The prerequisite check function has a prototype of `UNIT_TEST_PREREQUISITE` and -- if provided -- will be called
immediately before the test case. If this function returns any error, the test case will not be run and will be
recorded as `UNIT_TEST_ERROR_PREREQUISITE_NOT_MET`. The cleanup function (prototype `UNIT_TEST_CLEANUP`) will be called
immediately after the test case to provide an opportunity to reset any global state that may have been changed in the
test case. In the event of a prerequisite failure, the cleanup function will also be skipped. If either of these
functions is not needed, pass `NULL`.

The context pointer is entirely case-specific. It will be passed to the test case upon execution. One of the purposes
of the context pointer is to allow test case reuse with different input data. (Another use is for testing that wraps
around a system reboot, but that's beyond the scope of this guide.) The test case must know how to interpret the context
pointer, so it could be a simple value, or it could be a complex structure. If unneeded, pass `NULL`.

In 'SampleUnitTestUefiShell' app, the first test case is added using the code below...

```c
AddTestCase( SimpleMathTests, "Adding 1 to 1 should produce 2", "Addition", OnePlusOneShouldEqualTwo, NULL, NULL, NULL );
```

This test case calls the function `OnePlusOneShouldEqualTwo` and has no prerequisite, cleanup, or context.

Once all the suites and cases are added, it's time to run the Framework.

```c
//
// Execute the tests.
//
Status = RunAllTestSuites( Framework );
```

### Framework - A Simple Test Case

We'll take a look at the below test case from 'SampleUnitTestApp'...

```c
UNIT_TEST_STATUS
EFIAPI
OnePlusOneShouldEqualTwo (
  IN UNIT_TEST_FRAMEWORK_HANDLE  Framework,
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINTN     A, B, C;

  A = 1;
  B = 1;
  C = A + B;

  UT_ASSERT_EQUAL(C, 2);
  return UNIT_TEST_PASSED;
} // OnePlusOneShouldEqualTwo()
```

The prototype for this function matches the `UNIT_TEST_FUNCTION` prototype. It takes in a handle to the Framework
itself and the context pointer. The context pointer could be cast and interpreted as anything within this test case,
which is why it's important to configure contexts carefully. The test case returns a value of `UNIT_TEST_STATUS`, which
will be recorded in the Framework and reported at the end of all suites.

In this test case, the `UT_ASSERT_EQUAL` assertion is being used to establish that the business logic has functioned
correctly. There are several assertion macros, and you are encouraged to use one that matches as closely to your
intended test criterium as possible, because the logging is specific to the macro and more specific macros have more
detailed logs. When in doubt, there are always `UT_ASSERT_TRUE` and `UT_ASSERT_FALSE`. Assertion macros that fail their
test criterium will immediately return from the test case with `UNIT_TEST_ERROR_TEST_FAILED` and log an error string.
_Note_ that this early return can have implications for memory leakage.

At the end, if all test criteria pass, you should return `UNIT_TEST_PASSED`.

### Framework - More Complex Cases

To write more advanced tests, first look at all the Assertion and Logging macros provided in the framework.

Beyond that, if you're writing host-based tests and want to take a dependency on the UnitTestFrameworkPkg, you can
leverage the `cmocka.h` interface and write tests with all the features of the Cmocka framework.

Documentation for Cmocka can be found here:
https://api.cmocka.org/

## GoogleTest Samples

There is a sample unit test provided as both an example of how to write a unit test and leverage
many of the GoogleTest features. This sample can be found in the `Test/GoogleTest/Sample/SampleGoogleTest`
directory.

The sample is provided for the HOST_APPLICATION build type, which can be run on a host system without
needing a target.

## GoogleTest Usage

This section is built a lot like a "Getting Started". We'll go through some of the components that are needed
when constructing a unit test and some of the decisions that are made by the test writer. We'll also describe
how to check for expected conditions in test cases and a bit of the logging characteristics.

Most of these examples will refer to the SampleGoogleTestHost app found in this package.

### GoogleTest Requirements - INF

In our INF file, we'll need to bring in the `GoogleTest` library. Conveniently, the interface
header for the `GoogleTest` is in `UnitTestFrameworkPkg`, so you shouldn't need to depend on any other
packages. As long as your DSC file knows where to find the lib implementation that you want to use,
you should be good to go.

See this example in 'SampleGoogleTestHost.inf'...

```
[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[LibraryClasses]
  GoogleTestLib
  BaseLib
  DebugLib
```

Also, if you want you test to automatically be picked up by the Test Runner plugin, you will need
to make sure that the module `BASE_NAME` contains the word `Test`...

```
[Defines]
  BASE_NAME      = SampleGoogleTestHost
```

### GoogleTest Requirements - Code

Not to state the obvious, but let's make sure we have the following include before getting too far along...

```
#include <gtest/gtest.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
}
```

GoogleTest applications are implemented in C++. The first include brings in the
GoogleTest definitions. Other EDK II related include files must be wrapped in
`extern "C" {}` because they are C include files. Link failures will occur if
this is not done.

Now that we've got that squared away, let's look at our 'Main()'' routine (or DriverEntryPoint() or whatever).

### GoogleTest Configuration

Unlike the Framework, GoogleTest does not require test suites or test cases to
be registered. Instead, the test cases declare the test suite name and test
case name as part of their implementation. The only requirement for GoogleTest
is to have a `main()` function that initialize the GoogleTest infrastructure and
call the service `RUN_ALL_TESTS()` to run all the unit tests.

```c
int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
```

### GoogleTest - A Simple Test Case

We'll look at the below test case from 'SampleGoogleTestHost'...

```c
TEST(SimpleMathTests, OnePlusOneShouldEqualTwo) {
  UINTN  A;
  UINTN  B;
  UINTN  C;

  A = 1;
  B = 1;
  C = A + B;

  ASSERT_EQ (C, 2);
}
```

This uses the simplest form of a GoogleTest unit test using `TEST()` that
declares the test suite name and the unit test name within that test suite.
The unit test performs actions and typically makes calls to the code under test
and contains test assertions to verify that the code under test behaves as
expected for the given inputs.

In this test case, the `ASSERT_EQ` assertion is being used to establish that the business logic has functioned
correctly. There are several assertion macros, and you are encouraged to use one that matches as closely to your
intended test criterium as possible, because the logging is specific to the macro and more specific macros have more
detailed logs. When in doubt, there are always `ASSERT_TRUE` and `ASSERT_FALSE`. Assertion macros that fail their
test criterium will immediately return from the test case with a failed status and log an error string.
_Note_ that this early return can have implications for memory leakage.

There is no return status from a GooglTest unit test. If no assertions are
triggered then the unit test has a passing status.

### GoogleTest - More Complex Cases

To write more advanced tests, take a look at the
[GoogleTest User's Guide](http://google.github.io/googletest/).

## Development

### Iterating on a Single Test

When using the EDK2 Pytools for CI testing, the host-based unit tests will be built and run on any build that includes
the `NOOPT` build target.

If you are trying to iterate on a single test, a convenient pattern is to build only that test module. For example,
the following command will build only the SafeIntLib host-based test from the MdePkg...

```bash
stuart_ci_build -c .pytool/CISettings.py TOOL_CHAIN_TAG=VS2017 -p MdePkg -t NOOPT BUILDMODULE=MdePkg/Test/UnitTest/Library/BaseSafeIntLib/TestBaseSafeIntLib.inf
```

### Hooking BaseLib

Most unit test mocking can be performed by the functions provided in the UnitTestFrameworkPkg libraries, but since
BaseLib is consumed by the Framework itself, it requires different techniques to substitute parts of the
functionality.

To solve some of this, the UnitTestFrameworkPkg consumes a special implementation of BaseLib for host-based tests.
This implementation contains a [hook table](https://github.com/tianocore/edk2/blob/e188ecc8b4aed8fdd26b731d43883861f5e5e7b4/MdePkg/Test/UnitTest/Include/Library/UnitTestHostBaseLib.h#L507)
that can be used to substitute test functionality for any of the BaseLib functions. By default, this implementation
will use the underlying BaseLib implementation, so the unit test writer only has to supply minimal code to test a
particular case.

### Debugging the Framework Itself

While most of the tests that are produced by the UnitTestFrameworkPkg are easy to step through in a debugger, the Framework
itself consumes code (mostly Cmocka) that sets its own build flags. These flags cause parts of the Framework to not
export symbols and captures exceptions, and as such are harder to debug. We have provided a Stuart parameter to force
symbolic debugging to be enabled.

You can run a build by adding the `BLD_*_UNIT_TESTING_DEBUG=TRUE` parameter to enable this build option.

```bash
stuart_ci_build -c .pytool/CISettings.py TOOL_CHAIN_TAG=VS2019 -p MdePkg -t NOOPT BLD_*_UNIT_TESTING_DEBUG=TRUE
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
# stuart_setup -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=<GCC5, VS2019, etc.>
stuart_setup -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=VS2019

# Update all binary dependencies
# stuart_update -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=<GCC5, VS2019, etc.>
stuart_update -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=VS2019

# Build and run the tests
# stuart_ci_build -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=<GCC5, VS2019, etc.> -t NOOPT [-p <Package Name>]
stuart_ci_build -c ./.pytool/CISettings.py TOOL_CHAIN_TAG=VS2019 -t NOOPT -p MdePkg
```

### Evaluating the Results

In your immediate output, any build failures will be highlighted. You can see these below as "WARNING" and "ERROR" messages.

```text
(edk_env) PS C:\_uefi\edk2> stuart_ci_build -c .\.pytool\CISettings.py TOOL_CHAIN_TAG=VS2019 -t NOOPT -p MdePkg

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
(edk_env) PS C:\_uefi\edk2> .\Build\MdePkg\HostTest\NOOPT_VS2019\X64\TestBaseSafeIntLibHost.exe

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
  Build\MdePkg\HostTest\NOOPT_VS2019\X64\TestBaseSafeIntLibHost.exe.Int Safe Conversions Test Suite.X64.result.xml
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

For Windows, This is primarily leverage for pipeline builds, but this can be leveraged locally using the
OpenCppCoverage windows tool to parse coverage data to cobertura xml format.

- Windows Prerequisite
  ```bash
  Download and install https://github.com/OpenCppCoverage/OpenCppCoverage/releases
  python -m pip install --upgrade -r ./pip-requirements.txt
  stuart_ci_build -c .pytool/CISettings.py  -t NOOPT TOOL_CHAIN_TAG=VS2019 -p MdeModulePkg
  Open Build/coverage.xml
  ```

  - How to see code coverage data on IDE Visual Studio
    ```
    Open Visual Studio VS2019 or above version
    Click "Tools" -> "OpenCppCoverage Settings"
    Fill your execute file into "Program to run:"
    Click "Tools" -> "Run OpenCppCoverage"
    ```


For Linux, This is primarily leveraged for pipeline builds, but this can be leveraged locally using the
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

## Framework Known Limitations

### PEI, DXE, SMM

While sample tests have been provided for these execution environments, only cursory build validation
has been performed. Care has been taken while designing the frameworks to allow for execution during
boot phases, but only UEFI Shell and host-based tests have been thoroughly evaluated. Full support for
PEI, DXE, and SMM is forthcoming, but should be considered beta/staging for now.

### Host-Based Support vs Other Tests

The host-based test framework is powered internally by the Cmocka framework. As such, it has abilities
that the target-based tests don't (yet). It would be awesome if this meant that it was a super set of
the target-based tests, and it worked just like the target-based tests but with more features. Unfortunately,
this is not the case. While care has been taken to keep them as close as possible, there are a few known
inconsistencies that we're still ironing out. For example, the logging messages in the target-based tests
are cached internally and associated with the running test case. They can be saved later as part of the
reporting lib. This isn't currently possible with host-based. Only the assertion failures are logged.

We will continue trying to make these as similar as possible.

## Unit Test Location/Layout Rules

Code/Test                                   | Location
---------                                   | --------
Host-Based Unit Tests for a Library/Protocol/PPI/GUID Interface   | If what's being tested is an interface (e.g. a library with a public header file, like DebugLib), the test should be scoped to the parent package.<br/>Example: `MdePkg/Test/UnitTest/[Library/Protocol/Ppi/Guid]/`<br/><br/>A real-world example of this is the BaseSafeIntLib test in MdePkg.<br/>`MdePkg/Test/UnitTest/Library/BaseSafeIntLib/TestBaseSafeIntLibHost.inf`
Host-Based Unit Tests for a Library/Driver (PEI/DXE/SMM) implementation   | If what's being tested is a specific implementation (e.g. BaseDebugLibSerialPort for DebugLib), the test should be scoped to the implementation directory itself, in a UnitTest subdirectory.<br/><br/>Module Example: `MdeModulePkg/Universal/EsrtFmpDxe/UnitTest/`<br/>Library Example: `MdePkg/Library/BaseMemoryLib/UnitTest/`
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

  <Package>Pkg.dsc          # Standard Modules and any Target-Based Test Apps (including in Test/)

```

### Future Locations in Consideration

We don't know if these types will exist or be applicable yet, but if you write a support library or module that matches the following, please make sure they live in the correct place.

Code/Test                                   | Location
---------                                   | --------
Host-Based Library Implementations                 | Host-Based Implementations of common libraries (eg. MemoryAllocationLibHost) should live in the same package that declares the library interface in its .DEC file in the `*Pkg/HostLibrary` directory. Should have 'Host' in the name.
Host-Based Mocks and Stubs  | Mock and Stub libraries should live in the `UefiTestFrameworkPkg/StubLibrary` with either 'Mock' or 'Stub' in the library name.

### If still in doubt...

Hop on GitHub and ask @corthon, @mdkinney, or @spbrogan. ;)

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
