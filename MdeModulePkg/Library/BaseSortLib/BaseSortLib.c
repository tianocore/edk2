/** @file
  Library used for sorting routines.

  Copyright (c) 2009 - 2021, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SortLib.h>

/**
  Function to perform a Quick Sort alogrithm on a buffer of comparable elements.

  Each element must be equal sized.

  if BufferToSort is NULL, then ASSERT.
  if CompareFunction is NULL, then ASSERT.

  if Count is < 2 then perform no action.
  if Size is < 1 then perform no action.

  @param[in, out] BufferToSort   on call a Buffer of (possibly sorted) elements
                                 on return a buffer of sorted elements
  @param[in] Count               the number of elements in the buffer to sort
  @param[in] ElementSize         Size of an element in bytes
  @param[in] CompareFunction     The function to call to perform the comparison
                                 of any 2 elements
**/
VOID
EFIAPI
PerformQuickSort (
  IN OUT VOID            *BufferToSort,
  IN CONST UINTN         Count,
  IN CONST UINTN         ElementSize,
  IN       SORT_COMPARE  CompareFunction
  )
{
  VOID  *Buffer;

  ASSERT (BufferToSort     != NULL);
  ASSERT (CompareFunction  != NULL);

  Buffer = AllocateZeroPool (ElementSize);
  ASSERT (Buffer != NULL);

  QuickSort (
    BufferToSort,
    Count,
    ElementSize,
    CompareFunction,
    Buffer
    );

  FreePool (Buffer);
  return;
}

/**
  Not supported in Base version.

  @param[in] Buffer1  Ignored.
  @param[in] Buffer2  Ignored.

  ASSERT and return 0.
**/
INTN
EFIAPI
DevicePathCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Not supported in Base version.

  @param[in] Buffer1  Ignored.
  @param[in] Buffer2  Ignored.

  ASSERT and return 0.
**/
INTN
EFIAPI
StringNoCaseCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  )
{
  ASSERT (FALSE);
  return 0;
}

/**
  Not supported in Base version.

  @param[in] Buffer1  Ignored.
  @param[in] Buffer2  Ignored.

  ASSERT and return 0.
**/
INTN
EFIAPI
StringCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  )
{
  ASSERT (FALSE);
  return 0;
}
