/** @file
  Unit tests of the DxeResetSystemLib instance of the ResetSystemLib class

  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/UnitTestLib.h>
#include <Library/ResetSystemLib.h>

#define UNIT_TEST_APP_NAME     "DxeResetSystemLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

/**
  Resets the entire platform.

  @param[in]  ResetType         The type of reset to perform.
  @param[in]  ResetStatus       The status code for the reset.
  @param[in]  DataSize          The size, in bytes, of ResetData.
  @param[in]  ResetData         For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                string, optionally followed by additional binary data.
                                The string is a description that the caller may use to further
                                indicate the reason for the system reset.
                                For a ResetType of EfiResetPlatformSpecific the data buffer
                                also starts with a Null-terminated string that is followed
                                by an EFI_GUID that describes the specific type of reset to perform.
**/
STATIC
VOID
EFIAPI
MockResetSystem (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  check_expected_ptr (ResetType);
  check_expected_ptr (ResetStatus);

  //
  // NOTE: Mocked functions can also return values, but that
  //       is for another demo.
}

///
/// Mock version of the UEFI Runtime Services Table
///
EFI_RUNTIME_SERVICES  MockRuntime = {
  {
    EFI_RUNTIME_SERVICES_SIGNATURE,     // Signature
    EFI_RUNTIME_SERVICES_REVISION,      // Revision
    sizeof (EFI_RUNTIME_SERVICES),      // HeaderSize
    0,                                  // CRC32
    0                                   // Reserved
  },
  NULL,               // GetTime
  NULL,               // SetTime
  NULL,               // GetWakeupTime
  NULL,               // SetWakeupTime
  NULL,               // SetVirtualAddressMap
  NULL,               // ConvertPointer
  NULL,               // GetVariable
  NULL,               // GetNextVariableName
  NULL,               // SetVariable
  NULL,               // GetNextHighMonotonicCount
  MockResetSystem,    // ResetSystem
  NULL,               // UpdateCapsule
  NULL,               // QueryCapsuleCapabilities
  NULL                // QueryVariableInfo
};

/**
  Unit test for ColdReset () API of the ResetSystemLib.

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
ResetColdShouldIssueAColdReset (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  expect_value (MockResetSystem, ResetType, EfiResetCold);
  expect_value (MockResetSystem, ResetStatus, EFI_SUCCESS);

  ResetCold ();

  return UNIT_TEST_PASSED;
}

/**
  Unit test for WarmReset () API of the ResetSystemLib.

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
ResetWarmShouldIssueAWarmReset (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  expect_value (MockResetSystem, ResetType, EfiResetWarm);
  expect_value (MockResetSystem, ResetStatus, EFI_SUCCESS);

  ResetWarm ();

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ResetShutdown () API of the ResetSystemLib.

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
ResetShutdownShouldIssueAShutdown (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  expect_value (MockResetSystem, ResetType, EfiResetShutdown);
  expect_value (MockResetSystem, ResetStatus, EFI_SUCCESS);

  ResetShutdown ();

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ResetPlatformSpecific () API of the ResetSystemLib.

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
ResetPlatformSpecificShouldIssueAPlatformSpecificReset (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  expect_value (MockResetSystem, ResetType, EfiResetPlatformSpecific);
  expect_value (MockResetSystem, ResetStatus, EFI_SUCCESS);

  ResetPlatformSpecific (0, NULL);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for ResetSystem () API of the ResetSystemLib.

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
ResetSystemShouldPassTheParametersThrough (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  expect_value (MockResetSystem, ResetType, EfiResetCold);
  expect_value (MockResetSystem, ResetStatus, EFI_SUCCESS);

  ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);

  expect_value (MockResetSystem, ResetType, EfiResetShutdown);
  expect_value (MockResetSystem, ResetStatus, EFI_SUCCESS);

  ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);

  return UNIT_TEST_PASSED;
}

/**
  Initialze the unit test framework, suite, and unit tests for the
  ResetSystemLib and run the ResetSystemLib unit test.

  @retval  EFI_SUCCESS           All test cases were dispatched.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available to
                                 initialize the unit tests.
**/
STATIC
EFI_STATUS
EFIAPI
UnitTestingEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      ResetTests;

  Framework = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the ResetSytemLib Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&ResetTests, Framework, "DxeResetSystemLib Reset Tests", "ResetSystemLib.Reset", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for ResetTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // --------------Suite-----------Description--------------Name----------Function--------Pre---Post-------------------Context-----------
  //
  AddTestCase (ResetTests, "ResetCold should issue a cold reset", "Cold", ResetColdShouldIssueAColdReset, NULL, NULL, NULL);
  AddTestCase (ResetTests, "ResetWarm should issue a warm reset", "Warm", ResetWarmShouldIssueAWarmReset, NULL, NULL, NULL);
  AddTestCase (ResetTests, "ResetShutdown should issue a shutdown", "Shutdown", ResetShutdownShouldIssueAShutdown, NULL, NULL, NULL);
  AddTestCase (ResetTests, "ResetPlatformSpecific should issue a platform-specific reset", "Platform", ResetPlatformSpecificShouldIssueAPlatformSpecificReset, NULL, NULL, NULL);
  AddTestCase (ResetTests, "ResetSystem should pass all parameters through", "Parameters", ResetSystemShouldPassTheParametersThrough, NULL, NULL, NULL);

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
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  return UnitTestingEntry ();
}
