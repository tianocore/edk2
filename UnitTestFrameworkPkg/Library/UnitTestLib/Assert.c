/**
  Implement UnitTestLib assert services

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <UnitTestFrameworkTypes.h>
#include <Library/UnitTestLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>

STATIC
EFI_STATUS
AddUnitTestFailure (
  IN OUT UNIT_TEST     *UnitTest,
  IN     CONST CHAR8   *FailureMessage,
  IN     FAILURE_TYPE  FailureType
  )
{
  //
  // Make sure that you're cooking with gas.
  //
  if (UnitTest == NULL || FailureMessage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UnitTest->FailureType = FailureType;
  AsciiStrCpyS (
    &UnitTest->FailureMessage[0],
    UNIT_TEST_TESTFAILUREMSG_LENGTH,
    FailureMessage
    );

  return EFI_SUCCESS;
}

STATIC
VOID
UnitTestLogFailure (
  IN FAILURE_TYPE  FailureType,
  IN CONST CHAR8   *Format,
  ...
  )
{
  UNIT_TEST_FRAMEWORK_HANDLE  FrameworkHandle;
  CHAR8                       LogString[UNIT_TEST_TESTFAILUREMSG_LENGTH];
  VA_LIST                     Marker;

  //
  // Get active Framework handle
  //
  FrameworkHandle = GetActiveFrameworkHandle ();

  //
  // Convert the message to an ASCII String
  //
  VA_START (Marker, Format);
  AsciiVSPrint (LogString, sizeof (LogString), Format, Marker);
  VA_END (Marker);

  //
  // Finally, add the string to the log.
  //
  AddUnitTestFailure (
    ((UNIT_TEST_FRAMEWORK *)FrameworkHandle)->CurrentTest,
    LogString,
    FailureType
    );

  return;
}

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
  if (!Expression) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTTRUE,
      "%a::%d Expression (%a) is not TRUE!\n",
      FunctionName,
      LineNumber,
      Description
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Expression (%a) is not TRUE!\n",
      FunctionName,
      LineNumber,
      Description
      );
  }
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
  if (Expression) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTFALSE,
      "%a::%d Expression(%a) is not FALSE!\n",
      FunctionName,
      LineNumber,
      Description
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Expression (%a) is not FALSE!\n",
      FunctionName,
      LineNumber,
      Description
      );
  }
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
  if (EFI_ERROR (Status)) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTNOTEFIERROR,
      "%a::%d Status '%a' is EFI_ERROR (%r)!\n",
      FunctionName,
      LineNumber,
      Description,
      Status
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Status '%a' is EFI_ERROR (%r)!\n",
      FunctionName,
      LineNumber,
      Description,
      Status
      );
  }
  return !EFI_ERROR( Status );
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
  if ((ValueA != ValueB)) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTEQUAL,
      "%a::%d Value %a != %a (%d != %d)!\n",
      FunctionName,
      LineNumber,
      DescriptionA,
      DescriptionB,
      ValueA,
      ValueB
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Value %a != %a (%d != %d)!\n",
      FunctionName,
      LineNumber,
      DescriptionA,
      DescriptionB,
      ValueA,
      ValueB
      );
  }
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
  if (CompareMem(BufferA, BufferB, Length) != 0) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTEQUAL,
      "%a::%d Memory at %a != %a for length %d bytes!\n",
      FunctionName,
      LineNumber,
      DescriptionA,
      DescriptionB,
      Length
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Value %a != %a for length %d bytes!\n",
      FunctionName,
      LineNumber,
      DescriptionA,
      DescriptionB,
      Length
      );
    return FALSE;
  }
  return TRUE;
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
  if ((ValueA == ValueB)) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTNOTEQUAL,
      "%a::%d Value %a == %a (%d == %d)!\n",
      FunctionName,
      LineNumber,
      DescriptionA,
      DescriptionB,
      ValueA,
      ValueB
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Value %a == %a (%d == %d)!\n",
      FunctionName,
      LineNumber,
      DescriptionA,
      DescriptionB,
      ValueA,
      ValueB
      );
  }
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
  if ((Status != Expected)) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTSTATUSEQUAL,
      "%a::%d Status '%a' is %r, should be %r!\n",
      FunctionName,
      LineNumber,
      Description,
      Status,
      Expected
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Status '%a' is %r, should be %r!\n",
      FunctionName,
      LineNumber,
      Description,
      Status,
      Expected
      );
  }
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
  if (Pointer == NULL) {
    UnitTestLogFailure (
      FAILURETYPE_ASSERTNOTNULL,
      "%a::%d Pointer (%a) is NULL!\n",
      FunctionName,
      LineNumber,
      PointerName
      );
    UT_LOG_ERROR (
      "[ASSERT FAIL] %a::%d Pointer (%a) is NULL!\n",
      FunctionName,
      LineNumber,
      PointerName
      );
  }
  return (Pointer != NULL);
}
