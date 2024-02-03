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
that may be used to implement, run, and debug unit tests implemented using GoogleTest.

If a component can be tested with host-based unit tests, then GoogleTest is recommended. The MdePkg
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

Also, if you want your test to automatically be picked up by the Test Runner plugin, you will need
to make sure that the module `BASE_NAME` contains the word `Test`...

```
[Defines]
  BASE_NAME      = SampleUnitTestUefiShell
```

### Framework Requirements - DSC

In our DSC file, we'll need to bring in the INF file that was just created into the `[Components]`
section so that the unit tests will be built.

See this example in `UnitTestFrameworkPkg.dsc`...

```
[Components]
  UnitTestFrameworkPkg/Test/UnitTest/Sample/SampleUnitTest/SampleUnitTestUefiShell.inf
```

Also, based on the type of tests that are being created, the associated DSC include file from the
UnitTestFrameworkPkg for Host or Target based tests should also be included at the top of the DSC
file.

```
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgTarget.dsc.inc
```

Lastly, in the case that the test build has specific dependent libraries associated with it,
they should be added in the \<LibraryClasses\> sub-section for the INF file in the
`[Components]` section of the DSC file.

See this example in `SecurityPkgHostTest.dsc`...

```
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

## GoogleTest Libraries

### GoogleTestLib

GoogleTestLib is the core library used for GoogleTest in EDK II. This library is mainly a wrapper
around the GoogleTest and gMock header and source files. So all the standard
[GoogleTest](http://google.github.io/googletest/) and
[gMock](https://github.com/google/googletest/tree/main/googlemock) documentation for writing tests
and using mocks applies.

Additionally, to support the use of some primitive types that are not directly supported by
GoogleTest and gMock (but are needed to allow gMock to be used in EDK II), some custom gMock
actions and matchers were added to GoogleTestLib. These customizations are briefly described in
the following tables.

#### Custom Actions

| Action Name | Similar gMock Generic Action | Usage |
|:--- |:--- |:--- |
| `SetArgBuffer()` | `SetArgPointee()` | Used to set a buffer output argument (such as UINT8*, VOID*, a structure pointer, etc.) with data in an expect call. Can be used in an `EXPECT_CALL()` |

#### Custom Matchers

| Matcher Name | Similar gMock Generic Matcher | Usage |
|:--- |:--- |:--- |
| `BufferEq()` | `Pointee(Eq())` | Used to compare two buffer pointer types (such as UINT8*, VOID*, a structure pointer, etc.). Can be used in an `EXPECT_CALL()`, `EXPECT_THAT()`, or anywhere else a matcher to compare two buffers is needed. |
| `Char16StrEq()` | `Pointee(Eq())` | Used to compare two CHAR16* strings. Can be used in an `EXPECT_CALL()`, `EXPECT_THAT()`, or anywhere else a matcher to compare two CHAR16* strings is needed. |

### FunctionMockLib

FunctionMockLib is the library that allows gMock to be used with free (C style) functions. This
library contains a set of macros that wrap gMock's MOCK_METHOD macro to enable the standard gMock
capabilities to be used with free functions. The details of how this is implemented is outside the
scope of this document, but a brief description of each of the public macros in FunctionMockLib is
described below. A full example of how to use these macros to create a mock is described in a
later section.

In total there are six public macros in FunctionMockLib. Four of the macros are related to creating
the mock functions, and the other two macros are related to creating an interface that is necessary
to contain the mock functions and connect them into the gMock framework.

The macros used to create the interface are...
1. `MOCK_INTERFACE_DECLARATION(MOCK)`
2. `MOCK_INTERFACE_DEFINITION(MOCK)`

These macros both take one argument which is the name of the interface for the mock. The
`MOCK_INTERFACE_DECLARATION` macro is used to declare the interface in the `.h` file and the
`MOCK_INTERFACE_DEFINITION` macro is used to define the interface in the `.cpp` file. For
example, to create a mock for the `UefiLib`, a `MockUefiLib.h` file would be created and the
below code would be added to it.

```cpp
struct MockUefiLib {
  MOCK_INTERFACE_DECLARATION(MockUefiLib);
};
```

Additionally, the below code would be written into a `MockUefiLib.cpp` file.

```cpp
MOCK_INTERFACE_DEFINITION(MockUefiLib);
```

The macros used to create the mock functions are...
1. `MOCK_FUNCTION_DECLARATION(RET_TYPE, FUNC, ARGS)`
2. `MOCK_FUNCTION_DEFINITION(MOCK, FUNC, NUM_ARGS, CALL_TYPE)`
3. `MOCK_FUNCTION_INTERNAL_DECLARATION(RET_TYPE, FUNC, ARGS)`
4. `MOCK_FUNCTION_INTERNAL_DEFINITION(MOCK, FUNC, NUM_ARGS, CALL_TYPE)`

You will notice that there are two sets of macros: one to mock external functions and
another to mock internal functions. Each set of macros has the exact same arguments, but
they are used for slightly different use cases. The details of these different use cases
is described in detail in a later section. For now, we will focus on the usage of the macro
arguments since that is common between them.

The `MOCK_FUNCTION_DECLARATION` macro is used to declare the mock function in the `.h` file,
and it takes three arguments: return type, function name, and argument list. The
`MOCK_FUNCTION_DEFINITION` macro is used to define the mock function in the `.cpp` file,
and it takes four arguments: name of the interface for the mock, function name, number of
arguments the function takes, and calling convention type of the function. For example, to
continue with the `UefiLib` mock example above, the `GetVariable2` and `GetEfiGlobalVariable2`
functions are declared in `UefiLib.h` as shown below.

```cpp
EFI_STATUS
EFIAPI
GetVariable2 (
  IN CONST CHAR16    *Name,
  IN CONST EFI_GUID  *Guid,
  OUT VOID           **Value,
  OUT UINTN          *Size OPTIONAL
  );

EFI_STATUS
EFIAPI
GetEfiGlobalVariable2 (
  IN CONST CHAR16  *Name,
  OUT VOID         **Value,
  OUT UINTN        *Size OPTIONAL
  );
```

To declare mocks for these functions within the previously created `MockUefiLib` interface,
the below code would be added to the `MockUefiLib.h` file. Note that the previously added
interface declaration is also included in the code below for context.

```cpp
struct MockUefiLib {
  MOCK_INTERFACE_DECLARATION (MockUefiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVariable2,
    (IN CONST CHAR16    *Name,
     IN CONST EFI_GUID  *Guid,
     OUT VOID           **Value,
     OUT UINTN          *Size OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetEfiGlobalVariable2,
    (IN CONST CHAR16  *Name,
     OUT VOID         **Value,
     OUT UINTN        *Size OPTIONAL)
    );
};
```

Additionally, the below code would be added into the `MockUefiLib.cpp` file to provide
the definitions for these mock functions. Again, the previously added interface
definition is also included in the code below for context.

```cpp
MOCK_INTERFACE_DEFINITION(MockUefiLib);

MOCK_FUNCTION_DEFINITION(MockUefiLib, GetVariable2, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiLib, GetEfiGlobalVariable2, 3, EFIAPI);
```

That concludes the basic overview on how to use the macros, but a more detailed
description on how to name the mocks, where to put the files, how to build the
mocks, and how to use the mocks is described in detail later.

### SubhookLib

SubhookLib is the library used by FunctionMockLib to implement the macros to
mock internal functions: `MOCK_FUNCTION_INTERNAL_DECLARATION` and
`MOCK_FUNCTION_INTERNAL_DEFINITION`. These macros require the additional
functionality provided by SubhookLib because they create mock functions
for functions that are already defined and compiled within the module being
tested. More detail on this is provided in a later section, but for now it is
sufficient to know that the SubhookLib allows a second definition of the
function to be compiled into the test application and then hooked to during a
test.

This library is mainly a wrapper around the
[subhook](https://github.com/Zeex/subhook) header and source files. It is
important to note that the use of the mock function macros and the creation
of mock functions requires no knowledge about the SubhookLib. The SubhookLib
library is entirely hidden and encapsulated within FunctionMockLib, and it
is only mentioned here to provide a complete explanation on all the libraries
used in the implementation.

## FunctionMockLib Mocks

This section describes the details on how to use the mock function macros in
FunctionMockLib to create mock functions, name them, organize their files,
and build them so that they can be used within GoogleTest tests. The usage
of the mock functions is detailed in a later section while this section
simply details how to create them, build them, and where to put them.

### FunctionMockLib Mocks - External vs. Internal

The first question to ask when creating a mock function is if the function being
mocked is external or internal to the module being tested. This is very important
because the macros in FunctionMockLib used to create the mock function are named
differently for these two use cases. Fortunately, the arguments to these macros
and the usage of the mock functions within the tests are exactly the same.
However, because of the different underlying implementations, two different sets
of macros are used.

A more detailed description of when to use the external vs. internal mock function
macros is in the following sections, but the quick summary is as follows.

* External mock function macros are used to mock functions that are outside the
module being tested and use link-time replacement.
* Internal mock function macros are used to mock functions that are inside the
module being tested and use run-time replacement.

The below table shows which macros to use in these two use cases. However, note that
for the creation of the interface, the same macros are used in both cases.

| Mock Function Use Case | Mock Interface Macros | Mock Function Macros |
|:--- |:--- |:--- |
| External mock functions | `MOCK_INTERFACE_DECLARATION`</br>`MOCK_INTERFACE_DEFINITION` | `MOCK_FUNCTION_DECLARATION`</br>`MOCK_FUNCTION_DEFINITION` |
| Internal mock functions | `MOCK_INTERFACE_DECLARATION`</br>`MOCK_INTERFACE_DEFINITION` | `MOCK_FUNCTION_INTERNAL_DECLARATION`</br>`MOCK_FUNCTION_INTERNAL_DEFINITION` |

#### FunctionMockLib Mocks - External mock function

The external mock function macros are used to create mock function definitions
for a library, global service, or protocol that is defined outside of the module
being tested. These mock function definitions are linked into the test application
instead of linking in the design function definitions. In other words, the
external mock function macros use link-time replacement of the design functions.

The `.h/.cpp` files for these mock functions are created within the package
directory where the library, global table, or protocol that is being mocked is
declared. These files are compiled into their own separate library (using
an INF file) that can be shared and linked into many test applications, but more
on that later.

#### FunctionMockLib Mocks - Internal mock function

The internal mock function macros are used to create mock function definitions
for functions that are defined within the module being tested. These mock
function definitions are compiled into the test application along with the design
function definitions. This is possible because the mock functions are given a
slightly different name during compilation. Then during test execution, the
design function is hooked and replaced with the mock function. In other words,
the internal mock function macros use run-time replacement of the design
functions.

The `.h/.cpp` files for these mock functions are created within the GoogleTest
directory containing the specific tests that are using them. These files are
compiled directly in the GoogleTest INF file that builds the test application,
and they are not shared outside of that GoogleTest directory, but more on that
later.

### FunctionMockLib Mocks - Declaration

The declaration of mock functions using the FunctionMockLib macros are done
in header files. The name of the header file is determined by the interface
(such as a library or a protocol) that is being created for the mock functions.
The rules for naming the file are shown in the table below.

| Interface Type | Header File Name |
| :--- | :--- |
| Library | Mock\<LibraryName\>Lib.h |
| Global Table (e.g. gRT, gBS, etc.) | Mock\<GlobalTableLibraryName\>Lib.h |
| Protocol | Mock\<ProtocolName\>Protocol.h |

The below table shows examples for file names with each of the above cases.

| Interface Type | Interface Name | Header File Name |
| :--- | :--- | :--- |
| Library | UefiLib | MockUefiLib.h |
| Global Table (e.g. gRT, gBS, etc.) | UefiRuntimeServicesTableLib | MockUefiRuntimeServicesTableLib.h |
| Protocol | EFI_USB_IO_PROTOCOL | MockEfiUsbIoProtocol.h |

Once the header file name is known, the file needs to be created in the proper
location. For internal mock functions, the location is simply the same
GoogleTest directory that contains the INF file that builds the test application.
For external mock functions, the location is within the `Test` directory under the
package where the library, global table, or protocol that is being mocked is
declared. The exact location depends on the interface type and is shown in the
below table.

| Interface Type | Header File Location |
| :--- | :--- |
| Library | \<PackageName\>/Test/Mock/Include/GoogleTest/Library |
| Global Table (e.g. gRT, gBS, etc.) | \<PackageName\>/Test/Mock/Include/GoogleTest/Library |
| Protocol | \<PackageName\>/Test/Mock/Include/GoogleTest/Protocol |

The below table shows examples for file locations with each of the above cases.

| Interface Type | Interface Name | Header File Location |
| :--- | :--- | :--- |
| Library | UefiLib | MdePkg/Test/Mock/Include/GoogleTest/Library/MockUefiLib.h |
| Global Table (e.g. gRT, gBS, etc.) | UefiRuntimeServicesTableLib | MdePkg/Test/Mock/Include/GoogleTest/Library/MockUefiRuntimeServicesTableLib.h |
| Protocol | EFI_USB_IO_PROTOCOL | MdePkg/Test/Mock/Include/GoogleTest/Protocol/MockEfiUsbIoProtocol.h |

Now that the file location is known, the contents can be added to it. After the
standard `#ifndef` for a header file is added at the top of the file, the
`GoogleTestLib.h` and `FunctionMockLib.h` files are always added. Following these
includes other EDK II related include files are added and must be wrapped in
`extern "C" {}` because they are C include files. Failure to do this will cause
link errors to occur. Note that a `#include` of the interface being mocked must
also be added. This causes the declarations of the functions being mocked to be
included in the compilation and allows the compilation to verify that the function
signatures of the mock and design functions are identical.

After all the needed includes have been added in the file , a `struct` is declared
using the same name as the header file (which was determined using the rules
above). Within this structure declaration a `MOCK_INTERFACE_DECLARATION` is
added along with a `MOCK_FUNCTION_DECLARATION` (or a
`MOCK_FUNCTION_INTERNAL_DECLARATION` if this interface is for internal mock
functions) for each function in the interface. To build on the examples above,
the complete `MockUefiLib.h` file would be as shown below. Note that for brevity
only the `GetVariable2` and `GetEfiGlobalVariable2` declarations are included in
the example.

```cpp
#ifndef MOCK_UEFI_LIB_H_
#define MOCK_UEFI_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/UefiLib.h>
}

struct MockUefiLib {
  MOCK_INTERFACE_DECLARATION (MockUefiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVariable2,
    (IN CONST CHAR16    *Name,
     IN CONST EFI_GUID  *Guid,
     OUT VOID           **Value,
     OUT UINTN          *Size OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetEfiGlobalVariable2,
    (IN CONST CHAR16  *Name,
     OUT VOID         **Value,
     OUT UINTN        *Size OPTIONAL)
    );
};

#endif
```

In the case of libraries, the function names in the mock declarations
align exactly with the function names in the design. However, in the
case of global tables and protocols, to eliminate possible function
name collisions, the names are adjusted slightly in the mock
declarations as shown in the below table.

| Mock Function Use Case | Design Function Name | Mock Function Name |
| :--- | :--- | :--- |
| Library | GetVariable2 | GetVariable2  |
| Global Table (e.g. gRT, gBS, etc.) | gRT->GetVariable | gRT_GetVariable |
| Protocol | UsbIoProtocol->UsbPortReset | UsbIoProtocol_UsbPortReset |

Lastly, when creating mock functions, there are two limitations to be
aware of in gMock that extend into FunctionMockLib.

1. gMock does not support mocking functions that have more than 15 arguments.
2. gMock does not support mocking variadic functions.

With those limitations in mind, that completes the mock function
declarations, and now the mock function definitions for those declarations
can be created.

### FunctionMockLib Mocks - Definition

The definition of mock functions using the FunctionMockLib macros are done
in source files. The name of the source file is determined by the interface
(such as a library or a protocol) that is being created for the mock functions.
The rules for naming the file align with the naming of the file for declarations
and are shown in the table below.

| Interface Type | Source File Name |
| :--- | :--- |
| Library | Mock\<LibraryName\>Lib.cpp |
| Global Table (e.g. gRT, gBS, etc.) | Mock\<GlobalTableLibraryName\>Lib.cpp |
| Protocol | Mock\<ProtocolName\>Protocol.cpp |

The below table shows examples for file names with each of the above cases.

| Interface Type | Interface Name | Source File Name |
| :--- | :--- | :--- |
| Library | UefiLib | MockUefiLib.cpp |
| Global Table (e.g. gRT, gBS, etc.) | UefiRuntimeServicesTableLib | MockUefiRuntimeServicesTableLib.cpp |
| Protocol | EFI_USB_IO_PROTOCOL | MockEfiUsbIoProtocol.cpp |

Once the source file name is known, the file needs to be created in the proper
location. The location of the source file is aligned with the location for the
header file. For internal mock functions, the location is simply the same
GoogleTest directory that contains the INF file that builds the test application.
For external mock functions, the location is within the `Test` directory under the
package where the library, global table, or protocol that is being mocked is
declared. The exact location depends on the interface type and is shown in the
below table.

| Interface Type | Source File Location |
| :--- | :--- |
| Library | \<PackageName\>/Test/Mock/Library/GoogleTest/Mock\<LibraryName\>Lib |
| Global Table (e.g. gRT, gBS, etc.) | \<PackageName\>/Test/Mock/Library/GoogleTest/Mock\<GlobalTableLibraryName\>Lib |
| Protocol | \<PackageName\>/Test/Mock/Library/GoogleTest/Mock\<ProtocolName\>Protocol |

The below table shows examples for file locations with each of the above cases.

| Interface Type | Interface Name | Source File Location |
| :--- | :--- | :--- |
| Library | UefiLib | MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.cpp |
| Global Table (e.g. gRT, gBS, etc.) | UefiRuntimeServicesTableLib | MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.cpp |
| Protocol | EFI_USB_IO_PROTOCOL | MdePkg/Test/Mock/Library/GoogleTest/MockEfiUsbIoProtocol/MockEfiUsbIoProtocol.cpp |

Now that the file location is known, the contents can be added to it. At the top
of the file, the header file containing the mock function declarations is always
added. After this `#include`, the interface definition is created using
`MOCK_INTERFACE_DEFINITION` with the interface name that was used in the mock
function declaration header file. A `MOCK_FUNCTION_DEFINITION` is then added (or
a `MOCK_FUNCTION_INTERNAL_DEFINITION` if this interface is for internal mock
functions) for each function that was declared in the interface. To build on the
prior declaration examples, the complete `MockUefiLib.cpp` file would be as shown
below. Note that for brevity only the `GetVariable2` and `GetEfiGlobalVariable2`
definitions are included in the example.

```cpp
#include <GoogleTest/Library/MockUefiLib.h>

MOCK_INTERFACE_DEFINITION(MockUefiLib);

MOCK_FUNCTION_DEFINITION(MockUefiLib, GetVariable2, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiLib, GetEfiGlobalVariable2, 3, EFIAPI);
```

When creating the defintions, there are a few things to keep in mind.

First, when using `MOCK_FUNCTION_DEFINITION`, some functions being mocked do
not specify a calling convention. In this case, it is fine to leave the last
argument of `MOCK_FUNCTION_DEFINITION` empty. For example, if `GetVariable2`
did not specify the `EFIAPI` calling convention in its declaration, then the
below code would be used for the mock function definition.

```cpp
MOCK_FUNCTION_DEFINITION(MockUefiLib, GetVariable2, 4, );
```

Second, the function name used in `MOCK_FUNCTION_DEFINITION` must align with
the function name used in the associated `MOCK_FUNCTION_DECLARATION` in the
header file.

Last, if the interface is mocking a global table or protocol, then the structure
of function pointers for that interface must also be defined within the source
file as a `static` structure with the mock function definitions being assigned
to the associated entries in the structure. The address of this `static`
structure is then assigned to the global table or protocol pointer. Note that
this pointer must be wrapped in `extern "C" {}` because it needs C style
linkage. Failure to do this will cause link errors to occur. For example, when
creating the definition of the mock for the global runtime services table, the
complete `MockUefiRuntimeServicesTableLib.cpp` file would be as shown below.
Note that for brevity only the `GetVariable` and `SetVariable` definitions are
included in the example.

```cpp
#include <GoogleTest/Library/MockUefiRuntimeServicesTableLib.h>

MOCK_INTERFACE_DEFINITION(MockUefiRuntimeServicesTableLib);

MOCK_FUNCTION_DEFINITION(MockUefiRuntimeServicesTableLib, gRT_GetVariable, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiRuntimeServicesTableLib, gRT_SetVariable, 5, EFIAPI);

static EFI_RUNTIME_SERVICES localRt = {
  {0},              // EFI_TABLE_HEADER

  NULL,             // EFI_GET_TIME
  NULL,             // EFI_SET_TIME
  NULL,             // EFI_GET_WAKEUP_TIME
  NULL,             // EFI_SET_WAKEUP_TIME

  NULL,             // EFI_SET_VIRTUAL_ADDRESS_MAP
  NULL,             // EFI_CONVERT_POINTER

  gRT_GetVariable,  // EFI_GET_VARIABLE
  NULL,             // EFI_GET_NEXT_VARIABLE_NAME
  gRT_SetVariable,  // EFI_SET_VARIABLE

  NULL,             // EFI_GET_NEXT_HIGH_MONO_COUNT
  NULL,             // EFI_RESET_SYSTEM

  NULL,             // EFI_UPDATE_CAPSULE
  NULL,             // EFI_QUERY_CAPSULE_CAPABILITIES

  NULL,             // EFI_QUERY_VARIABLE_INFO
};

extern "C" {
  EFI_RUNTIME_SERVICES* gRT = &localRt;
}
```

That completes the mock function definitions. So now these mock function
definitions can be compiled.

### FunctionMockLib Mocks - Build

The building of mock functions using FunctionMockLib is done slightly
differently for external and internal function mocks. External mock
functions are built using their own separate INF file and internal mock
functions are built as source files directly referenced in the GoogleTest
INF file that builds the test application.

#### FunctionMockLib Mocks - Build External Mock Functions

The building of external mock functions is done using their own separate INF
file which is placed in the same location as the associated source file
containing the mock function definitions. The name of the INF file is exactly
the same as the mock function definitions file, but uses the `.inf` extension
rather than `.cpp`.

Within the `.inf` file the `BASE_NAME` should be set to the same name as the
file (minus the extension), the `MODULE_TYPE` should be set to
`HOST_APPLICATION`, and the `LIBRARY_CLASS` should be the same as the
`BASE_NAME` but without the `Mock` prefix.

The `[Sources]` section will contain the single mock function definition
source file, the `[Packages]` section will contain all the necessary DEC
files to compile the mock functions (which at a minimum will include the
`UnitTestFrameworkPkg.dec` file), the `[LibraryClasses]` section will contain
the `GoogleTestLib`, and the `[BuildOptions]` will need to append the `/EHsc`
compilation flag to all MSFT builds to enable proper use of the C++ exception
handler. Below is the complete `MockUefiLib.inf` as an example.

```
[Defines]
  INF_VERSION                    = 0x00010005
  BASE_NAME                      = MockUefiLib
  FILE_GUID                      = 47211F7A-6D90-4DFB-BDF9-610B69197C2E
  MODULE_TYPE                    = HOST_APPLICATION
  VERSION_STRING                 = 1.0
  LIBRARY_CLASS                  = UefiLib

#
# The following information is for reference only and not required by the build tools.
#
#  VALID_ARCHITECTURES           = IA32 X64
#

[Sources]
  MockUefiLib.cpp

[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[LibraryClasses]
  GoogleTestLib

[BuildOptions]
  MSFT:*_*_*_CC_FLAGS = /EHsc
```

To ensure that this specific set of mock functions are always buildable even
if no test uses it yet, this created INF file needs to be added into the
`[Components]` section of the associated `Test` DSC file for the package in
which this INF file resides. For example, the above `MockUefiLib.inf` would
need to be added to the `MdePkg/Test/MdePkgHostTest.dsc` file as shown below.

```
[Components]
  MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
```

This created INF file will also be referenced within the necessary `Test` DSC
files in order to include the mock function definitions in the test
applications which use this set of mock functions, but more on that later.

One small additional requirement is that if this INF file is added into a
package that does not yet have any other external mock functions in it, then
that package's DEC file will need to have the mock include directory (more
specifically the `Test/Mock/Include` directory) added to its `[Includes]`
section so that test files who want to use the mock functions will be able to
locate the mock function header file. For example, if `MockUefiLib.inf` were
the first mock added to the `MdePkg`, then the below snippet would need to be
added to the `MdePkg.dec` file.

```
[Includes]
  Test/Mock/Include
```

#### FunctionMockLib Mocks - Build Internal Mock Functions

The building of internal mock functions is done using the GoogleTest INF file
that already needs to exist to build the test application. This is easy to
manage since the source and header files for the internal mock functions are
also located in the same GoogleTest directory as the GoogleTest INF file that
will reference them.

The only additions that are required to the GoogleTest INF file are that the
mock function definitions file be added to the `[Sources]` section, the
`UnitTestFrameworkPkg.dec` file be added to the `[Packages]` section, and the
`GoogleTestLib` and `SubhookLib` be added to the `[LibraryClasses]` section.
Below is a minimal contrived example for a `MyModuleGoogleTest.inf` that uses a
`MockMyModuleInternalFunctions.cpp` source file for its internal mock functions.

```
[Defines]
  INF_VERSION         = 0x00010017
  BASE_NAME           = MyModuleGoogleTest
  FILE_GUID           = 814B09B9-2D51-4786-8A77-2E10CD1C55F3
  VERSION_STRING      = 1.0
  MODULE_TYPE         = HOST_APPLICATION

#
# The following information is for reference only and not required by the build tools.
#
#  VALID_ARCHITECTURES           = IA32 X64
#

[Sources]
  MyModuleGoogleTest.cpp
  MockMyModuleInternalFunctions.cpp

[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[LibraryClasses]
  GoogleTestLib
  SubhookLib
```

## GoogleTest Samples

There is a sample unit test provided as both an example of how to write a unit test and leverage
many of the GoogleTest features. This sample can be found in the `Test/GoogleTest/Sample/SampleGoogleTest`
directory.

The sample is provided for the HOST_APPLICATION build type, which can be run on a host system without
needing a target.

There is also a sample unit test provided as both an example of how to write a unit test with
mock functions and leverage some of the gMock features. This sample can be found in the
`SecurityPkg/Library/SecureBootVariableLib/GoogleTest` directory.

It too is provided for the HOST_APPLICATION build type, which can be run on a host system without
needing a target.

## GoogleTest Usage

This section is built a lot like a "Getting Started". We'll go through some of the components that are needed
when constructing a unit test and some of the decisions that are made by the test writer. We'll also describe
how to check for expected conditions in test cases and a bit of the logging characteristics.

Most of these examples will refer to the `SampleGoogleTestHost` app found in this package, but
the examples related to mock functions will refer to the `SecureBootVariableLibGoogleTest` app
found in the `SecurityPkg`.

### GoogleTest Requirements - INF

In our INF file, we'll need to bring in the `GoogleTestLib` library. Conveniently, the interface
header for the `GoogleTestLib` is in `UnitTestFrameworkPkg`, so you shouldn't need to depend on any other
packages. As long as your DSC file knows where to find the lib implementation that you want to use,
you should be good to go.

See this example in `SampleGoogleTestHost.inf`...

```
[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[LibraryClasses]
  GoogleTestLib
  BaseLib
  DebugLib
```

Also, if you want your test to automatically be picked up by the Test Runner plugin, you will need
to make sure that the module `BASE_NAME` contains the word `Test`...

```
[Defines]
  BASE_NAME      = SampleGoogleTestHost
```

### GoogleTest Requirements - DSC

In our DSC file, we'll need to bring in the INF file that was just created into the `[Components]`
section so that the unit tests will be built.

See this example in `UnitTestFrameworkPkgHostTest.dsc`...

```
[Components]
  UnitTestFrameworkPkg/Test/GoogleTest/Sample/SampleGoogleTest/SampleGoogleTestHost.inf
```

Also, based on the type of tests that are being created, the associated DSC include file from the
UnitTestFrameworkPkg for Host or Target based tests should also be included at the top of the DSC
file.

```
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc
```

Lastly, in the case that the test build has specific dependent libraries associated with it,
they should be added in the \<LibraryClasses\> sub-section for the INF file in the
`[Components]` section of the DSC file. Note that it is within this sub-section where you can
control whether the design or mock version of a component is linked into the test exectuable.

See this example in `SecurityPkgHostTest.dsc` where the `SecureBootVariableLib` design is
being tested using mock versions of `UefiRuntimeServicesTableLib`, `PlatformPKProtectionLib`,
and `UefiLib`...

```
[Components]
  SecurityPkg/Library/SecureBootVariableLib/GoogleTest/SecureBootVariableLibGoogleTest.inf {
    <LibraryClasses>
      SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
      PlatformPKProtectionLib|SecurityPkg/Test/Mock/Library/GoogleTest/MockPlatformPKProtectionLib/MockPlatformPKProtectionLib.inf
      UefiLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
  }
```

### GoogleTest Requirements - Code

GoogleTest applications are implemented in C++, so make sure that your test file has
a `.cpp` extension. With that behind us, not to state the obvious, but let's make sure
we have the following includes before getting too far along in the file...

```cpp
#include <Library/GoogleTestLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
}
```

The first include brings in the GoogleTest definitions. Other EDK II related include
files must be wrapped in `extern "C" {}` because they are C include files. Link
failures will occur if this is not done.

Also, when using GoogleTest it is helpful to add a `using` declaration for its
`testing` namespace. This `using` statement greatly reduces the amount of code you
need to write in the tests when referencing the utilities within the `testing`
namespace. For example, instead of writing `::testing::Return` or `::testing::Test`,
you can just write `Return` or `Test` respectively, and these types of references
occur numerous times within the tests.

Lastly, in the case that tests within a GoogleTest application require the usage of
mock functions, it is also necessary to include the header files for those interfaces
as well. As an example, the `SecureBootVariableLibGoogleTest` uses the mock versions
of `UefiLib` and `UefiRuntimeServicesTableLib`. So its test file contains the below
includes. Note that the `using` declaration mentioned above is also shown in the code
below for completeness of the example.

```cpp
#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiLib.h>
#include <GoogleTest/Library/MockUefiRuntimeServicesTableLib.h>

extern "C" {
  #include <Uefi.h>
  ...
}

using namespace testing;
```

Now that we've got that squared away, let's look at our 'Main()' routine (or DriverEntryPoint() or whatever).

### GoogleTest Configuration

Unlike the Framework, GoogleTest does not require test suites or test cases to
be registered. Instead, the test cases declare the test suite name and test
case name as part of their implementation. The only requirement for GoogleTest
is to have a `main()` function that initializes the GoogleTest infrastructure
and calls the service `RUN_ALL_TESTS()` to run all the unit tests.

```cpp
int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
```

### GoogleTest - A Simple Test Case

Below is a sample test case from `SampleGoogleTestHost`.

```cpp
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

For most `ASSERT` macros in GoogleTest there is also an equivalent `EXPECT` macro. Both macro versions
will ultimately cause the `TEST` to fail if the check fails. However, the difference between the two
macro versions is that when the check fails, the `ASSERT` version immediately returns from the `TEST`
while the `EXPECT` version continues running the `TEST`.

There is no return status from a GooglTest unit test. If no assertions (or expectations) are
triggered then the unit test has a passing status.

### GoogleTest - A gMock Test Case

Below is a sample test case from `SecureBootVariableLibGoogleTest`. Although
actually, the test case is not written exactly like this in the test file, but
more on that in a bit.

```cpp
TEST(SetSecureBootModeTest, SetVarError) {
  MockUefiRuntimeServicesTableLib RtServicesMock;
  UINT8                           SecureBootMode;
  EFI_STATUS                      Status;

  // Any random magic number can be used for these tests
  SecureBootMode = 0xAB;

  EXPECT_CALL(RtServicesMock, gRT_SetVariable)
    .WillOnce(Return(EFI_INVALID_PARAMETER));

  Status = SetSecureBootMode(SecureBootMode);
  EXPECT_EQ(Status, EFI_INVALID_PARAMETER);
}
```

Keep in mind that this test is written to verify that `SetSecureBootMode()` will
return `EFI_INVALID_PARAMETER` when the call to `gRT->SetVariable()` within the
implementation of `SetSecureBootMode()` returns `EFI_INVALID_PARAMETER`. With that
in mind, let's discuss how a mock function is used to accomplish this in the test.

In this test case, the `MockUefiRuntimeServicesTableLib` interface is instantiated as
`RtServicesMock` which enables its associated mock functions. These interface
instantiations that contain the mock functions are very important for mock function
based unit tests because without these instantiations, the mock functions within that
interface will not exist and can not be used.

The next line of interest is the `EXPECT_CALL`, which is a standard part of the gMock
framework. This macro is telling the test that a call is expected to occur to a
specific function on a specific interface. The first argument is the name of the
interface object that was instantiated in this test, and the second argument is the
name of the mock function within that interface that is expected to be called. The
`WillOnce(Return(EFI_INVALID_PARAMETER))` associated with this `EXPECT_CALL` states
that the `gRT_SetVariable()` function (remember from earlier in this documentation
that this refers to the `gRT->SetVariable()` function) will be called once during
this test, and when it does get called, we want it to return `EFI_INVALID_PARAMETER`.

Once this `EXPECT_CALL` has been setup, the call to `SetSecureBootMode()` occurs in
the test, and its return value is saved in `Status` so that it can be tested. Based
on the `EXPECT_CALL` that was setup earlier, when `SetSecureBootMode()` internally
calls `gRT->SetVariable()`, it returns `EFI_INVALID_PARAMETER`. This value should
then be returned by `SetSecureBootMode()`, and the
`EXPECT_EQ(Status, EFI_INVALID_PARAMETER)` verifies this is the case.

There is much more that can be done with `EXPECT_CALL` and mock functions, but we
will leave those details to be explained in the gMock documentation.

Now it was mentioned earlier that this test case is not written exactly like this
in the test file, and the next section describes how this test is slightly
refactored to reduce the total amount of code in the entire test suite.

### GoogleTest - A gMock Test Case (refactored)

The sample test case from `SecureBootVariableLibGoogleTest` in the prior section is
actually written as shown below.

```cpp
class SetSecureBootModeTest : public Test {
  protected:
    MockUefiRuntimeServicesTableLib RtServicesMock;
    UINT8       SecureBootMode;
    EFI_STATUS  Status;

    void SetUp() override {
      // Any random magic number can be used for these tests
      SecureBootMode = 0xAB;
    }
};

TEST_F(SetSecureBootModeTest, SetVarError) {
  EXPECT_CALL(RtServicesMock, gRT_SetVariable)
    .WillOnce(Return(EFI_INVALID_PARAMETER));

  Status = SetSecureBootMode(SecureBootMode);
  EXPECT_EQ(Status, EFI_INVALID_PARAMETER);
}
```

This code may at first seem more complicated, but you will notice that the code
with in it is still the same. There is still a `MockUefiRuntimeServicesTableLib`
instantiation, there is still a `SecureBootMode` and `Status` variable defined,
there is still an `EXPECT_CALL`, and etc. However, the benefit of constructing
the test this way is that the new `TEST_F()` requires less code than the prior
`TEST()`.

This is made possible by the usage of what GoogleTest calls a _test fixture_.
This concept of a test fixture allows multiple tests to use (or more specifically
inherit from a base class) a common set of variables and initial conditions.
Notice that using `TEST_F()` requires the first argument to be a name that aligns
with a test fixture (in this case `SetSecureBootModeTest`), and the second
argument is the name of the test (just like in the `TEST()` macro).

All `TEST_F()` tests that use a specific test fixture can be thought of as having
all of that test fixture's variables automatically defined in the test as well as
having that text fixture's `SetUp()` function called before entering the test.

This means that another `TEST_F()` can be written without needing to worry about
defining a bunch of variables or instantiating a bunch of interfaces for mock
functions. For example, the below test (also in `SecureBootVariableLibGoogleTest`)
uses the same test fixture and makes use of its `RtServicesMock`, `Status`, and
`SecureBootMode` variables.

```cpp
TEST_F(SetSecureBootModeTest, PropogateModeToSetVar) {
  EXPECT_CALL(RtServicesMock,
    gRT_SetVariable(
      Char16StrEq(EFI_CUSTOM_MODE_NAME),
      BufferEq(&gEfiCustomModeEnableGuid, sizeof(EFI_GUID)),
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof(SecureBootMode),
      BufferEq(&SecureBootMode, sizeof(SecureBootMode))))
    .WillOnce(Return(EFI_SUCCESS));

  Status = SetSecureBootMode(SecureBootMode);
  EXPECT_EQ(Status, EFI_SUCCESS);
}
```

The biggest benefit is that the `TEST_F()` code can now focus on what is being
tested and not worry about any repetitive setup. There is more that can be done
with test fixtures, but we will leave those details to be explained in the
gMock documentation.

Now, as for what is in the above test, it is slightly more complicated than the
first test. So let's explain this added complexity and what it is actually
testing. In this test, there is still an `EXPECT_CALL` for the
`gRT_SetVariable()` function. However, in this test we are stating that we
expect the input arguments passed to `gRT_SetVariable()` be specific values.
The order they are provided in the `EXPECT_CALL` align with the order of the
arguments in the `gRT_SetVariable()` function. In this case the order of the
`gRT_SetVariable()` arguments is as shown below.

```cpp
IN  CHAR16                       *VariableName,
IN  EFI_GUID                     *VendorGuid,
IN  UINT32                       Attributes,
IN  UINTN                        DataSize,
IN  VOID                         *Data
```

So in the `EXPECT_CALL` we are stating that the call to `gRT_SetVariable()`
will be made with the below input argument values.

1. `VariableName` is equal to the `EFI_CUSTOM_MODE_NAME` string
2. `VendorGuid` is equal to the `gEfiCustomModeEnableGuid` GUID (which has a byte length of `sizeof(EFI_GUID)`)
3. `Attributes` is equal to `EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS`
4. `DataSize` is equal to `sizeof(SecureBootMode)`
5. `Data` is equal to `SecureBootMode` (which has a byte length of `sizeof(SecureBootMode)`)

If any one of these input arguments does not match in the actual call to
`gRT_SetVariable()` in the design, then the test will fail. There is much more
that can be done with `EXPECT_CALL` and mock functions, but again we will
leave those details to be explained in the gMock documentation.

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

For Windows, this is primarily leveraged for pipeline builds, but this can be leveraged locally using the
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

Hop on GitHub and ask @corthon, @mdkinney, or @spbrogan. ;)

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
