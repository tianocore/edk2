/** @file
  UnitTestLib APIs to run unit tests using cmocka

  Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <UnitTestFrameworkTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

STATIC UNIT_TEST_FRAMEWORK_HANDLE  mFrameworkHandle = NULL;

UNIT_TEST_FRAMEWORK_HANDLE
GetActiveFrameworkHandle (
  VOID
  )
{
  ASSERT (mFrameworkHandle != NULL);
  return mFrameworkHandle;
}

//
// The currently active test suite
//
UNIT_TEST_SUITE  *mActiveUnitTestSuite = NULL;

void
CmockaUnitTestFunctionRunner (
  void **state
  )
{
  UNIT_TEST            *UnitTest;
  UNIT_TEST_SUITE      *Suite;
  UNIT_TEST_FRAMEWORK  *Framework;

  UnitTest  = (UNIT_TEST *)(*state);
  Suite     = (UNIT_TEST_SUITE *)(UnitTest->ParentSuite);
  Framework = (UNIT_TEST_FRAMEWORK *)(Suite->ParentFramework);

  if (UnitTest->RunTest == NULL) {
    UnitTest->Result = UNIT_TEST_SKIPPED;
  } else {
    UnitTest->Result = UNIT_TEST_RUNNING;
    Framework->CurrentTest = UnitTest;
    UnitTest->Result = UnitTest->RunTest (UnitTest->Context);
    Framework->CurrentTest = NULL;
  }
}

int
CmockaUnitTestSetupFunctionRunner (
  void **state
  )
{
  UNIT_TEST            *UnitTest;
  UNIT_TEST_SUITE      *Suite;
  UNIT_TEST_FRAMEWORK  *Framework;
  UNIT_TEST_STATUS     Result;

  UnitTest  = (UNIT_TEST *)(*state);
  Suite     = (UNIT_TEST_SUITE *)(UnitTest->ParentSuite);
  Framework = (UNIT_TEST_FRAMEWORK *)(Suite->ParentFramework);

  if (UnitTest->Prerequisite == NULL) {
    return 0;
  }

  Framework->CurrentTest = UnitTest;
  Result = UnitTest->Prerequisite (UnitTest->Context);
  Framework->CurrentTest = NULL;

  //
  // Return 0 for success.  Non-zero for error.
  //
  return (int)Result;
}

int
CmockaUnitTestTeardownFunctionRunner (
  void **state
  )
{
  UNIT_TEST            *UnitTest;
  UNIT_TEST_SUITE      *Suite;
  UNIT_TEST_FRAMEWORK  *Framework;

  UnitTest  = (UNIT_TEST *)(*state);
  Suite     = (UNIT_TEST_SUITE *)(UnitTest->ParentSuite);
  Framework = (UNIT_TEST_FRAMEWORK *)(Suite->ParentFramework);

  if (UnitTest->CleanUp != NULL) {
    Framework->CurrentTest = UnitTest;
    UnitTest->CleanUp (UnitTest->Context);
    Framework->CurrentTest = NULL;
  }

  //
  // Print out the log messages - This is a partial solution as it
  // does not get the log into the XML.  Need cmocka changes to support
  // stdout and stderr in their xml format
  //
  if (UnitTest->Log != NULL) {
    print_message("UnitTest: %s - %s\n", UnitTest->Name, UnitTest->Description);
    print_message("Log Output Start\n");
    print_message("%s", UnitTest->Log);
    print_message("Log Output End\n");
  }

  //
  // Return 0 for success.  Non-zero for error.
  //
  return 0;
}

int
CmockaUnitTestSuiteSetupFunctionRunner (
  void **state
  )
{
  if (mActiveUnitTestSuite == NULL) {
    return -1;
  }
  if (mActiveUnitTestSuite->Setup == NULL) {
    return 0;
  }

  mActiveUnitTestSuite->Setup ();
  //
  // Always succeed
  //
  return 0;
}

int
CmockaUnitTestSuiteTeardownFunctionRunner (
  void **state
  )
{
  if (mActiveUnitTestSuite == NULL) {
    return -1;
  }
  if (mActiveUnitTestSuite->Teardown == NULL) {
    return 0;
  }

  mActiveUnitTestSuite->Teardown ();
  //
  // Always succeed
  //
  return 0;
}

STATIC
EFI_STATUS
RunTestSuite (
  IN UNIT_TEST_SUITE  *Suite
  )
{
  UNIT_TEST_LIST_ENTRY  *TestEntry;
  UNIT_TEST             *UnitTest;
  struct CMUnitTest     *Tests;
  UINTN                 Index;

  TestEntry       = NULL;

  if (Suite == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  DEBUG ((DEBUG_VERBOSE, "RUNNING TEST SUITE: %a\n", Suite->Title));
  DEBUG ((DEBUG_VERBOSE, "---------------------------------------------------------\n"));

  //
  // Allocate buffer of CMUnitTest entries
  //
  Tests = AllocateZeroPool (Suite->NumTests * sizeof (struct CMUnitTest));
  ASSERT (Tests != NULL);

  //
  // Populate buffer of CMUnitTest entries
  //
  Index = 0;
  for (TestEntry = (UNIT_TEST_LIST_ENTRY *)GetFirstNode (&(Suite->TestCaseList));
       (LIST_ENTRY *)TestEntry != &(Suite->TestCaseList);
       TestEntry = (UNIT_TEST_LIST_ENTRY *)GetNextNode (&(Suite->TestCaseList), (LIST_ENTRY *)TestEntry)) {
    UnitTest                   = &TestEntry->UT;
    Tests[Index].name          = UnitTest->Description;
    Tests[Index].test_func     = CmockaUnitTestFunctionRunner;
    Tests[Index].setup_func    = CmockaUnitTestSetupFunctionRunner;
    Tests[Index].teardown_func = CmockaUnitTestTeardownFunctionRunner;
    Tests[Index].initial_state = UnitTest;
    Index++;
  }
  ASSERT (Index == Suite->NumTests);

  //
  // Run all unit tests in a test suite
  //
  mActiveUnitTestSuite = Suite;
  _cmocka_run_group_tests (
    Suite->Title,
    Tests,
    Suite->NumTests,
    CmockaUnitTestSuiteSetupFunctionRunner,
    CmockaUnitTestSuiteTeardownFunctionRunner
    );
  mActiveUnitTestSuite = NULL;
  FreePool (Tests);

  return EFI_SUCCESS;
}

/**
  Execute all unit test cases in all unit test suites added to a Framework.

  Once a unit test framework is initialized and all unit test suites and unit
  test cases are registered, this function will cause the unit test framework to
  dispatch all unit test cases in sequence and record the results for reporting.

  @param[in]  FrameworkHandle  A handle to the current running framework that
                               dispatched the test.  Necessary for recording
                               certain test events with the framework.

  @retval  EFI_SUCCESS            All test cases were dispatched.
  @retval  EFI_INVALID_PARAMETER  FrameworkHandle is NULL.
**/
EFI_STATUS
EFIAPI
RunAllTestSuites (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  )
{
  UNIT_TEST_FRAMEWORK         *Framework;
  UNIT_TEST_SUITE_LIST_ENTRY  *Suite;
  EFI_STATUS                  Status;

  Framework = (UNIT_TEST_FRAMEWORK *)FrameworkHandle;
  Suite     = NULL;

  if (Framework == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  DEBUG((DEBUG_VERBOSE, "------------     RUNNING ALL TEST SUITES   --------------\n"));
  DEBUG((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  mFrameworkHandle = FrameworkHandle;

  //
  // Iterate all suites
  //
  for (Suite = (UNIT_TEST_SUITE_LIST_ENTRY *)GetFirstNode (&Framework->TestSuiteList);
    (LIST_ENTRY *)Suite != &Framework->TestSuiteList;
    Suite = (UNIT_TEST_SUITE_LIST_ENTRY *)GetNextNode (&Framework->TestSuiteList, (LIST_ENTRY *)Suite)) {
    Status = RunTestSuite (&(Suite->UTS));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Test Suite Failed with Error.  %r\n", Status));
    }
  }

  mFrameworkHandle = NULL;

  return EFI_SUCCESS;
}
