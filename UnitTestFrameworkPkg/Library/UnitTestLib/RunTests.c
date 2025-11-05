/**
  UnitTestLib APIs to run unit tests

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestResultReportLib.h>

STATIC UNIT_TEST_FRAMEWORK_HANDLE  mFrameworkHandle = NULL;

BASE_LIBRARY_JUMP_BUFFER  gUnitTestJumpBuffer;

UNIT_TEST_FRAMEWORK_HANDLE
GetActiveFrameworkHandle (
  VOID
  )
{
  return mFrameworkHandle;
}

STATIC
EFI_STATUS
RunTestSuite (
  IN UNIT_TEST_SUITE  *Suite
  )
{
  UNIT_TEST_LIST_ENTRY  *TestEntry;
  UNIT_TEST             *Test;
  UNIT_TEST_FRAMEWORK   *ParentFramework;

  if (Suite == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TestEntry       = NULL;
  ParentFramework = (UNIT_TEST_FRAMEWORK *)Suite->ParentFramework;

  DEBUG ((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  DEBUG ((DEBUG_VERBOSE, "RUNNING TEST SUITE: %a\n", Suite->Title));
  DEBUG ((DEBUG_VERBOSE, "---------------------------------------------------------\n"));

  if (Suite->Setup != NULL) {
    Suite->Setup ();
  }

  //
  // Iterate all tests within the suite
  //
  for (TestEntry = (UNIT_TEST_LIST_ENTRY *)GetFirstNode (&(Suite->TestCaseList));
       (LIST_ENTRY *)TestEntry != &(Suite->TestCaseList);
       TestEntry = (UNIT_TEST_LIST_ENTRY *)GetNextNode (&(Suite->TestCaseList), (LIST_ENTRY *)TestEntry))
  {
    Test                         = &TestEntry->UT;
    ParentFramework->CurrentTest = Test;

    DEBUG ((DEBUG_VERBOSE, "*********************************************************\n"));
    DEBUG ((DEBUG_VERBOSE, " RUNNING TEST: %a:\n", Test->Description));
    DEBUG ((DEBUG_VERBOSE, "**********************************************************\n"));

    //
    // First, check to see whether the test has already been run.
    // NOTE: This would generally only be the case if a saved state was detected and loaded.
    //
    if ((Test->Result != UNIT_TEST_PENDING) && (Test->Result != UNIT_TEST_RUNNING)) {
      DEBUG ((DEBUG_VERBOSE, "Test was run on a previous pass. Skipping.\n"));
      ParentFramework->CurrentTest = NULL;
      continue;
    }

    //
    // Next, if we're still running, make sure that our test prerequisites are in place.
    if ((Test->Result == UNIT_TEST_PENDING) && (Test->Prerequisite != NULL)) {
      DEBUG ((DEBUG_VERBOSE, "PREREQ\n"));
      if (SetJump (&gUnitTestJumpBuffer) == 0) {
        if (Test->Prerequisite (Test->Context) != UNIT_TEST_PASSED) {
          DEBUG ((DEBUG_ERROR, "Prerequisite Not Met\n"));
          Test->Result                 = UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
          ParentFramework->CurrentTest = NULL;
          continue;
        }
      } else {
        DEBUG ((DEBUG_ERROR, "Prerequisite Not Met\n"));
        Test->Result                 = UNIT_TEST_ERROR_PREREQUISITE_NOT_MET;
        ParentFramework->CurrentTest = NULL;
        continue;
      }
    }

    //
    // Now we should be ready to call the actual test.
    // We set the status to UNIT_TEST_RUNNING in case the test needs to reboot
    // or quit. The UNIT_TEST_RUNNING state will allow the test to resume
    // but will prevent the Prerequisite from being dispatched a second time.
    if (SetJump (&gUnitTestJumpBuffer) == 0) {
      Test->Result = UNIT_TEST_RUNNING;
      Test->Result = Test->RunTest (Test->Context);
    } else {
      Test->Result = UNIT_TEST_ERROR_TEST_FAILED;
    }

    //
    // Finally, clean everything up, if need be.
    if (Test->CleanUp != NULL) {
      DEBUG ((DEBUG_VERBOSE, "CLEANUP\n"));
      if (SetJump (&gUnitTestJumpBuffer) == 0) {
        Test->CleanUp (Test->Context);
      }
    }

    //
    // End the test.
    //
    ParentFramework->CurrentTest = NULL;
  }

  if (Suite->Teardown != NULL) {
    Suite->Teardown ();
  }

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

  DEBUG ((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  DEBUG ((DEBUG_VERBOSE, "------------     RUNNING ALL TEST SUITES   --------------\n"));
  DEBUG ((DEBUG_VERBOSE, "---------------------------------------------------------\n"));
  mFrameworkHandle = FrameworkHandle;

  //
  // Iterate all suites
  //
  for (Suite = (UNIT_TEST_SUITE_LIST_ENTRY *)GetFirstNode (&Framework->TestSuiteList);
       (LIST_ENTRY *)Suite != &Framework->TestSuiteList;
       Suite = (UNIT_TEST_SUITE_LIST_ENTRY *)GetNextNode (&Framework->TestSuiteList, (LIST_ENTRY *)Suite))
  {
    Status = RunTestSuite (&(Suite->UTS));
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Test Suite Failed with Error.  %r\n", Status));
    }
  }

  //
  // Save current state so if test is started again it doesn't have to run.  It will just report
  //
  SaveFrameworkState (NULL, 0);
  OutputUnitTestFrameworkReport (FrameworkHandle);

  mFrameworkHandle = NULL;

  return EFI_SUCCESS;
}
