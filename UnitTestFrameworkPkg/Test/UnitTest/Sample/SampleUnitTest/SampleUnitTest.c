/** @file
  This is a sample to demostrate the usage of the Unit Test Library that
  supports the PEI, DXE, SMM, UEFI SHell, and host execution environments.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiPei.h>
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PrintLib.h>

#define UNIT_TEST_NAME     "Sample Unit Test"
#define UNIT_TEST_VERSION  "0.1"

///
/// Global variables used in unit tests
///
BOOLEAN  mSampleGlobalTestBoolean  = FALSE;
VOID     *mSampleGlobalTestPointer = NULL;

/**
  Sample Unit-Test Prerequisite Function that checks to make sure the global
  pointer used in the test is already set to NULL.

  Functions with this prototype are registered to be dispatched by the unit test
  framework prior to a given test case. If this prereq function returns
  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET, the test case will be skipped.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED                      Unit test case prerequisites
                                                 are met.
  @retval  UNIT_TEST_ERROR_PREREQUISITE_NOT_MET  Test case should be skipped.

**/
UNIT_TEST_STATUS
EFIAPI
MakeSureThatPointerIsNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UT_ASSERT_EQUAL ((UINTN)mSampleGlobalTestPointer, (UINTN)NULL);
  return UNIT_TEST_PASSED;
}

/**
  Sample Unit-Test Cleanup (after) function that resets the global pointer to
  NULL.

  Functions with this prototype are registered to be dispatched by the
  unit test framework after a given test case. This will be called even if the
  test case returns an error, but not if the prerequisite fails and the test is
  skipped.  The purpose of this function is to clean up any global state or
  test data.

  @param[in]  Context    [Optional] An optional parameter that enables:
                         1) test-case reuse with varied parameters and
                         2) test-case re-entry for Target tests that need a
                         reboot.  This parameter is a VOID* and it is the
                         responsibility of the test author to ensure that the
                         contents are well understood by all test cases that may
                         consume it.

  @retval  UNIT_TEST_PASSED                Test case cleanup succeeded.
  @retval  UNIT_TEST_ERROR_CLEANUP_FAILED  Test case cleanup failed.

**/
VOID
EFIAPI
ClearThePointer (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  mSampleGlobalTestPointer = NULL;
}

/**
  Sample unit test that verifies the expected result of an unsigned integer
  addition operation.

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
OnePlusOneShouldEqualTwo (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINTN  A;
  UINTN  B;
  UINTN  C;

  A = 1;
  B = 1;
  C = A + B;

  UT_ASSERT_EQUAL (C, 2);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test that verifies that a global BOOLEAN is updatable.

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
GlobalBooleanShouldBeChangeable (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  mSampleGlobalTestBoolean = TRUE;
  UT_ASSERT_TRUE (mSampleGlobalTestBoolean);

  mSampleGlobalTestBoolean = FALSE;
  UT_ASSERT_FALSE (mSampleGlobalTestBoolean);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test that logs a warning message and verifies that a global
  pointer is updatable.

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
GlobalPointerShouldBeChangeable (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Example of logging.
  //
  UT_LOG_WARNING ("About to change a global pointer! Current value is 0x%X\n", mSampleGlobalTestPointer);

  mSampleGlobalTestPointer = (VOID *)-1;
  UT_ASSERT_EQUAL ((UINTN)mSampleGlobalTestPointer, (UINTN)((VOID *)-1));
  return UNIT_TEST_PASSED;
}

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

/**
  Sample unit test using the UT_ASSERT_TRUE() macro.

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
MacroUtAssertTrue (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64  Result;

  //
  // This test passes because expression always evaluated to TRUE.
  //
  UT_ASSERT_TRUE (TRUE);

  //
  // This test passes because expression always evaluates to TRUE.
  //
  Result = LShiftU64 (BIT0, 1);
  UT_ASSERT_TRUE (Result == BIT1);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_ASSERT_FALSE() macro.

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
MacroUtAssertFalse (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64  Result;

  //
  // This test passes because expression always evaluated to FALSE.
  //
  UT_ASSERT_FALSE (FALSE);

  //
  // This test passes because expression always evaluates to FALSE.
  //
  Result = LShiftU64 (BIT0, 1);
  UT_ASSERT_FALSE (Result == BIT0);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_ASSERT_EQUAL() macro.

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
MacroUtAssertEqual (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64  Result;

  //
  // This test passes because both values are always equal.
  //
  UT_ASSERT_EQUAL (1, 1);

  //
  // This test passes because both values are always equal.
  //
  Result = LShiftU64 (BIT0, 1);
  UT_ASSERT_EQUAL (Result, BIT1);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_ASSERT_MEM_EQUAL() macro.

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
MacroUtAssertMemEqual (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  CHAR8  *String1;
  CHAR8  *String2;
  UINTN  Length;

  //
  // This test passes because String1 and String2 are the same.
  //
  String1 = "Hello";
  String2 = "Hello";
  Length  = sizeof ("Hello");
  UT_ASSERT_MEM_EQUAL (String1, String2, Length);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_ASSERT_NOT_EQUAL() macro.

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
MacroUtAssertNotEqual (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64  Result;

  //
  // This test passes because both values are never equal.
  //
  UT_ASSERT_NOT_EQUAL (0, 1);

  //
  // This test passes because both values are never equal.
  //
  Result = LShiftU64 (BIT0, 1);
  UT_ASSERT_NOT_EQUAL (Result, BIT0);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_ASSERT_NOT_EFI_ERROR() macro.

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
MacroUtAssertNotEfiError (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // This test passes because the status is not an EFI error.
  //
  UT_ASSERT_NOT_EFI_ERROR (EFI_SUCCESS);

  //
  // This test passes because the status is not an EFI error.
  //
  UT_ASSERT_NOT_EFI_ERROR (EFI_WARN_BUFFER_TOO_SMALL);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_ASSERT_STATUS_EQUAL() macro.

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
MacroUtAssertStatusEqual (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // This test passes because the status value are always equal.
  //
  UT_ASSERT_STATUS_EQUAL (EFI_SUCCESS, EFI_SUCCESS);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_ASSERT_NOT_NULL() macro.

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
MacroUtAssertNotNull (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT64  Result;

  //
  // This test passes because the pointer is never NULL.
  //
  UT_ASSERT_NOT_NULL (&Result);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_EXPECT_ASSERT_FAILURE() macro.

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
MacroUtExpectAssertFailure (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // This test passes because it directly triggers an ASSERT().
  //
  UT_EXPECT_ASSERT_FAILURE (ASSERT (FALSE), NULL);

  //
  // This test passes because DecimalToBcd() generates an ASSERT() if the
  // value passed in is >= 100.  The expected ASSERT() is caught by the unit
  // test framework and UT_EXPECT_ASSERT_FAILURE() returns without an error.
  //
  UT_EXPECT_ASSERT_FAILURE (DecimalToBcd8 (101), NULL);

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_LOG_ERROR() macro.

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
MacroUtLogError (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Example of logging.
  //
  UT_LOG_ERROR ("UT_LOG_ERROR() message\n");

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_LOG_WARNING() macro.

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
MacroUtLogWarning (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Example of logging.
  //
  UT_LOG_WARNING ("UT_LOG_WARNING() message\n");

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_LOG_INFO() macro.

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
MacroUtLogInfo (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Example of logging.
  //
  UT_LOG_INFO ("UT_LOG_INFO() message\n");

  return UNIT_TEST_PASSED;
}

/**
  Sample unit test using the UT_LOG_VERBOSE() macro.

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
MacroUtLogVerbose (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  //
  // Example of logging.
  //
  UT_LOG_VERBOSE ("UT_LOG_VERBOSE() message\n");

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
  UNIT_TEST_SUITE_HANDLE      SimpleMathTests;
  UNIT_TEST_SUITE_HANDLE      GlobalVarTests;
  UNIT_TEST_SUITE_HANDLE      MacroTestsAssertsEnabled;
  UNIT_TEST_SUITE_HANDLE      MacroTestsAssertsDisabled;

  Framework = NULL;

  DEBUG(( DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION ));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (&Framework, UNIT_TEST_NAME, gEfiCallerBaseName, UNIT_TEST_VERSION);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the SimpleMathTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&SimpleMathTests, Framework, "Simple Math Tests", "Sample.Math", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for SimpleMathTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (SimpleMathTests, "Adding 1 to 1 should produce 2", "Addition", OnePlusOneShouldEqualTwo, NULL, NULL, NULL);

  //
  // Populate the GlobalVarTests Unit Test Suite.
  //
  Status = CreateUnitTestSuite (&GlobalVarTests, Framework, "Global Variable Tests", "Sample.Globals", NULL, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for GlobalVarTests\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (GlobalVarTests, "You should be able to change a global BOOLEAN", "Boolean", GlobalBooleanShouldBeChangeable, NULL, NULL, NULL);
  AddTestCase (GlobalVarTests, "You should be able to change a global pointer", "Pointer", GlobalPointerShouldBeChangeable, MakeSureThatPointerIsNull, ClearThePointer, NULL);

  //
  // Populate the Macro Tests with ASSERT() enabled
  //
  Status = CreateUnitTestSuite (&MacroTestsAssertsEnabled, Framework, "Macro Tests with ASSERT() enabled", "Sample.MacroAssertsEnabled", TestSuiteEnableAsserts, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MacroTestsAssertsEnabled\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_TRUE() macro",           "MacroUtAssertTrue",          MacroUtAssertTrue,          NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_FALSE() macro",          "MacroUtAssertFalse",         MacroUtAssertFalse,         NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_EQUAL() macro",          "MacroUtAssertEqual",         MacroUtAssertEqual,         NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_MEM_EQUAL() macro",      "MacroUtAssertMemEqual",      MacroUtAssertMemEqual,      NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_NOT_EQUAL() macro",      "MacroUtAssertNotEqual",      MacroUtAssertNotEqual,      NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_NOT_EFI_ERROR() macro",  "MacroUtAssertNotEfiError",   MacroUtAssertNotEfiError,   NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_STATUS_EQUAL() macro",   "MacroUtAssertStatusEqual",   MacroUtAssertStatusEqual,   NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_ASSERT_NOT_NULL() macro",       "MacroUtAssertNotNull",       MacroUtAssertNotNull,       NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_EXPECT_ASSERT_FAILURE() macro", "MacroUtExpectAssertFailure", MacroUtExpectAssertFailure, NULL, NULL, NULL);

  AddTestCase (MacroTestsAssertsEnabled, "Test UT_LOG_ERROR() macro",   "MacroUtLogError",   MacroUtLogError,   NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_LOG_WARNING() macro", "MacroUtLogWarning", MacroUtLogWarning, NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_LOG_INFO() macro",    "MacroUtLogInfo",    MacroUtLogInfo,    NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsEnabled, "Test UT_LOG_VERBOSE() macro", "MacroUtLogVerbose", MacroUtLogVerbose, NULL, NULL, NULL);

  //
  // Populate the Macro Tests with ASSERT() disabled
  //
  Status = CreateUnitTestSuite (&MacroTestsAssertsDisabled, Framework, "Macro Tests with ASSERT() disabled", "Sample.MacroAssertsDisables", TestSuiteDisableAsserts, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MacroTestsAssertsDisabled\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_TRUE() macro",           "MacroUtAssertTrue",          MacroUtAssertTrue,          NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_FALSE() macro",          "MacroUtAssertFalse",         MacroUtAssertFalse,         NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_EQUAL() macro",          "MacroUtAssertEqual",         MacroUtAssertEqual,         NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_MEM_EQUAL() macro",      "MacroUtAssertMemEqual",      MacroUtAssertMemEqual,      NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_NOT_EQUAL() macro",      "MacroUtAssertNotEqual",      MacroUtAssertNotEqual,      NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_NOT_EFI_ERROR() macro",  "MacroUtAssertNotEfiError",   MacroUtAssertNotEfiError,   NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_STATUS_EQUAL() macro",   "MacroUtAssertStatusEqual",   MacroUtAssertStatusEqual,   NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_ASSERT_NOT_NULL() macro",       "MacroUtAssertNotNull",       MacroUtAssertNotNull,       NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_EXPECT_ASSERT_FAILURE() macro", "MacroUtExpectAssertFailure", MacroUtExpectAssertFailure, NULL, NULL, NULL);

  AddTestCase (MacroTestsAssertsDisabled, "Test UT_LOG_ERROR() macro",   "MacroUtLogError",   MacroUtLogError,   NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_LOG_WARNING() macro", "MacroUtLogWarning", MacroUtLogWarning, NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_LOG_INFO() macro",    "MacroUtLogInfo",    MacroUtLogInfo,    NULL, NULL, NULL);
  AddTestCase (MacroTestsAssertsDisabled, "Test UT_LOG_VERBOSE() macro", "MacroUtLogVerbose", MacroUtLogVerbose, NULL, NULL, NULL);

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
  int argc,
  char *argv[]
  )
{
  return UefiTestMain ();
}
