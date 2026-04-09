/** @file
  Unit tests of the UefiSortLib

  Copyright (C) Huawei Technologies Co., Ltd. All rights reserved
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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/UnitTestLib.h>
#include <Library/SortLib.h>

#define UNIT_TEST_APP_NAME     "UefiSortLib Unit Tests"
#define UNIT_TEST_APP_VERSION  "1.0"

#define TEST_ARRAY_SIZE_9  9

/**
  The function is called by PerformQuickSort to compare int values.

  @param[in] Left            The pointer to first buffer.
  @param[in] Right           The pointer to second buffer.

  @retval 0                  Buffer1 equal to Buffer2.
  @return <0                 Buffer1 is less than Buffer2.
  @return >0                 Buffer1 is greater than Buffer2.

**/
INTN
EFIAPI
TestCompareFunction (
  IN CONST VOID  *Left,
  IN CONST VOID  *Right
  )
{
  if (*(UINT32 *)Right > *(UINT32 *)Left) {
    return 1;
  } else if (*(UINT32 *)Right < *(UINT32 *)Left) {
    return -1;
  }

  return 0;
}

/**
  Unit test for PerformQuickSort () API of the UefiSortLib.

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
SortUINT32ArrayShouldSucceed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN   TestCount;
  UINT32  Index;
  UINT32  TestBuffer[TEST_ARRAY_SIZE_9];
  UINT32  TestResult[TEST_ARRAY_SIZE_9];

  TestCount = TEST_ARRAY_SIZE_9;
  for (Index = 0; Index < TEST_ARRAY_SIZE_9; Index++) {
    TestBuffer[Index] = Index + 1;
    TestResult[Index] = TEST_ARRAY_SIZE_9 - Index;
  }

  PerformQuickSort (TestBuffer, TestCount, sizeof (UINT32), (SORT_COMPARE)TestCompareFunction);
  UT_ASSERT_MEM_EQUAL (TestBuffer, TestResult, sizeof (UINT32) * TEST_ARRAY_SIZE_9);

  return UNIT_TEST_PASSED;
}

/**
  Unit test for StringCompare () API of the UefiSortLib.

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
CompareSameBufferShouldSucceed (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  INTN          retval;
  CONST CHAR16  *TestBuffer[] = { L"abcdefg" };

  retval = StringCompare (TestBuffer, TestBuffer);
  UT_ASSERT_TRUE (retval == 0);

  return UNIT_TEST_PASSED;
}

/**
  Initialze the unit test framework, suite, and unit tests for the
  UefiSortLib and run the UefiSortLib unit test.

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
  UNIT_TEST_SUITE_HANDLE      SortTests;

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
  // Populate the UefiSortLib Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&SortTests, Framework, "UefiSortLib Sort Tests", "UefiSortLib.SortLib", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for UefiSortLib API Tests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // --------------Suite--------Description------------Name--------------Function----------------Pre---Post---Context-----------
  //
  AddTestCase (SortTests, "Sort the Array", "Sort", SortUINT32ArrayShouldSucceed, NULL, NULL, NULL);
  AddTestCase (SortTests, "Compare the Buffer", "Compare", CompareSameBufferShouldSucceed, NULL, NULL, NULL);

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

///
/// Avoid ECC error for function name that starts with lower case letter
///
#define UefiSortLibUnitTestMain  main

/**
  Standard POSIX C entry point for host based unit test execution.

  @param[in] Argc  Number of arguments
  @param[in] Argv  Array of pointers to arguments

  @retval 0      Success
  @retval other  Error
**/
INT32
UefiSortLibUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  UnitTestingEntry ();
  return 0;
}
