/** @file
  This is a sample to demonstrates the use of GoogleTest that supports host
  execution environments.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/GoogleTestLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <Library/MemoryAllocationLib.h>
  #include <Library/HostMemoryAllocationBelowAddressLib.h>
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

  ASSERT_EQ (C, (UINTN)2);
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
  VOID *SampleGlobalTestPointer = NULL;

protected:
  void
  SetUp (
    ) override
  {
    ASSERT_EQ ((UINTN)SampleGlobalTestPointer, (UINTN)NULL);
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
  ASSERT_EQ ((UINTN)SampleGlobalTestPointer, (UINTN)((VOID *)-1));
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
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertFalse) {
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
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertEqual) {
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
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertMemEqual) {
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
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertNotEqual) {
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
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertNotEfiError) {
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
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertStatusEqual) {
  //
  // This test passes because the status value are always equal.
  //
  ASSERT_EQ (EFI_SUCCESS, EFI_SUCCESS);
}

/**
  Sample unit test using ASSERT_NE() macro to make sure a pointer is not NULL.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroAssertNotNull) {
  UINT64  Result;

  //
  // This test passes because the pointer is never NULL.
  //
  ASSERT_NE (&Result, (UINT64 *)NULL);
}

/**
  Sample unit test using that should not generate any ASSERTs()
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroExpectNoAssertFailure) {
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
  Sample unit test using the EXPECT_ANY_THROW() macro to test expected ASSERT()s.
**/
TEST_P (MacroTestsAssertsEnabledDisabled, MacroExpectAssertFailure) {
  //
  // Skip tests that verify an ASSERT() is triggered if ASSERT()s are disabled.
  //
  if ((PcdGet8 (PcdDebugPropertyMask) & BIT0) == 0x00) {
    return;
  }

  //
  // This test passes because it directly triggers an ASSERT().
  //
  EXPECT_ANY_THROW (ASSERT (FALSE));

  //
  // This test passes because DecimalToBcd() generates an ASSERT() if the
  // value passed in is >= 100.  The expected ASSERT() is caught by the unit
  // test framework and EXPECT_ANY_THROW() returns without an error.
  //
  EXPECT_ANY_THROW (DecimalToBcd8 (101));

  //
  // This test passes because DecimalToBcd() generates an ASSERT() if the
  // value passed in is >= 100.  The expected ASSERT() is caught by the unit
  // test framework and throws the C++ exception of type std::runtime_error.
  // EXPECT_THROW() returns without an error.
  //
  EXPECT_THROW (DecimalToBcd8 (101), std::runtime_error);

  //
  // This test passes because DecimalToBcd() generates an ASSERT() if the
  // value passed in is >= 100.  The expected ASSERT() is caught by the unit
  // test framework and throws the C++ exception of type std::runtime_error with
  // a message that includes the filename, linenumber, and the expression that
  // triggered the ASSERT().
  //
  // EXPECT_THROW_MESSAGE() calls DecimalToBcd() expecting DecimalToBds() to
  // throw a C++ exception of type std::runtime_error with a message that
  // includes the expression of "Value < 100" that triggered the ASSERT().
  //
  EXPECT_THROW_MESSAGE (DecimalToBcd8 (101), "Value < 100");
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

  //
  // Always pass
  //
  ASSERT_TRUE (TRUE);
}

/**
  Sample unit test that performs double free
**/
TEST (SanitizerTests, DoubleFreeDeathTest) {
  UINT8  *Pointer;

  Pointer = (UINT8 *)AllocatePool (100);
  ASSERT_NE (Pointer, (UINT8 *)NULL);
  FreePool (Pointer);
  //
  // Second free that should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (FreePool (Pointer), "ERROR: AddressSanitizer: heap-use-after-free");
}

/**
  Sample unit test that performs read past end of allocated buffer
**/
TEST (SanitizerTests, BufferOverflowReadDeathTest) {
  UINT8  *Pointer;
  UINT8  Value;

  Pointer = (UINT8 *)AllocatePool (100);
  ASSERT_NE (Pointer, (UINT8 *)NULL);

  //
  // Read past end of allocated buffer that should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (Value = Pointer[110], "ERROR: AddressSanitizer: heap-buffer-overflow");
  ASSERT_EQ (Value, Value);

  FreePool (Pointer);
}

/**
  Sample unit test that performs write past end of allocated buffer
**/
TEST (SanitizerTests, BufferOverflowWriteDeathTest) {
  UINT8  *Pointer;

  Pointer = (UINT8 *)AllocatePool (100);
  ASSERT_NE (Pointer, (UINT8 *)NULL);

  //
  // Write past end of allocated buffer that should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (Pointer[110] = 0, "ERROR: AddressSanitizer: heap-buffer-overflow");

  FreePool (Pointer);
}

/**
  Sample unit test that performs read before beginning of allocated buffer
**/
TEST (SanitizerTests, BufferUnderflowReadDeathTest) {
  UINT8  *Pointer;
  UINT8  Value;

  Pointer = (UINT8 *)AllocatePool (100);
  ASSERT_NE (Pointer, (UINT8 *)NULL);

  //
  // Read past end of allocated buffer that should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (Value = Pointer[-10], "ERROR: AddressSanitizer: heap-buffer-overflow");
  ASSERT_EQ (Value, Value);

  FreePool (Pointer);
}

/**
  Sample unit test that performs read from address 0 (NULL)
**/
TEST (SanitizerTests, NullPointerReadDeathTest) {
  UINT8  Value;

  //
  // Read from address 0 should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (Value = *(volatile UINT8 *)(NULL), "ERROR: AddressSanitizer: ");
  ASSERT_EQ (Value, Value);
}

/**
  Sample unit test that performs write to address 0 (NULL)
**/
TEST (SanitizerTests, NullPointerWriteDeathTest) {
  //
  // Write to address 0 should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (*(volatile UINT8 *)(NULL) = 0, "ERROR: AddressSanitizer: ");
}

/**
  Sample unit test that performs read from invalid address -1
**/
TEST (SanitizerTests, InvalidPointerReadDeathTest) {
  UINT8  Value;

  //
  // Read from address -1 should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (Value = *(volatile UINT8 *)(-1), "ERROR: AddressSanitizer: ");
  ASSERT_EQ (Value, Value);
}

/**
  Sample unit test that performs write to invalid address -1
**/
TEST (SanitizerTests, InvalidPointerWriteDeathTest) {
  //
  // Write to address -1 should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (*(volatile UINT8 *)(-1) = 0, "ERROR: AddressSanitizer: ");
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
  Sample unit test that performs a divide by 0
**/
TEST (SanitizerTests, DivideByZeroDeathTest) {
  //
  // Divide by 0 should be caught by address sanitizer, log details, and exit
  //
  EXPECT_DEATH (DivideWithNoParameterChecking (10, 0), "ERROR: AddressSanitizer: ");
}

/**
  Sample unit test that allocates and frees buffers below 4GB
**/
TEST (MemoryAllocationTests, Below4GB) {
  VOID   *Buffer1;
  VOID   *Buffer2;
  UINT8  EmptyBuffer[0x100];

  //
  // Length 0 always fails
  //
  Buffer1 = HostAllocatePoolBelowAddress (BASE_4GB - 1, 0);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Length == Maximum Address always fails
  //
  Buffer1 = HostAllocatePoolBelowAddress (BASE_4GB - 1, SIZE_4GB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Length > Maximum Address always fails
  //
  Buffer1 = HostAllocatePoolBelowAddress (BASE_4GB - 1, SIZE_8GB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Maximum Address 0 always fails
  //
  Buffer1 = HostAllocatePoolBelowAddress (0, SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Maximum Address < 64KB always fails
  //
  Buffer1 = HostAllocatePoolBelowAddress (BASE_64KB - 1, SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Not enough memory available always fails
  //
  Buffer1 = HostAllocatePoolBelowAddress (BASE_128KB - 1, SIZE_64KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Allocation of 4KB buffer below 4GB must succeed
  //
  Buffer1 = HostAllocatePoolBelowAddress (BASE_4GB - 1, SIZE_4KB);
  ASSERT_NE (Buffer1, (VOID *)NULL);
  ASSERT_LT ((UINTN)Buffer1, BASE_4GB);

  //
  // Allocated buffer must support read and write
  //
  *(UINT8 *)Buffer1 = 0x5A;
  ASSERT_EQ (*(UINT8 *)Buffer1, 0x5A);

  //
  // Allocation of 1MB buffer below 4GB must succeed
  //
  Buffer2 = HostAllocatePoolBelowAddress (BASE_4GB - 1, SIZE_1MB);
  ASSERT_NE (Buffer2, (VOID *)NULL);
  ASSERT_LT ((UINTN)Buffer2, BASE_4GB);

  //
  // Allocated buffer must support read and write
  //
  *(UINT8 *)Buffer2 = 0x5A;
  ASSERT_EQ (*(UINT8 *)Buffer2, 0x5A);

  //
  // Allocations must return different values
  //
  ASSERT_NE (Buffer1, Buffer2);

  //
  // Free buffers below 4GB must not ASSERT
  //
  HostFreePoolBelowAddress (Buffer1);
  HostFreePoolBelowAddress (Buffer2);

  //
  // Expect ASSERT() tests
  //
  EXPECT_ANY_THROW (HostFreePoolBelowAddress (NULL));
  EXPECT_ANY_THROW (HostFreePoolBelowAddress (EmptyBuffer + 0x80));
  Buffer1 = AllocatePool (0x100);
  EXPECT_ANY_THROW (HostFreePoolBelowAddress ((UINT8 *)Buffer1 + 0x80));
  FreePool (Buffer1);
}

/**
  Sample unit test that allocates and frees aligned pages below 4GB
**/
TEST (MemoryAllocationTests, AlignedBelow4GB) {
  VOID   *Buffer1;
  VOID   *Buffer2;
  UINT8  EmptyBuffer[0x100];

  //
  // Pages 0 always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, 0, SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Alignment not a power of 2 always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, SIZE_4KB, 5);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Alignment not a power of 2 always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, SIZE_4KB, SIZE_16KB + 1);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Alignment larger than largest supported virtual address always fails
  // Only applies to 32-bit architectures
  //
  if (sizeof (UINTN) == sizeof (UINT32)) {
    Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, SIZE_4KB, SIZE_4GB);
    ASSERT_EQ (Buffer1, (VOID *)NULL);
  }

  //
  // Length == Maximum Address always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, EFI_SIZE_TO_PAGES (SIZE_4GB), SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Length > Maximum Address always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, EFI_SIZE_TO_PAGES (SIZE_8GB), SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Alignment >= Maximum Address always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, EFI_SIZE_TO_PAGES (SIZE_4GB), SIZE_4GB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Maximum Address 0 always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (0, EFI_SIZE_TO_PAGES (SIZE_4KB), SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Maximum Address <= 64KB always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_64KB - 1, EFI_SIZE_TO_PAGES (SIZE_4KB), SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Not enough memory available always fails
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_128KB - 1, EFI_SIZE_TO_PAGES (SIZE_64KB), SIZE_4KB);
  ASSERT_EQ (Buffer1, (VOID *)NULL);

  //
  // Allocation of 4KB buffer below 4GB must succeed
  //
  Buffer1 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, EFI_SIZE_TO_PAGES (SIZE_4KB), SIZE_4KB);
  ASSERT_NE (Buffer1, (VOID *)NULL);
  ASSERT_LT ((UINTN)Buffer1, BASE_4GB);

  //
  // Allocated buffer must support read and write
  //
  *(UINT8 *)Buffer1 = 0x5A;
  ASSERT_EQ (*(UINT8 *)Buffer1, 0x5A);

  //
  // Allocation of 1MB buffer below 4GB must succeed
  //
  Buffer2 = HostAllocateAlignedPagesBelowAddress (BASE_4GB - 1, EFI_SIZE_TO_PAGES (SIZE_1MB), SIZE_1MB);
  ASSERT_NE (Buffer2, (VOID *)NULL);
  ASSERT_LT ((UINTN)Buffer2, BASE_4GB);

  //
  // Allocated buffer must support read and write
  //
  *(UINT8 *)Buffer2 = 0x5A;
  ASSERT_EQ (*(UINT8 *)Buffer2, 0x5A);

  //
  // Allocations must return different values
  //
  ASSERT_NE (Buffer1, Buffer2);

  //
  // Free buffers below 4GB must not ASSERT
  //
  HostFreeAlignedPagesBelowAddress (Buffer1, EFI_SIZE_TO_PAGES (SIZE_4KB));
  HostFreeAlignedPagesBelowAddress (Buffer2, EFI_SIZE_TO_PAGES (SIZE_1MB));

  //
  // Expect ASSERT() tests
  //
  EXPECT_ANY_THROW (HostFreeAlignedPagesBelowAddress (NULL, 0));
  EXPECT_ANY_THROW (HostFreeAlignedPagesBelowAddress (EmptyBuffer + 0x80, 1));
  Buffer1 = AllocatePool (0x100);
  EXPECT_ANY_THROW (HostFreeAlignedPagesBelowAddress ((UINT8 *)Buffer1 + 0x80, 1));
  FreePool (Buffer1);
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
