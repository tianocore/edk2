# GoogleTest - Unit Testing Framework

## About

This unit test framework, called **GoogleTest** is implemented as a set of EDK II libraries. It is one of two unit test frameworks supported by EDK II. Please refer to [the ReadMe](./ReadMe.md) for a comparison of the two.

[GoogleTest](http://google.github.io/googletest/) and can be used to implement host-based unit tests. [GoogleTest on GitHub](https://github.com/google/googletest) is included in the UnitTestFrameworkPkg as a submodule. Use of GoogleTest for target-based unit tests of EDK II components is not supported.
Host-based unit tests that require mocked interfaces can use the mocking infrastructure included with GoogleTest called [gMock](https://github.com/google/googletest/tree/main/googlemock).


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

There's an instructional video that walks through the creation of a new GoogleTest in UEFI:

[![Watch the Video Here](https://img.youtube.com/vi/Ufc3P95MqJE/default.jpg)](https://www.youtube.com/watch?v=Ufc3P95MqJE)

### GoogleTest Requirements - INF

In our INF file, we'll need to bring in the `GoogleTestLib` library. Conveniently, the interface
header for the `GoogleTestLib` is in `UnitTestFrameworkPkg`, so you shouldn't need to depend on any other
packages. As long as your DSC file knows where to find the lib implementation that you want to use,
you should be good to go.

See this example in `SampleGoogleTestHost.inf`...

```inf
[Packages]
  MdePkg/MdePkg.dec
  UnitTestFrameworkPkg/UnitTestFrameworkPkg.dec

[LibraryClasses]
  GoogleTestLib
  BaseLib
  DebugLib
```

Also, if you want your test to automatically be picked up by the Test Runner plugin (found in
BaseTools/Plugin/HostBasedTestRunner and used in stuart_ci_build) , you will need
to make sure that the module `BASE_NAME` contains the word `Test`...

```inf
[Defines]
  BASE_NAME      = SampleGoogleTestHost
```

### GoogleTest Requirements - DSC

In our DSC file, we'll need to bring in the INF file that was just created into the `[Components]`
section so that the unit tests will be built.

See this example in `UnitTestFrameworkPkgHostTest.dsc`...

```dsc
[Components]
  UnitTestFrameworkPkg/Test/GoogleTest/Sample/SampleGoogleTest/SampleGoogleTestHost.inf
```
Also, based on the type of tests that are being created, the associated DSC include file from the
UnitTestFrameworkPkg for Host or Target based tests should also be included at the top of the DSC
file.

```text
!include UnitTestFrameworkPkg/UnitTestFrameworkPkgHost.dsc.inc
```

Lastly, in the case that the test build has specific dependent libraries associated with it,
they should be added in the \<LibraryClasses\> sub-section for the INF file in the
`[Components]` section of the DSC file. Note that it is within this sub-section where you can
control whether the design or mock version of a component is linked into the test exectuable.

See this example in `SecurityPkgHostTest.dsc` where the `SecureBootVariableLib` design is
being tested using mock versions of `UefiRuntimeServicesTableLib`, `PlatformPKProtectionLib`,
and `UefiLib`...

```text
[Components]
  SecurityPkg/Library/SecureBootVariableLib/GoogleTest/SecureBootVariableLibGoogleTest.inf {
    <LibraryClasses>
      SecureBootVariableLib|SecurityPkg/Library/SecureBootVariableLib/SecureBootVariableLib.inf
      UefiRuntimeServicesTableLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.inf
      PlatformPKProtectionLib|SecurityPkg/Test/Mock/Library/GoogleTest/MockPlatformPKProtectionLib/MockPlatformPKProtectionLib.inf
      UefiLib|MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.inf
  }
```
### GoogleTest Requirements - Source Code

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

Now that we've got that squared away, let's look at our 'Main()' function.

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

However, while GoogleTest does not require test suites or test cases to be
registered, there is still one rule within EDK II that currently needs to be
followed. This rule is that all tests for a given GoogleTest application must
be contained within the same source file that contains the `main()` function
shown above. These tests can be written directly in the file or a `#include`
can be used to add them into the file indirectly.

The reason for this is due to EDK II taking the host application INF file and
first compiling all of its source files into a static library. This static
library is then linked into the final host application. The problem with this
method is that only the tests in the object file containing the `main()`
function are linked into the final host application. This is because the other
tests are contained in their own object files within the static library and
they have no symbols in them that the final host application depends on, so
those object files are not linked into the final host application.

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
within it is still the same. There is still a `MockUefiRuntimeServicesTableLib`
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
[subhook](https://github.com/tianocore/edk2-subhook) header and source files. It
is important to note that the use of the mock function macros and the creation
of mock functions requires no knowledge about the SubhookLib. The SubhookLib
library is entirely hidden and encapsulated within FunctionMockLib, and it
is only mentioned here to provide a complete explanation on all the libraries
used in the implementation.

Subhook only supports x86 architecture, so tests that use FunctionMockLib can only be built with x86.

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
| Library or Global Table | MockLibraryName\>.h |
| Protocol | Mock<ProtocolName\>.h |

The below table shows examples for file names with each of the above cases.

| Interface Type | Interface Name | Header File Name |
| :--- | :--- | :--- |
| Library | UefiLib | MockUefiLib.h |
| Global Table (e.g. gRT, gBS, etc.) | UefiRuntimeServicesTableLib | MockUefiRuntimeServicesTableLib.h |
| Protocol | EFI_USB_IO_PROTOCOL | MockRng.h |

Once the header file name is known, the file needs to be created in the proper
location. For internal mock functions, the location is simply the same
GoogleTest directory that contains the INF file that builds the test application.
For external mock functions, the location is within the `Test` directory under the
package where the library, global table, or protocol that is being mocked is
declared. The exact location depends on the interface type and is shown in the
below table.

| Interface Type | Header File Location |
| :--- | :--- |
| Library or Global Table | \<PackageName\>/Test/Mock/Include/GoogleTest/Library |
| Protocol | \<PackageName\>/Test/Mock/Include/GoogleTest/Protocol |

The below table shows examples for file locations with each of the above cases.
All of these files can be found in MdePkg.

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

The naming convention for global tables and protocols is currently being discussed. Points of discussion include removing the use of underscores, use of "Efi," and including the use case, e.g. `Protocol` in the mock function name.

Lastly, when creating mock functions, there are two limitations to be
aware of in gMock that extend into FunctionMockLib.

1. gMock does not support mocking functions that have more than 15 arguments.
2. gMock does not support mocking variadic functions.

#### Mocking Variadic Functions
When possible, we have worked around this limitation by implementing a stub function
inside the mock file. An example can be found in MockUefiBootServicesTableLib.cpp, or below:
```cpp
extern "C" {
  EFI_STATUS
  EFIAPI
  gBS_InstallMultipleProtocolInterfaces (
    IN OUT EFI_HANDLE  *Handle,
    ...
    )
  {
    VA_LIST     Args;
    EFI_STATUS  Status;
    EFI_GUID    *Protocol;
    VOID        *Interface;
    UINTN       Index;

    VA_START (Args, Handle);
    for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
      //
      // If protocol is NULL, then it's the end of the list
      //
      Protocol = VA_ARG (Args, EFI_GUID *);
      if (Protocol == NULL) {
        break;
      }

      Interface = VA_ARG (Args, VOID *);

      //
      // Install it
      //
      Status = gBS_InstallProtocolInterface (Handle, Protocol, EFI_NATIVE_INTERFACE, Interface);
    }

    VA_END (Args);
    return Status;
  }
```
In this example the function being mocked is one that iterates on the argument list to make repeated calls to another function that is already mocked.

With those limitations in mind, that completes the mock function
declarations, and now the mock function definitions for those declarations
can be created.


### FunctionMockLib Mocks - Definition
The definition of mock functions using FunctionMockLib is done
differently for library and service table function mocks. Library mock
functions are built using their own separate INF file and other mock
functions (of a protocol or any other service table with function pointers) are built in the source file of each GoogleTest
that consumes them. This allows for multiple instantiation.
To minimize repeated code, this is done by creating a macro in the mock header file - detailed later in this section.

#### FunctionMockLib Mocks - Definition for Libraries
The definition of library mock functions using the FunctionMockLib macros are done
in source files. The name of the source file is determined by the interface
(such as a library) that is being created for the mock functions.
The rules for naming the file align with the naming of the file for declarations
and are shown in the table below.

| Interface Type | Source File Name |
| :--- | :--- |
| Library | Mock\Mock<LibraryName\>Lib.cpp |
| Global Table (e.g. gRT, gBS, etc.) | Mock\<GlobalTableLibraryName\>Lib.cpp |

The below table shows examples for file names with each of the above cases.

| Interface Type | Interface Name | Source File Name |
| :--- | :--- | :--- |
| Library | UefiLib | MockUefiLib.cpp |
| Global Table (e.g. gRT, gBS, etc.) | UefiRuntimeServicesTableLib | MockUefiRuntimeServicesTableLib.cpp |

Once the source file name is known, the file needs to be created in the proper
location. The location of the source file is aligned with the location for the
header file. For internal mock functions, the location is simply the same
GoogleTest directory that contains the INF file that builds the test application.
For external mock functions, the location is within the `Test` directory under the
package where the library or global table that is being mocked is
declared. The exact location depends on the interface type and is shown in the
below table.

| Interface Type | Source File Location |
| :--- | :--- |
| Library | \<PackageName\>/Test/Mock/Library/GoogleTest/Mock<LibraryName\>Lib |
| Global Table (e.g. gRT, gBS, etc.) | \<PackageName\>/Test/Mock/Library/GoogleTest/Mock<GlobalTableLibraryName\>Lib |

The below table shows examples for file locations with each of the above cases.

| Interface Type | Interface Name | Source File Location |
| :--- | :--- | :--- |
| Library | UefiLib | MdePkg/Test/Mock/Library/GoogleTest/MockUefiLib/MockUefiLib.cpp |
| Global Table (e.g. gRT, gBS, etc.) | UefiRuntimeServicesTableLib | MdePkg/Test/Mock/Library/GoogleTest/MockUefiRuntimeServicesTableLib/MockUefiRuntimeServicesTableLib.cpp |

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

#### FunctionMockLib Mocks - Definition for structures with function pointers
The definitions of mock structures that contain function pointers, or interfaces that don't 
have an inf associated with them (for example, protocols or PPIs) can be in the same header 
file as the declarations.

The rules for naming the file are shown in the table below.

| Interface Type | Header File Location |
| :--- | :--- |
| Protocol | \<PackageName\>/Test/Mock/Include/GoogleTest/Protocol\Mock<ProtocolName\>.h |
| PPI | \<PackageName\>/Test/Mock/Include/GoogleTest/Ppi\Mock<PPiName\>.h |


The below table shows examples for file locations with each of the above cases.

| Interface Type | Interface Name | Header File Location |
| :--- | :--- | :--- |
| Protocol | Rng | MdePkg/Test/Mock/Include/GoogleTest/Protocol/MockRng.h |
| PPI | /MdePkg/Test/Mock/Include/GoogleTest/Ppi/MockPeiReportStatusCodeHandler.h |


Below the struct of mock function declarations, you can use `MOCK_INTERFACE_DEFINITION`
and `MOCK_FUNCTION_DEFINITION`.
For example, when creating the definition of the mock for the `EFI_RNG_PROTOCOL` data
structure, the definitions below are made in `MockRng.h`

```cpp
...

MOCK_INTERFACE_DEFINITION(MockEfiRngProtocol);

MOCK_FUNCTION_DEFINITION(MockEfiRngProtocol, GetInfo, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockEfiRngProtocol, GetRng, 4, EFIAPI);
```

To allow multiple instances of the mocked protocol to be created, use a macro in the same 
header file following the naming convention: `MOCK_<INTERFACE_NAME>_INSTANCE`.

```cpp
...

#define MOCK_EFI_RNG_PROTOCOL_INSTANCE(NAME)  \
  EFI_RNG_PROTOCOL NAME##_INSTANCE = {        \
    GetInfo,                                  \
    GetRng };                                 \
  EFI_RNG_PROTOCOL  *NAME = &NAME##_INSTANCE;
```

Now in a test source file we can instantiate the protocol

```cpp
...
#include <GoogleTest/Protocol/MockRng.h>

MOCK_EFI_RNG_PROTOCOL_INSTANCE (gRngProtocol)

```

And in our test we can use this pointer to mock an assignment from LocateProtocol

```cpp
 EXPECT_CALL (BsMock, gBS_LocateProtocol)
    .WillOnce (
       DoAll (
          SetArgPointee<2> (ByRef (gRngProtocol)),
            Return (EFI_SUCCESS)
      )
    );
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

```text
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

```text
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

```text
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

```text
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

### If still in doubt

Hop on GitHub and ask @mdkinney, or @spbrogan.

## Copyright

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
