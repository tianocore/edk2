/** @file
  This is a sample to demonstrate the usage of the Unit Test Library that
  supports the PEI, DXE, SMM, UEFI Shell, and host execution environments.
  This test case generates an exception. For some host-based environments, this
  is a fatal condition that terminates the unit tests and no additional test
  cases are executed. On other environments, this condition may be report a unit
  test failure and continue with additional unit tests.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>

#define UNIT_TEST_NAME     "Sample Unit Test Generate Exception"
#define UNIT_TEST_VERSION  "0.1"

/**
  Unit-Test Test Suite Setup (before) function that enables ASSERT() macros.
**/
VOID
EFIAPI
TestSuiteEnableAsserts (
  VOID
  )
{
  //
  // Set BIT0 (DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED)
  //
  PatchPcdSet8 (PcdDebugPropertyMask, PcdGet8 (PcdDebugPropertyMask) | BIT0);
}

/**
  Unit-Test Test Suite Setup (before) function that disables ASSERT() macros.
**/
VOID
EFIAPI
TestSuiteDisableAsserts (
  VOID
  )
{
  //
  // Clear BIT0 (DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED)
  //
  PatchPcdSet8 (PcdDebugPropertyMask, PcdGet8 (PcdDebugPropertyMask) & (~BIT0));
}

UINTN
DivideWithNoParameterChecking (
  UINTN  Dividend,
  UINTN  Divisor
  )
{
  //
  // Perform integer division with no check for divide by zero
  //
  return (Dividend / Divisor);
}

/**
  Sample unit test the triggers an unexpected exception

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
GenerateUnexpectedException (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Assertion that passes without generating an exception
  //
  UT_ASSERT_EQUAL (DivideWithNoParameterChecking (20, 1), (UINTN)20);
  //
  // Assertion that generates divide by zero exception before result evaluated
  //
  UT_ASSERT_EQUAL (DivideWithNoParameterChecking (20, 0), MAX_UINTN);

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  sample unit tests and run the unit tests.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
EFI_STATUS
EFIAPI
UefiTestMain (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      MacroTestsAssertsEnabled;
  UNIT_TEST_SUITE_HANDLE      MacroTestsAssertsDisabled;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the Macro Tests with ASSERT() enabled
  //
  Status = CreateUnitTestSuite (&MacroTestsAssertsEnabled, Framework, "Macro Tests with ASSERT() enabled", "Sample.MacroAssertsEnabled", TestSuiteEnableAsserts, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MacroTestsAssertsEnabled\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (MacroTestsAssertsEnabled, "Test Unexpected Exception", "GenerateUnexpectedException", GenerateUnexpectedException, NULL, NULL, NULL);

  //
  // Populate the Macro Tests with ASSERT() disabled
  //
  Status = CreateUnitTestSuite (&MacroTestsAssertsDisabled, Framework, "Macro Tests with ASSERT() disabled", "Sample.MacroAssertsDisables", TestSuiteDisableAsserts, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MacroTestsAssertsDisabled\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  AddTestCase (MacroTestsAssertsDisabled, "Test Unexpected Exception", "GenerateUnexpectedException", GenerateUnexpectedException, NULL, NULL, NULL);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

/**
  Standard PEIM entry point for target based unit test execution from PEI.
**/
EFI_STATUS
EFIAPI
PeiEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  return UefiTestMain ();
}

/**
  Standard UEFI entry point for target based unit test execution from DXE, SMM,
  UEFI Shell.
**/
EFI_STATUS
EFIAPI
DxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UefiTestMain ();
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UefiTestMain ();
}
