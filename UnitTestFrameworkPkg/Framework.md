# Framework - Unit Testing Framework

## About

This unit test framework, called **Framework** is implemented as a set of EDK II libraries. It is one of two unit test frameworks supported by EDK II. Please refer to [the ReadMe](./ReadMe.md) for a comparison of the two.

The Framework supports both host-based unit tests and target-based unit tests that share the same
source style, macros, and APIs. In some scenarios, the same unit test case sources can be built
for both host-based unit test execution and target-based unit test execution. Host-based unit tests
that require mocked interfaces can use the mocking infrastructure provided by
[cmocka](https://api.cmocka.org/) that is included in the UnitTestFrameworkPkg as a submodule.

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

Most of these examples will refer to the `SampleUnitTestUefiShell` app found in this package.

### Framework Requirements - INF

In our INF file, we'll need to bring in the `UnitTestLib` library. Conveniently, the interface
header for the `UnitTestLib` is located in `MdePkg`, so you shouldn't need to depend on any other
packages. As long as your DSC file knows where to find the lib implementation that you want to use,
you should be good to go.

See this example in `SampleUnitTestUefiShell.inf`...

```inf
[Packages]
  MdePkg/MdePkg.dec

[LibraryClasses]
  UefiApplicationEntryPoint
  BaseLib
  DebugLib
  UnitTestLib
  PrintLib
```

Also, if you want your test to automatically be picked up by the Test Runner plugin, you will need
to make sure that the module `BASE_NAME` contains the word `Test`...

```inf
[Defines]
  BASE_NAME      = SampleUnitTestUefiShell
```

### Framework Requirements - DSC

In our DSC file, we'll need to bring in the INF file that was just created into the `[Components]`
section so that the unit tests will be built.

See this example in `UnitTestFrameworkPkg.dsc`...

```inf
[Components]
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestUefiShell.inf
```

Also, based on the type of tests that are being created, the associated DSC include file from the
UnitTestFrameworkPkg for Host or Target based tests should also be included at the top of the DSC
file.

```inf
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc
```

Lastly, in the case that the test build has specific dependent libraries associated with it,
they should be added in the \<LibraryClasses\> sub-section for the INF file in the
`[Components]` section of the DSC file.

See this example in `SecurityPkgHostTest.dsc`...

```inf
[Components]
  SecurityPkg/Library/SecureBootVariableLib/UnitTest/SecureBootVariableLibUnitTest.inf {
    <LibraryClasses>
      SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
      UefiRuntimeServicesTableLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiRuntimeServicesTableLib.inf
      PlatformPKProtectionLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockPlatformPKProtectionLib.inf
      UefiLib|SecurityPkg/Library/SecureBootVariableLib/UnitTest/MockUefiLib.inf
  }
```

### Framework Requirements - Code

Not to state the obvious, but let's make sure we have the following include before getting too far along...

```c
#include <Library/UnitTestLib.h>
```

Now that we've got that squared away, let's look at our 'Main()' routine (or DriverEntryPoint() or whatever).

### Framework Configuration

Everything in the UnitTestFrameworkPkg framework is built around an object called -- conveniently -- the Framework.
This Framework object will contain all the information about our test, the test suites and test cases associated
with it, the current location within the test pass, and any results that have been recorded so far.

To get started with a test, we must first create a Framework instance. The function for this is
`InitUnitTestFramework`. It takes in `CHAR8` strings for the long name, short name, and test version.
The long name and version strings are just for user presentation and relatively flexible. The short name
will be used to name any cache files and/or test results, so should be a name that makes sense in that context.
These strings are copied internally to the Framework, so using stack-allocated or literal strings is fine.

In the `SampleUnitTestUefiShell` app, the module name is used as the short name, so the initialization looks like this.

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

Looking at `SampleUnitTestUefiShell` app, you can see that the first test suite is created as below...

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

In `SampleUnitTestUefiShell` app, the first test case is added using the code below...

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
export symbols and capture exceptions, and as such are harder to debug. We have provided a Stuart parameter to force
symbolic debugging to be enabled.

You can run a build by adding the `BLD_*_UNIT_TESTING_DEBUG=TRUE` parameter to enable this build option.

```bash
stuart_ci_build -c .pytool/CISettings.py TOOL_CHAIN_TAG=VS2022 -p MdePkg -t NOOPT BLD_*_UNIT_TESTING_DEBUG=TRUE
```

### Host-Based Support vs Other Tests

The host-based test framework is powered internally by the Cmocka framework. As such, it has abilities
that the target-based tests don't (yet). It would be awesome if this meant that it was a super set of
the target-based tests, and it worked just like the target-based tests but with more features. Unfortunately,
this is not the case. While care has been taken to keep them as close as possible, there are a few known
inconsistencies that we're still ironing out. For example, the logging messages in the target-based tests
are cached internally and associated with the running test case. They can be saved later as part of the
reporting lib. This isn't currently possible with host-based. Only the assertion failures are logged.

We will continue trying to make these as similar as possible.

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

Documentation for Cmocka can be found [here](https://api.cmocka.org/).

## Developing

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
stuart_ci_build -c .pytool/CISettings.py TOOL_CHAIN_TAG=VS2022 -p MdePkg -t NOOPT BLD_*_UNIT_TESTING_DEBUG=TRUE
```

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

### If still in doubt

Ask a question in [edk2 discussions](https://github.com/tianocore/edk2/discussions) or reach out on the [EDK II development mailing list \<`devel@edk2.groups.io`\>](mailto:devel@edk2.groups.io).

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
