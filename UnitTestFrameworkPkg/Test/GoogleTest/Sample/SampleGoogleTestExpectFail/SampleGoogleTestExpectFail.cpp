/** @file
  This is a sample to demonstrates the use of GoogleTest that supports host
  execution environments for test case that are always expected to fail to
  demonstrate the format of the log file and reports when failures occur.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/GoogleTestLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
}

/**
  Sample unit test that verifies the expected result of an unsigned integer
  addition operation.
**/
TEST (SimpleMathTests, OnePlusOneShouldEqualTwo) {
  UINTN  A;
  UINTN  B;
  UINTN  C;

  A = 1;
  B = 1;
  C = A + B;

  ASSERT_NE (C, (UINTN)2);
}

/**
  Sample unit test that verifies that a global BOOLEAN is updatable.
**/
class GlobalBooleanVarTests : public ::testing::Test {
public:
  BOOLEAN SampleGlobalTestBoolean = FALSE;
};

TEST_F (GlobalBooleanVarTests, GlobalBooleanShouldBeChangeable) {
  SampleGlobalTestBoolean = TRUE;
  EXPECT_FALSE (SampleGlobalTestBoolean);

  SampleGlobalTestBoolean = FALSE;
  EXPECT_TRUE (SampleGlobalTestBoolean);
}

/**
  Sample unit test that logs a warning message and verifies that a global
  pointer is updatable.
**/
class GlobalVarTests : public ::testing::Test {
public:
  VOID *SampleGlobalTestPointer = NULL;

protected:
  void
  SetUp (
    ) override
  {
    ASSERT_NE ((UINTN)SampleGlobalTestPointer, (UINTN)NULL);
  }

  void
  TearDown (
    )
  {
    SampleGlobalTestPointer = NULL;
  }
};

TEST_F (GlobalVarTests, GlobalPointerShouldBeChangeable) {
  SampleGlobalTestPointer = (VOID *)-1;
  ASSERT_NE ((UINTN)SampleGlobalTestPointer, (UINTN)((VOID *)-1));
}

/**
  Set PcdDebugPropertyMask for each MacroTestsAssertsEnabledDisabled test
**/
class MacroTestsAssertsEnabledDisabled : public testing::TestWithParam<UINT8> {
  void
  SetUp (
    )
  {
    PatchPcdSet8 (PcdDebugPropertyMask, GetParam ());
  }
};

/**
  Sample unit test using the ASSERT_TRUE() macro.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertTrue) {
  UINT64  Result;

  //
  // This test passes because expression always evaluated to TRUE.
  //
  EXPECT_FALSE (TRUE);

  //
  // This test passes because expression always evaluates to TRUE.
  //
  Result = LShiftU64 (BIT0, 1);
  EXPECT_FALSE (Result == BIT1);
}

/**
  Sample unit test using the ASSERT_FALSE() macro.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertFalse) {
  UINT64  Result;

  //
  // This test passes because expression always evaluated to FALSE.
  //
  EXPECT_TRUE (FALSE);

  //
  // This test passes because expression always evaluates to FALSE.
  //
  Result = LShiftU64 (BIT0, 1);
  EXPECT_TRUE (Result == BIT0);
}

/**
  Sample unit test using the ASSERT_EQ() macro.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertEqual) {
  UINT64  Result;

  //
  // This test passes because both values are always equal.
  //
  EXPECT_NE (1, 1);

  //
  // This test passes because both values are always equal.
  //
  Result = LShiftU64 (BIT0, 1);
  EXPECT_NE (Result, (UINT64)BIT1);
}

/**
  Sample unit test using the ASSERT_STREQ() macro.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertMemEqual) {
  CHAR8  *String1;
  CHAR8  *String2;

  //
  // This test passes because String1 and String2 are the same.
  //
  String1 = (CHAR8 *)"Hello";
  String2 = (CHAR8 *)"Hello";
  EXPECT_STRNE (String1, String2);
}

/**
  Sample unit test using the ASSERT_NE() macro.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertNotEqual) {
  UINT64  Result;

  //
  // This test passes because both values are never equal.
  //
  EXPECT_EQ (0, 1);

  //
  // This test passes because both values are never equal.
  //
  Result = LShiftU64 (BIT0, 1);
  EXPECT_EQ (Result, (UINT64)BIT0);
}

/**
  Sample unit test using the ASSERT_TRUE() and ASSERT(FALSE)
  and EFI_EFFOR() macros to check status
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertNotEfiError) {
  //
  // This test passes because the status is not an EFI error.
  //
  EXPECT_TRUE (EFI_ERROR (EFI_SUCCESS));

  //
  // This test passes because the status is not an EFI error.
  //
  EXPECT_TRUE (EFI_ERROR (EFI_WARN_BUFFER_TOO_SMALL));
}

/**
  Sample unit test using the ASSERT_EQ() macro to compare EFI_STATUS values.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertStatusEqual) {
  //
  // This test passes because the status value are always equal.
  //
  EXPECT_NE (EFI_SUCCESS, EFI_SUCCESS);
}

/**
  Sample unit test using ASSERT_NE() macro to make sure a pointer is not NULL.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertNotNull) {
  UINT64  Result;

  //
  // This test passes because the pointer is never NULL.
  //
  EXPECT_EQ (&Result, (UINT64 *)NULL);
}

/**
  Sample unit test using that generates an unexpected ASSERT
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroDirectForceAssertExpectTestFail) {
  //
  // Skip tests that verify an ASSERT() is triggered if ASSERT()s are disabled.
  //
  if ((PcdGet8 (PcdDebugPropertyMask) & BIT0) == 0x00) {
    EXPECT_TRUE (FALSE);
    return;
  }

  //
  // This test fails because it directly triggers an ASSERT().
  //
  ASSERT (FALSE);
}

/**
  Sample unit test using that generates an unexpected ASSERT
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroIndirectForceAssertExpectTestFail) {
  //
  // Skip tests that verify an ASSERT() is triggered if ASSERT()s are disabled.
  //
  if ((PcdGet8 (PcdDebugPropertyMask) & BIT0) == 0x00) {
    EXPECT_TRUE (FALSE);
    return;
  }

  //
  // This test fails because DecimalToBcd() generates an ASSERT() if the
  // value passed in is >= 100.  The unexpected ASSERT() is caught by the unit
  // test framework and generates a failed test.
  //
  DecimalToBcd8 (101);
}

/**
  Sample unit test using that do not generate an expected ASSERT()
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroExpectedAssertNotTriggeredExpectTestFail) {
  //
  // When ASSERT()s are disabled, all tests for ASSERT()s will fail.
  //
  if ((PcdGet8 (PcdDebugPropertyMask) & BIT0) == 0x00) {
    EXPECT_ANY_THROW (ASSERT (TRUE));
    EXPECT_ANY_THROW (DecimalToBcd8 (99));
    EXPECT_ANY_THROW (DecimalToBcd8 (101));
    EXPECT_THROW (DecimalToBcd8 (99), std::runtime_error);
    EXPECT_THROW (DecimalToBcd8 (101), std::runtime_error);
    EXPECT_THROW (DecimalToBcd8 (99), std::overflow_error);
    EXPECT_THROW (DecimalToBcd8 (101), std::overflow_error);
    EXPECT_THROW_MESSAGE (DecimalToBcd8 (99), "Value < 999");
    EXPECT_THROW_MESSAGE (DecimalToBcd8 (101), "Value < 999");
    return;
  }

  //
  // This test fails because ASSERT(TRUE) never triggers an ASSERT().
  //
  EXPECT_ANY_THROW (ASSERT (TRUE));

  //
  // This test fails because DecimalToBcd() does not generate an ASSERT() if the
  // value passed in is < 100.
  //
  EXPECT_ANY_THROW (DecimalToBcd8 (99));

  //
  // This test fails because DecimalToBcd() does not generate an ASSERT() if the
  // value passed in is < 100.
  //
  EXPECT_THROW (DecimalToBcd8 (99), std::runtime_error);

  //
  // This test fails because DecimalToBcd() does generate an ASSERT() if the
  // value passed in is >= 100, but is generates a C++ exception of type
  // std::runtime_error
  //
  EXPECT_THROW (DecimalToBcd8 (101), std::overflow_error);

  //
  // This test fails because DecimalToBcd() generates an ASSERT() if the
  // value passed in is >= 100.  The expected ASSERT() is caught by the unit
  // test framework and throws the C++ exception of type std::runtime_error with
  // a message that includes the filename, linenumber, and the expression that
  // triggered the ASSERT().  The message generated by BcdToDecimal() is
  // "Value < 100", but the expression tested is not "Value < 100".
  //
  EXPECT_THROW_MESSAGE (DecimalToBcd8 (101), "Value < 999");
}

INSTANTIATE_TEST_SUITE_P (
  ValidInput,
  MacroTestsAssertsEnabledDisabled,
  ::testing::Values (PcdGet8 (PcdDebugPropertyMask) | BIT0, PcdGet8 (PcdDebugPropertyMask) & (~BIT0))
  );

/**
  Sample unit test using the SCOPED_TRACE() macro for trace messages.
**/
TEST (MacroTestsMessages, MacroTraceMessage) {
  //
  // Example of logging.
  //
  SCOPED_TRACE ("SCOPED_TRACE message\n");
  EXPECT_TRUE (FALSE);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
