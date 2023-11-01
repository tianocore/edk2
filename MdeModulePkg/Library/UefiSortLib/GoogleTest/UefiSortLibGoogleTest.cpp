/** @file
  Unit tests for the implementation of UefiSortLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/SortLib.h>
}

using namespace testing;

INTN
EFIAPI
CompareUint32 (
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

// Test PerformQuickSort() API from UefiSortLib to verify a UINT32 array
// with 9 elements in ascending order is sorted into descending order.
TEST (PerformQuickSortTest, SortUint32AscendingArray_Size9) {
  CONST UINT32  ArraySize = 9;
  UINT32        BuffActual[ArraySize];
  UINT32        BuffExpected[ArraySize];

  for (UINT32 Index = 0; Index < ArraySize; Index++) {
    BuffActual[Index]   = Index + 1;
    BuffExpected[Index] = ArraySize - Index;
  }

  PerformQuickSort (BuffActual, (UINTN)ArraySize, sizeof (UINT32), (SORT_COMPARE)CompareUint32);
  EXPECT_THAT (BuffActual, ElementsAreArray (BuffExpected, ArraySize));
}

// Test StringCompare() API from UefiSortLib to verify the comparison
// succeeds when the same buffer is compared with itself.
TEST (StringCompareTest, CompareSameBuffer) {
  INTN          RetVal;
  CONST CHAR16  *Buffer = (CHAR16 *)L"abcdefg";

  RetVal = StringCompare (&Buffer, &Buffer);
  EXPECT_EQ (RetVal, 0);
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
