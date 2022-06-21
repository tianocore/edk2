/** @file
  This is a sample to demonstrates the use of GoogleTest that supports host
  execution environments.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <gtest/gtest.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
}

/**
  Sample unit test that verifies the expected result of an unsigned integer
  addition operation.
**/
TEST(SimpleMathTests, OnePlusOneShouldEqualTwo) {
  UINTN  A;
  UINTN  B;
  UINTN  C;

  A = 1;
  B = 1;
  C = A + B;

  ASSERT_EQ (C, (UINTN)2);
}

/**
  Sample unit test that verifies that a global BOOLEAN is updatable.
**/
class GlobalBooleanVarTests : public ::testing::Test {
  public:
    BOOLEAN  SampleGlobalTestBoolean  = FALSE;
};

TEST_F(GlobalBooleanVarTests, GlobalBooleanShouldBeChangeable) {
  SampleGlobalTestBoolean = TRUE;
  ASSERT_TRUE (SampleGlobalTestBoolean);

  SampleGlobalTestBoolean = FALSE;
  ASSERT_FALSE (SampleGlobalTestBoolean);
}

/**
  Sample unit test that logs a warning message and verifies that a global
  pointer is updatable.
**/
class GlobalVarTests : public ::testing::Test {
  public:
    VOID  *SampleGlobalTestPointer = NULL;

  protected:
  void SetUp() override {
    ASSERT_EQ ((UINTN)SampleGlobalTestPointer, (UINTN)NULL);
  }
  void TearDown() {
    SampleGlobalTestPointer = NULL;
  }
};

TEST_F(GlobalVarTests, GlobalPointerShouldBeChangeable) {
  SampleGlobalTestPointer = (VOID *)-1;
  ASSERT_EQ ((UINTN)SampleGlobalTestPointer, (UINTN)((VOID *)-1));
}


/**
  Set PcdDebugPropertyMask for each MacroTestsAssertsEnabledDisabled test
**/
class MacroTestsAssertsEnabledDisabled : public testing::TestWithParam<UINT8> {
  void SetUp() {
    PatchPcdSet8 (PcdDebugPropertyMask, GetParam());
  }
};

/**
  Sample unit test using the ASSERT_TRUE() macro.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertTrue) {
  UINT64  Result;

  //
  // This test passes because expression always evaluated to TRUE.
  //
  ASSERT_TRUE (TRUE);

  //
  // This test passes because expression always evaluates to TRUE.
  //
  Result = LShiftU64 (BIT0, 1);
  ASSERT_TRUE (Result == BIT1);
}

/**
  Sample unit test using the ASSERT_FALSE() macro.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertFalse) {
  UINT64  Result;

  //
  // This test passes because expression always evaluated to FALSE.
  //
  ASSERT_FALSE (FALSE);

  //
  // This test passes because expression always evaluates to FALSE.
  //
  Result = LShiftU64 (BIT0, 1);
  ASSERT_FALSE (Result == BIT0);
}

/**
  Sample unit test using the ASSERT_EQ() macro.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertEqual) {
  UINT64  Result;

  //
  // This test passes because both values are always equal.
  //
  ASSERT_EQ (1, 1);

  //
  // This test passes because both values are always equal.
  //
  Result = LShiftU64 (BIT0, 1);
  ASSERT_EQ (Result, (UINT64)BIT1);
}

/**
  Sample unit test using the ASSERT_STREQ() macro.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertMemEqual) {
  CHAR8  *String1;
  CHAR8  *String2;

  //
  // This test passes because String1 and String2 are the same.
  //
  String1 = (CHAR8 *)"Hello";
  String2 = (CHAR8 *)"Hello";
  ASSERT_STREQ (String1, String2);
}

/**
  Sample unit test using the ASSERT_NE() macro.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertNotEqual) {
  UINT64  Result;

  //
  // This test passes because both values are never equal.
  //
  ASSERT_NE (0, 1);

  //
  // This test passes because both values are never equal.
  //
  Result = LShiftU64 (BIT0, 1);
  ASSERT_NE (Result, (UINT64)BIT0);
}

/**
  Sample unit test using the ASSERT_TRUE() and ASSERT(FALSE)
  and EFI_EFFOR() macros to check status
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertNotEfiError) {
  //
  // This test passes because the status is not an EFI error.
  //
  ASSERT_FALSE (EFI_ERROR (EFI_SUCCESS));

  //
  // This test passes because the status is not an EFI error.
  //
  ASSERT_FALSE (EFI_ERROR (EFI_WARN_BUFFER_TOO_SMALL));
}

/**
  Sample unit test using the ASSERT_EQ() macro to compare EFI_STATUS values.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertStatusEqual) {
  //
  // This test passes because the status value are always equal.
  //
  ASSERT_EQ (EFI_SUCCESS, EFI_SUCCESS);
}

/**
  Sample unit test using ASSERT_NE() macro to make sure a pointer is not NULL.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroAssertNotNull) {
  UINT64  Result;

  //
  // This test passes because the pointer is never NULL.
  //
  ASSERT_NE (&Result, (UINT64 *)NULL);
}

/**
  Sample unit test using that should not generate any ASSERTs()
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroExpectNoAssertFailure) {
  //
  // This test passes because it never triggers an ASSERT().
  //
  ASSERT (TRUE);

  //
  // This test passes because DecimalToBcd() does not ASSERT() if the
  // value passed in is <= 99.
  //
  DecimalToBcd8 (99);
}

/**
  Sample unit test using the ASSERT_DEATH() macro to test expected ASSERT()s.
**/
TEST_P(MacroTestsAssertsEnabledDisabled, MacroExpectAssertFailure) {
  //
  // Skip tests that verify an ASSERT() is triggered if ASSERT()s are disabled.
  //
  if ((PcdGet8 (PcdDebugPropertyMask) & BIT0) == 0x00) {
    return;
  }

  //
  // This test passes because it directly triggers an ASSERT().
  //
  ASSERT_DEATH (ASSERT (FALSE), "");

  //
  // This test passes because DecimalToBcd() generates an ASSERT() if the
  // value passed in is >= 100.  The expected ASSERT() is caught by the unit
  // test framework and ASSERT_DEATH() returns without an error.
  //
  ASSERT_DEATH (DecimalToBcd8 (101), "");
}

INSTANTIATE_TEST_SUITE_P(ValidInput,
                         MacroTestsAssertsEnabledDisabled,
                         ::testing::Values(PcdGet8 (PcdDebugPropertyMask) | BIT0, PcdGet8 (PcdDebugPropertyMask) & (~BIT0)));

/**
  Sample unit test using the SCOPED_TRACE() macro for trace messages.
**/
TEST(MacroTestsMessages, MacroTraceMessage) {
  //
  // Example of logging.
  //
  SCOPED_TRACE ("SCOPED_TRACE message\n");
}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
