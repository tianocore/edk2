/** @file
  Implement UnitTestResultReportLib doing plain txt out to console

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UnitTestResultReportLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

VOID
EFIAPI
ReportPrint (
  IN CONST CHAR8  *Format,
  ...
  );

VOID
ReportOutput (
  IN CONST CHAR8  *Output
  );

struct _UNIT_TEST_STATUS_STRING {
  UNIT_TEST_STATUS    Status;
  CHAR8               *String;
};

struct _UNIT_TEST_FAILURE_TYPE_STRING {
  FAILURE_TYPE    Type;
  CHAR8           *String;
};

struct _UNIT_TEST_STATUS_STRING  mStatusStrings[] = {
  { UNIT_TEST_PASSED,                     "PASSED"                        },
  { UNIT_TEST_ERROR_PREREQUISITE_NOT_MET, "NOT RUN - PREREQUISITE FAILED" },
  { UNIT_TEST_ERROR_TEST_FAILED,          "FAILED"                        },
  { UNIT_TEST_RUNNING,                    "RUNNING"                       },
  { UNIT_TEST_PENDING,                    "PENDING"                       },
  { 0,                                    "**UNKNOWN**"                   }
};

struct _UNIT_TEST_FAILURE_TYPE_STRING  mFailureTypeStrings[] = {
  { FAILURETYPE_NOFAILURE,         "NO FAILURE"                 },
  { FAILURETYPE_OTHER,             "OTHER FAILURE"              },
  { FAILURETYPE_ASSERTTRUE,        "ASSERT_TRUE FAILURE"        },
  { FAILURETYPE_ASSERTFALSE,       "ASSERT_FALSE FAILURE"       },
  { FAILURETYPE_ASSERTEQUAL,       "ASSERT_EQUAL FAILURE"       },
  { FAILURETYPE_ASSERTNOTEQUAL,    "ASSERT_NOTEQUAL FAILURE"    },
  { FAILURETYPE_ASSERTNOTEFIERROR, "ASSERT_NOTEFIERROR FAILURE" },
  { FAILURETYPE_ASSERTSTATUSEQUAL, "ASSERT_STATUSEQUAL FAILURE" },
  { FAILURETYPE_ASSERTNOTNULL,     "ASSERT_NOTNULL FAILURE"     },
  { FAILURETYPE_EXPECTASSERT,      "EXPECT_ASSERT FAILURE"      },
  { 0,                             "*UNKNOWN* Failure"          }
};

//
// TEST REPORTING FUNCTIONS
//

STATIC
CONST CHAR8 *
GetStringForUnitTestStatus (
  IN UNIT_TEST_STATUS  Status
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mStatusStrings) - 1; Index++) {
    if (mStatusStrings[Index].Status == Status) {
      //
      // Return string from matching entry
      //
      return mStatusStrings[Index].String;
    }
  }

  //
  // Return last entry if no match found.
  //
  return mStatusStrings[Index].String;
}

STATIC
CONST CHAR8 *
GetStringForFailureType (
  IN FAILURE_TYPE  Failure
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mFailureTypeStrings) - 1; Index++) {
    if (mFailureTypeStrings[Index].Type == Failure) {
      //
      // Return string from matching entry
      //
      return mFailureTypeStrings[Index].String;
    }
  }

  //
  // Return last entry if no match found.
  //
  DEBUG ((DEBUG_INFO, "%a Failure Type does not have string defined 0x%X\n", __func__, (UINT32)Failure));
  return mFailureTypeStrings[Index].String;
}

/*
  Method to print the Unit Test run results

  @retval  Success
*/
EFI_STATUS
EFIAPI
OutputUnitTestFrameworkReport (
  IN UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle
  )
{
  UNIT_TEST_FRAMEWORK         *Framework;
  INTN                        Passed;
  INTN                        Failed;
  INTN                        NotRun;
  UNIT_TEST_SUITE_LIST_ENTRY  *Suite;
  UNIT_TEST_LIST_ENTRY        *Test;
  INTN                        SPassed;
  INTN                        SFailed;
  INTN                        SNotRun;

  Passed = 0;
  Failed = 0;
  NotRun = 0;
  Suite  = NULL;

  Framework = (UNIT_TEST_FRAMEWORK *)FrameworkHandle;
  if (Framework == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ReportPrint ("---------------------------------------------------------\n");
  ReportPrint ("------------- UNIT TEST FRAMEWORK RESULTS ---------------\n");
  ReportPrint ("---------------------------------------------------------\n");

  // print the version and time

  //
  // Iterate all suites
  //
  for (Suite = (UNIT_TEST_SUITE_LIST_ENTRY *)GetFirstNode (&Framework->TestSuiteList);
       (LIST_ENTRY *)Suite != &Framework->TestSuiteList;
       Suite = (UNIT_TEST_SUITE_LIST_ENTRY *)GetNextNode (&Framework->TestSuiteList, (LIST_ENTRY *)Suite))
  {
    Test    = NULL;
    SPassed = 0;
    SFailed = 0;
    SNotRun = 0;

    ReportPrint ("/////////////////////////////////////////////////////////\n");
    ReportPrint ("  SUITE: %a\n", Suite->UTS.Title);
    ReportPrint ("   PACKAGE: %a\n", Suite->UTS.Name);
    ReportPrint ("/////////////////////////////////////////////////////////\n");

    //
    // Iterate all tests within the suite
    //
    for (Test = (UNIT_TEST_LIST_ENTRY *)GetFirstNode (&(Suite->UTS.TestCaseList));
         (LIST_ENTRY *)Test != &(Suite->UTS.TestCaseList);
         Test = (UNIT_TEST_LIST_ENTRY *)GetNextNode (&(Suite->UTS.TestCaseList), (LIST_ENTRY *)Test))
    {
      ReportPrint ("*********************************************************\n");
      ReportPrint ("  CLASS NAME: %a\n", Test->UT.Name);
      ReportPrint ("  TEST:    %a\n", Test->UT.Description);
      ReportPrint ("  STATUS:  %a\n", GetStringForUnitTestStatus (Test->UT.Result));
      ReportPrint ("  FAILURE: %a\n", GetStringForFailureType (Test->UT.FailureType));
      ReportPrint ("  FAILURE MESSAGE:\n%a\n", Test->UT.FailureMessage);

      if (Test->UT.Log != NULL) {
        ReportPrint ("  LOG:\n");
        ReportOutput (Test->UT.Log);
      }

      switch (Test->UT.Result) {
        case UNIT_TEST_PASSED:
          SPassed++;
          break;
        case UNIT_TEST_ERROR_TEST_FAILED:
          SFailed++;
          break;
        case UNIT_TEST_PENDING:             // Fall through...
        case UNIT_TEST_RUNNING:             // Fall through...
        case UNIT_TEST_ERROR_PREREQUISITE_NOT_MET:
          SNotRun++;
          break;
        default:
          break;
      }

      ReportPrint ("**********************************************************\n");
    } // End Test iteration

    ReportPrint ("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    ReportPrint ("Suite Stats\n");
    ReportPrint (" Passed:  %d  (%d%%)\n", SPassed, (SPassed * 100)/(SPassed+SFailed+SNotRun));
    ReportPrint (" Failed:  %d  (%d%%)\n", SFailed, (SFailed * 100) / (SPassed + SFailed + SNotRun));
    ReportPrint (" Not Run: %d  (%d%%)\n", SNotRun, (SNotRun * 100) / (SPassed + SFailed + SNotRun));
    ReportPrint ("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    Passed += SPassed;  // add to global counters
    Failed += SFailed;  // add to global counters
    NotRun += SNotRun;  // add to global counters
  }// End Suite iteration

  ReportPrint ("=========================================================\n");
  ReportPrint ("Total Stats\n");
  ReportPrint (" Passed:  %d  (%d%%)\n", Passed, (Passed * 100) / (Passed + Failed + NotRun));
  ReportPrint (" Failed:  %d  (%d%%)\n", Failed, (Failed * 100) / (Passed + Failed + NotRun));
  ReportPrint (" Not Run: %d  (%d%%)\n", NotRun, (NotRun * 100) / (Passed + Failed + NotRun));
  ReportPrint ("=========================================================\n");

  return EFI_SUCCESS;
}
