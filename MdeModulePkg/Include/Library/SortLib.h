/** @file
  Library used for sorting and comparison routines.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SORT_LIB_H__
#define __SORT_LIB_H__

/**
  Prototype for comparison function for any two element types.

  @param[in] Buffer1                  The pointer to first buffer.
  @param[in] Buffer2                  The pointer to second buffer.

  @retval 0                           Buffer1 equal to Buffer2.
  @return <0                          Buffer1 is less than Buffer2.
  @return >0                          Buffer1 is greater than Buffer2.
**/
typedef
INTN
(EFIAPI *SORT_COMPARE)(
  IN CONST VOID                 *Buffer1,
  IN CONST VOID                 *Buffer2
  );

/**
  Function to perform a Quick Sort on a buffer of comparable elements.

  Each element must be equally sized.

  If BufferToSort is NULL, then ASSERT.
  If CompareFunction is NULL, then ASSERT.

  If Count is < 2 , then perform no action.
  If Size is < 1 , then perform no action.

  @param[in, out] BufferToSort   On call, a Buffer of (possibly sorted) elements;
                                 on return, a buffer of sorted elements.
  @param[in]  Count              The number of elements in the buffer to sort.
  @param[in]  ElementSize        The size of an element in bytes.
  @param[in]  CompareFunction    The function to call to perform the comparison
                                 of any two elements.
**/
VOID
EFIAPI
PerformQuickSort (
  IN OUT VOID            *BufferToSort,
  IN CONST UINTN         Count,
  IN CONST UINTN         ElementSize,
  IN       SORT_COMPARE  CompareFunction
  );

/**
  Function to compare 2 device paths for use as CompareFunction.

  @param[in] Buffer1            The pointer to Device Path to compare.
  @param[in] Buffer2            The pointer to second DevicePath to compare.

  @retval 0                     Buffer1 equal to Buffer2.
  @return < 0                   Buffer1 is less than Buffer2.
  @return > 0                   Buffer1 is greater than Buffer2.
**/
INTN
EFIAPI
DevicePathCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  );

/**
  Function to compare 2 strings without regard to case of the characters.

  @param[in] Buffer1            The pointer to String to compare (CHAR16**).
  @param[in] Buffer2            The pointer to second String to compare (CHAR16**).

  @retval 0                     Buffer1 equal to Buffer2.
  @return < 0                   Buffer1 is less than Buffer2.
  @return > 0                   Buffer1 is greater than Buffer2.
**/
INTN
EFIAPI
StringNoCaseCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  );

/**
  Function to compare 2 strings.

  @param[in] Buffer1            The pointer to String to compare (CHAR16**).
  @param[in] Buffer2            The pointer to second String to compare (CHAR16**).

  @retval 0                     Buffer1 equal to Buffer2.
  @return < 0                   Buffer1 is less than Buffer2.
  @return > 0                   Buffer1 is greater than Buffer2.
**/
INTN
EFIAPI
StringCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  );

#endif //__SORT_LIB_H__
