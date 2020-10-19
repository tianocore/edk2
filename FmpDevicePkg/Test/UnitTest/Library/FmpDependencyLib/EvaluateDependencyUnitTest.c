/** @file
  Unit tests of EvaluateDependency API in FmpDependencyLib.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpDependencyLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UnitTestLib.h>

#define UNIT_TEST_APP_NAME     "FmpDependencyLib Unit Test Application"
#define UNIT_TEST_APP_VERSION  "1.0"

typedef struct {
    UINT8       *Dependencies;
    UINTN       DependenciesSize;
    BOOLEAN     ExpectedResult;
} BASIC_TEST_CONTEXT;

//
// Image Type ID of FMP device A
//
#define IMAGE_TYPE_ID_1   { 0x97144DFA, 0xEB8E, 0xD14D, {0x8B, 0x4D, 0x39, 0x88, 0x24, 0x96, 0x56, 0x42}}

//
// Image Type ID of FMP device B
//
#define IMAGE_TYPE_ID_2   { 0xA42A7370, 0x433A, 0x684D, {0x9A, 0xA1, 0xDE, 0x62, 0x23, 0x30, 0x6C, 0xF3}}

//
// Device A's version is 0x00000002
// Device B's version is 0x00000003
//
static FMP_DEPEX_CHECK_VERSION_DATA mFmpVersions[] = {
  {IMAGE_TYPE_ID_1, 0x00000002},
  {IMAGE_TYPE_ID_2, 0x00000003}
};

// Valid Dependency Expression 1: (Version(A) > 0x00000001) && (Version(B) >= 0x00000003)
static UINT8 mExpression1[] = {
  EFI_FMP_DEP_PUSH_VERSION, 0x01, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0xFA, 0x4D, 0x14, 0x97, 0x8E, 0xEB, 0x4D, 0xD1, 0x8B, 0x4D, 0x39, 0x88, 0x24, 0x96, 0x56, 0x42,
  EFI_FMP_DEP_GT,
  EFI_FMP_DEP_PUSH_VERSION, 0x03, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0x70, 0x73, 0x2A, 0xA4, 0x3A, 0x43, 0x4D, 0x68, 0x9A, 0xA1, 0xDE, 0x62, 0x23, 0x30, 0x6C, 0xF3,
  EFI_FMP_DEP_GTE,
  EFI_FMP_DEP_AND,
  EFI_FMP_DEP_END
};

// Valid Dependency Expression 2: (Version(A) < 0x00000002) || (Version(B) <= 0x00000003)
static UINT8 mExpression2[] = {
  EFI_FMP_DEP_PUSH_VERSION, 0x02, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0xFA, 0x4D, 0x14, 0x97, 0x8E, 0xEB, 0x4D, 0xD1, 0x8B, 0x4D, 0x39, 0x88, 0x24, 0x96, 0x56, 0x42,
  EFI_FMP_DEP_LT,
  EFI_FMP_DEP_PUSH_VERSION, 0x03, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0x70, 0x73, 0x2A, 0xA4, 0x3A, 0x43, 0x4D, 0x68, 0x9A, 0xA1, 0xDE, 0x62, 0x23, 0x30, 0x6C, 0xF3,
  EFI_FMP_DEP_LTE,
  EFI_FMP_DEP_OR,
  EFI_FMP_DEP_END
};

// Valid Dependency Expression 3: !(Version(A) == 0x0000002)
static UINT8 mExpression3[] = {
  EFI_FMP_DEP_PUSH_VERSION, 0x02, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0xFA, 0x4D, 0x14, 0x97, 0x8E, 0xEB, 0x4D, 0xD1, 0x8B, 0x4D, 0x39, 0x88, 0x24, 0x96, 0x56, 0x42,
  EFI_FMP_DEP_EQ,
  EFI_FMP_DEP_NOT,
  EFI_FMP_DEP_END
};

// Valid Dependency Expression 4: "Test" TRUE && FALSE
static UINT8 mExpression4[] = {
  EFI_FMP_DEP_VERSION_STR, 'T', 'e', 's', 't', '\0',
  EFI_FMP_DEP_TRUE,
  EFI_FMP_DEP_FALSE,
  EFI_FMP_DEP_AND,
  EFI_FMP_DEP_END
};

// Invalid Dependency Expression 1: Invalid Op-code
static UINT8 mExpression5[] = {EFI_FMP_DEP_TRUE, 0xAA, EFI_FMP_DEP_END};

// Invalid Dependency Expression 2: String doesn't end with '\0'
static UINT8 mExpression6[] = {EFI_FMP_DEP_VERSION_STR, 'T', 'e', 's', 't', EFI_FMP_DEP_TRUE, EFI_FMP_DEP_END};

// Invalid Dependency Expression 3: GUID is in invalid size
static UINT8 mExpression7[] = {
  EFI_FMP_DEP_PUSH_VERSION, 0x02, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0xAA, 0xBB, 0xCC, 0xDD,
  EFI_FMP_DEP_GTE,
  EFI_FMP_DEP_END
};

// Invalid Dependency Expression 4: Version is in invalid size
static UINT8 mExpression8[] = {
  EFI_FMP_DEP_PUSH_VERSION, 0x02, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0xDA, 0xCB, 0x25, 0xAC, 0x9E, 0xCD, 0x5E, 0xE2, 0x9C, 0x5E, 0x4A, 0x99, 0x35, 0xA7, 0x67, 0x53,
  EFI_FMP_DEP_GTE,
  EFI_FMP_DEP_END
};

// Invalid Dependency Expression 5: Operand and operator mismatch
static UINT8 mExpression9[] = {EFI_FMP_DEP_TRUE, EFI_FMP_DEP_FALSE, EFI_FMP_DEP_GTE, EFI_FMP_DEP_END};

// Invalid Dependency Expression 6: GUID is NOT FOUND
static UINT8 mExpression10[] = {
  EFI_FMP_DEP_PUSH_VERSION, 0x02, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_PUSH_GUID, 0xDA, 0xCB, 0x25, 0xAC, 0x9E, 0xCD, 0x5E, 0xE2, 0x9C, 0x5E, 0x4A, 0x99, 0x35, 0xA7, 0x67, 0x53,
  EFI_FMP_DEP_GT,
  EFI_FMP_DEP_END
};

// Invalid Dependency Expression 7: Stack underflow
static UINT8 mExpression11[] = {
  EFI_FMP_DEP_PUSH_VERSION, 0x02, 0x00, 0x00, 0x00,
  EFI_FMP_DEP_GT,
  EFI_FMP_DEP_END
};

// ------------------------------------------------Test Depex------Depex Size----------------Expected Result
static BASIC_TEST_CONTEXT   mBasicTestTrue1      = {mExpression1,  sizeof(mExpression1),     TRUE};
static BASIC_TEST_CONTEXT   mBasicTestTrue2      = {mExpression2,  sizeof(mExpression2),     TRUE};
static BASIC_TEST_CONTEXT   mBasicTestFalse1     = {mExpression3,  sizeof(mExpression3),     FALSE};
static BASIC_TEST_CONTEXT   mBasicTestFalse2     = {mExpression4,  sizeof(mExpression4),     FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid1   = {mExpression1,  sizeof(mExpression1) - 1, FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid2   = {mExpression5,  sizeof(mExpression5),     FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid3   = {mExpression6,  sizeof(mExpression6),     FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid4   = {mExpression7,  sizeof(mExpression7),     FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid5   = {mExpression8,  sizeof(mExpression8),     FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid6   = {mExpression9,  sizeof(mExpression9),     FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid7   = {mExpression10, sizeof(mExpression10),    FALSE};
static BASIC_TEST_CONTEXT   mBasicTestInvalid8   = {mExpression11, sizeof(mExpression11),    FALSE};

/**
  Unit test for EvaluateDependency() API of the FmpDependencyLib.

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
STATIC
UNIT_TEST_STATUS
EFIAPI
EvaluateDependencyTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  BASIC_TEST_CONTEXT  *TestContext;
  BOOLEAN             EvaluationResult;
  UINT32              LastAttemptStatus;

  TestContext = (BASIC_TEST_CONTEXT *)Context;

  EvaluationResult = EvaluateDependency (
                       (EFI_FIRMWARE_IMAGE_DEP *)TestContext->Dependencies,
                       TestContext->DependenciesSize,
                       mFmpVersions,
                       sizeof(mFmpVersions)/sizeof(FMP_DEPEX_CHECK_VERSION_DATA),
                       &LastAttemptStatus
                       );

  UT_ASSERT_EQUAL (EvaluationResult, TestContext->ExpectedResult);

  return UNIT_TEST_PASSED;
}

/**
  Initialize the unit test framework, suite, and unit tests for the
  EvaluateDependency API in FmpDependencyLib and run the unit tests.

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
  UNIT_TEST_FRAMEWORK_HANDLE  Fw;
  UNIT_TEST_SUITE_HANDLE      DepexEvalTests;

  Fw = NULL;

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_APP_NAME, UNIT_TEST_APP_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Fw, UNIT_TEST_APP_NAME, gEfiCallerBaseName, UNIT_TEST_APP_VERSION);
  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
      goto EXIT;
  }

  //
  // Populate the Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&DepexEvalTests, Fw, "Evaluate Dependency Test", "FmpDependencyLib.EvaluateDependency", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for DepexEvalTests\n"));
    goto EXIT;
  }

  AddTestCase (DepexEvalTests, "Evaluate to True - 1", "Test1", EvaluateDependencyTest, NULL, NULL, &mBasicTestTrue1);
  AddTestCase (DepexEvalTests, "Evaluate to True - 2", "Test2", EvaluateDependencyTest, NULL, NULL, &mBasicTestTrue2);
  AddTestCase (DepexEvalTests, "Evaluate to False - 1", "Test3", EvaluateDependencyTest, NULL, NULL, &mBasicTestFalse1);
  AddTestCase (DepexEvalTests, "Evaluate to False - 2", "Test4", EvaluateDependencyTest, NULL, NULL, &mBasicTestFalse2);
  AddTestCase (DepexEvalTests, "Error: Non-END-terminated expression", "Test5", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid1);
  AddTestCase (DepexEvalTests, "Error: UNKNOWN Op-Code", "Test6", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid2);
  AddTestCase (DepexEvalTests, "Error: Non-Null-terminated string", "Test7", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid3);
  AddTestCase (DepexEvalTests, "Error: GUID size is not 16", "Test8", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid4);
  AddTestCase (DepexEvalTests, "Error: Version size is not 4", "Test9", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid5);
  AddTestCase (DepexEvalTests, "Error: Operand and operator mismatch", "Test10", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid6);
  AddTestCase (DepexEvalTests, "Error: GUID is NOT FOUND", "Test11", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid7);
  AddTestCase (DepexEvalTests, "Error: Stack Underflow", "Test12", EvaluateDependencyTest, NULL, NULL, &mBasicTestInvalid8);

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Fw);

EXIT:
  if (Fw) {
    FreeUnitTestFramework (Fw);
  }

  return Status;
}

/**
  Standard UEFI entry point for target based unit test execution from UEFI Shell.
**/
EFI_STATUS
EFIAPI
FmpDependencyLibUnitTestAppEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UnitTestingEntry ();
}

/**
  Standard POSIX C entry point for host based unit test execution.
**/
int
main (
  int argc,
  char *argv[]
  )
{
  return UnitTestingEntry ();
}
