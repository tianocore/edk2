/** @file
  Implement UnitTestLib assert services using cmocka services

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
#include <Library/BaseMemoryLib.h>
#include <Library/UnitTestLib.h>

#define MAX_STRING_SIZE  1025

/**
  If Expression is TRUE, then TRUE is returned.
  If Expression is FALSE, then an assert is triggered and the location of the
  assert provided by FunctionName, LineNumber, FileName, and Description are
  recorded and FALSE is returned.

  @param[in]  Expression    The BOOLEAN result of the expression evaluation.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  Description   Null-terminated ASCII string of the expression being
                            evaluated.

  @retval  TRUE   Expression is TRUE.
  @retval  FALSE  Expression is FALSE.
**/
BOOLEAN
EFIAPI
UnitTestAssertTrue (
  IN BOOLEAN      Expression,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_TRUE(%s:%x)", Description, Expression);
  _assert_true (Expression, TempStr, FileName, (INT32)LineNumber);

  return Expression;
}

/**
  If Expression is FALSE, then TRUE is returned.
  If Expression is TRUE, then an assert is triggered and the location of the
  assert provided by FunctionName, LineNumber, FileName, and Description are
  recorded and FALSE is returned.

  @param[in]  Expression    The BOOLEAN result of the expression evaluation.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  Description   Null-terminated ASCII string of the expression being
                            evaluated.

  @retval  TRUE   Expression is FALSE.
  @retval  FALSE  Expression is TRUE.
**/
BOOLEAN
EFIAPI
UnitTestAssertFalse (
  IN BOOLEAN      Expression,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_FALSE(%s:%x)", Description, Expression);
  _assert_true (!Expression, TempStr, FileName, (INT32)LineNumber);

  return !Expression;
}

/**
  If Status is not an EFI_ERROR(), then TRUE is returned.
  If Status is an EFI_ERROR(), then an assert is triggered and the location of
  the assert provided by FunctionName, LineNumber, FileName, and Description are
  recorded and FALSE is returned.

  @param[in]  Status        The EFI_STATUS value to evaluate.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  Description   Null-terminated ASCII string of the status
                            expression being evaluated.

  @retval  TRUE   Status is not an EFI_ERROR().
  @retval  FALSE  Status is an EFI_ERROR().
**/
BOOLEAN
EFIAPI
UnitTestAssertNotEfiError (
  IN EFI_STATUS   Status,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_NOT_EFI_ERROR(%s:%p)", Description, (void *)Status);
  _assert_true (!EFI_ERROR (Status), TempStr, FileName, (INT32)LineNumber);

  return !EFI_ERROR (Status);
}

/**
  If ValueA is equal ValueB, then TRUE is returned.
  If ValueA is not equal to ValueB, then an assert is triggered and the location
  of the assert provided by FunctionName, LineNumber, FileName, DescriptionA,
  and DescriptionB are recorded and FALSE is returned.

  @param[in]  ValueA        64-bit value.
  @param[in]  ValueB        64-bit value.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  DescriptionA  Null-terminated ASCII string that is a description
                            of ValueA.
  @param[in]  DescriptionB  Null-terminated ASCII string that is a description
                            of ValueB.

  @retval  TRUE   ValueA is equal to ValueB.
  @retval  FALSE  ValueA is not equal to ValueB.
**/
BOOLEAN
EFIAPI
UnitTestAssertEqual (
  IN UINT64       ValueA,
  IN UINT64       ValueB,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *DescriptionA,
  IN CONST CHAR8  *DescriptionB
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_EQUAL(%s:%llx, %s:%llx)", DescriptionA, ValueA, DescriptionB, ValueB);
  _assert_true ((ValueA == ValueB), TempStr, FileName, (INT32)LineNumber);

  return (ValueA == ValueB);
}

/**
  If the contents of BufferA are identical to the contents of BufferB, then TRUE
  is returned.  If the contents of BufferA are not identical to the contents of
  BufferB, then an assert is triggered and the location of the assert provided
  by FunctionName, LineNumber, FileName, DescriptionA, and DescriptionB are
  recorded and FALSE is returned.

  @param[in]  BufferA       Pointer to a buffer for comparison.
  @param[in]  BufferB       Pointer to a buffer for comparison.
  @param[in]  Length        Number of bytes to compare in BufferA and BufferB.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  DescriptionA  Null-terminated ASCII string that is a description
                            of BufferA.
  @param[in]  DescriptionB  Null-terminated ASCII string that is a description
                            of BufferB.

  @retval  TRUE   The contents of BufferA are identical to the contents of
                  BufferB.
  @retval  FALSE  The contents of BufferA are not identical to the contents of
                  BufferB.
**/
BOOLEAN
EFIAPI
UnitTestAssertMemEqual (
  IN VOID         *BufferA,
  IN VOID         *BufferB,
  IN UINTN        Length,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *DescriptionA,
  IN CONST CHAR8  *DescriptionB
  )
{
  CHAR8    TempStr[MAX_STRING_SIZE];
  BOOLEAN  Result;

  Result = (CompareMem (BufferA, BufferB, Length) == 0);

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_MEM_EQUAL(%s:%p, %s:%p)", DescriptionA, BufferA, DescriptionB, BufferB);
  _assert_true (Result, TempStr, FileName, (INT32)LineNumber);

  return Result;
}

/**
  If ValueA is not equal ValueB, then TRUE is returned.
  If ValueA is equal to ValueB, then an assert is triggered and the location
  of the assert provided by FunctionName, LineNumber, FileName, DescriptionA
  and DescriptionB are recorded and FALSE is returned.

  @param[in]  ValueA        64-bit value.
  @param[in]  ValueB        64-bit value.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  DescriptionA  Null-terminated ASCII string that is a description
                            of ValueA.
  @param[in]  DescriptionB  Null-terminated ASCII string that is a description
                            of ValueB.

  @retval  TRUE   ValueA is not equal to ValueB.
  @retval  FALSE  ValueA is equal to ValueB.
**/
BOOLEAN
EFIAPI
UnitTestAssertNotEqual (
  IN UINT64       ValueA,
  IN UINT64       ValueB,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *DescriptionA,
  IN CONST CHAR8  *DescriptionB
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_NOT_EQUAL(%s:%llx, %s:%llx)", DescriptionA, ValueA, DescriptionB, ValueB);
  _assert_true ((ValueA != ValueB), TempStr, FileName, (INT32)LineNumber);

  return (ValueA != ValueB);
}

/**
  If Status is equal to Expected, then TRUE is returned.
  If Status is not equal to Expected, then an assert is triggered and the
  location of the assert provided by FunctionName, LineNumber, FileName, and
  Description are recorded and FALSE is returned.

  @param[in]  Status        EFI_STATUS value returned from an API under test.
  @param[in]  Expected      The expected EFI_STATUS return value from an API
                            under test.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  Description   Null-terminated ASCII string that is a description
                            of Status.

  @retval  TRUE   Status is equal to Expected.
  @retval  FALSE  Status is not equal to Expected.
**/
BOOLEAN
EFIAPI
UnitTestAssertStatusEqual (
  IN EFI_STATUS   Status,
  IN EFI_STATUS   Expected,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_STATUS_EQUAL(%s:%p)", Description, (VOID *)Status);
  _assert_true ((Status == Expected), TempStr, FileName, (INT32)LineNumber);

  return (Status == Expected);
}

/**
  If Pointer is not equal to NULL, then TRUE is returned.
  If Pointer is equal to NULL, then an assert is triggered and the location of
  the assert provided by FunctionName, LineNumber, FileName, and PointerName
  are recorded and FALSE is returned.

  @param[in]  Pointer       Pointer value to be checked against NULL.
  @param[in]  Expected      The expected EFI_STATUS return value from a function
                            under test.
  @param[in]  FunctionName  Null-terminated ASCII string of the function
                            executing the assert macro.
  @param[in]  LineNumber    The source file line number of the assert macro.
  @param[in]  FileName      Null-terminated ASCII string of the filename
                            executing the assert macro.
  @param[in]  PointerName   Null-terminated ASCII string that is a description
                            of Pointer.

  @retval  TRUE   Pointer is not equal to NULL.
  @retval  FALSE  Pointer is equal to NULL.
**/
BOOLEAN
EFIAPI
UnitTestAssertNotNull (
  IN VOID         *Pointer,
  IN CONST CHAR8  *FunctionName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *FileName,
  IN CONST CHAR8  *PointerName
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  snprintf (TempStr, sizeof (TempStr), "UT_ASSERT_NOT_NULL(%s:%p)", PointerName, Pointer);
  _assert_true ((Pointer != NULL), TempStr, FileName, (INT32)LineNumber);

  return (Pointer != NULL);
}

/**
  If UnitTestStatus is UNIT_TEST_PASSED, then log an info message and return
  TRUE because an ASSERT() was expected when FunctionCall was executed and an
  ASSERT() was triggered. If UnitTestStatus is UNIT_TEST_SKIPPED, then log a
  warning message and return TRUE because ASSERT() macros are disabled.  If
  UnitTestStatus is UNIT_TEST_ERROR_TEST_FAILED, then log an error message and
  return FALSE because an ASSERT() was expected when FunctionCall was executed,
  but no ASSERT() conditions were triggered.  The log messages contain
  FunctionName, LineNumber, and FileName strings to provide the location of the
  UT_EXPECT_ASSERT_FAILURE() macro.

  @param[in]  UnitTestStatus  The status from UT_EXPECT_ASSERT_FAILURE() that
                              is either pass, skipped, or failed.
  @param[in]  FunctionName    Null-terminated ASCII string of the function
                              executing the UT_EXPECT_ASSERT_FAILURE() macro.
  @param[in]  LineNumber      The source file line number of the the function
                              executing the UT_EXPECT_ASSERT_FAILURE() macro.
  @param[in]  FileName        Null-terminated ASCII string of the filename
                              executing the UT_EXPECT_ASSERT_FAILURE() macro.
  @param[in]  FunctionCall    Null-terminated ASCII string of the function call
                              executed by the UT_EXPECT_ASSERT_FAILURE() macro.
  @param[out] ResultStatus    Used to return the UnitTestStatus value to the
                              caller of UT_EXPECT_ASSERT_FAILURE().  This is
                              optional parameter that may be NULL.

  @retval  TRUE   UnitTestStatus is UNIT_TEST_PASSED.
  @retval  TRUE   UnitTestStatus is UNIT_TEST_SKIPPED.
  @retval  FALSE  UnitTestStatus is UNIT_TEST_ERROR_TEST_FAILED.
**/
BOOLEAN
EFIAPI
UnitTestExpectAssertFailure (
  IN  UNIT_TEST_STATUS  UnitTestStatus,
  IN  CONST CHAR8       *FunctionName,
  IN  UINTN             LineNumber,
  IN  CONST CHAR8       *FileName,
  IN  CONST CHAR8       *FunctionCall,
  OUT UNIT_TEST_STATUS  *ResultStatus  OPTIONAL
  )
{
  CHAR8  TempStr[MAX_STRING_SIZE];

  if (ResultStatus != NULL) {
    *ResultStatus = UnitTestStatus;
  }

  if (UnitTestStatus == UNIT_TEST_PASSED) {
    UT_LOG_INFO (
      "[ASSERT PASS] %a:%d: UT_EXPECT_ASSERT_FAILURE(%a) detected expected assert\n",
      FileName,
      LineNumber,
      FunctionCall
      );
  }

  if (UnitTestStatus == UNIT_TEST_SKIPPED) {
    UT_LOG_WARNING (
      "[ASSERT WARN] %a:%d: UT_EXPECT_ASSERT_FAILURE(%a) disabled\n",
      FileName,
      LineNumber,
      FunctionCall
      );
  }

  if (UnitTestStatus == UNIT_TEST_ERROR_TEST_FAILED) {
    snprintf (TempStr, sizeof (TempStr), "UT_EXPECT_ASSERT_FAILURE(%s) did not trigger ASSERT()", FunctionCall);
    _assert_true (FALSE, TempStr, FileName, (INT32)LineNumber);
  }

  return (UnitTestStatus != UNIT_TEST_ERROR_TEST_FAILED);
}
