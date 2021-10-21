/** @file
  Library used for sorting routines.

  Copyright (c) 2009 - 2021, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/UnicodeCollation.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SortLib.h>
#include <Library/DevicePathLib.h>

STATIC EFI_UNICODE_COLLATION_PROTOCOL   *mUnicodeCollation = NULL;

#define USL_FREE_NON_NULL(Pointer)  \
{                                     \
  if ((Pointer) != NULL) {            \
  FreePool((Pointer));                \
  (Pointer) = NULL;                   \
  }                                   \
}

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
  IN OUT VOID                           *BufferToSort,
  IN CONST UINTN                        Count,
  IN CONST UINTN                        ElementSize,
  IN       SORT_COMPARE                 CompareFunction
  )
{
  VOID  *Buffer;

  ASSERT(BufferToSort     != NULL);
  ASSERT(CompareFunction  != NULL);

  Buffer = AllocateZeroPool(ElementSize);
  ASSERT(Buffer != NULL);

  QuickSort (
    BufferToSort,
    Count,
    ElementSize,
    CompareFunction,
    Buffer
    );

  FreePool(Buffer);
  return;
}

/**
  Function to compare 2 device paths for use in QuickSort.

  @param[in] Buffer1            pointer to Device Path poiner to compare
  @param[in] Buffer2            pointer to second DevicePath pointer to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
EFIAPI
DevicePathCompare (
  IN  CONST VOID             *Buffer1,
  IN  CONST VOID             *Buffer2
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath1;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath2;
  CHAR16                    *TextPath1;
  CHAR16                    *TextPath2;
  EFI_STATUS                Status;
  INTN                      RetVal;

  DevicePath1 = *(EFI_DEVICE_PATH_PROTOCOL**)Buffer1;
  DevicePath2 = *(EFI_DEVICE_PATH_PROTOCOL**)Buffer2;

  if (DevicePath1 == NULL) {
    if (DevicePath2 == NULL) {
      return 0;
    }

    return -1;
  }

  if (DevicePath2 == NULL) {
    return 1;
  }

  if (mUnicodeCollation == NULL) {
    Status = gBS->LocateProtocol(
      &gEfiUnicodeCollation2ProtocolGuid,
      NULL,
      (VOID**)&mUnicodeCollation);

    ASSERT_EFI_ERROR(Status);
  }

  TextPath1 = ConvertDevicePathToText(
    DevicePath1,
    FALSE,
    FALSE);

  TextPath2 = ConvertDevicePathToText(
    DevicePath2,
    FALSE,
    FALSE);

  if (TextPath1 == NULL) {
    RetVal = -1;
  } else if (TextPath2 == NULL) {
    RetVal = 1;
  } else {
    RetVal = mUnicodeCollation->StriColl(
      mUnicodeCollation,
      TextPath1,
      TextPath2);
  }

  USL_FREE_NON_NULL(TextPath1);
  USL_FREE_NON_NULL(TextPath2);

  return (RetVal);
}

/**
  Function to compare 2 strings without regard to case of the characters.

  @param[in] Buffer1            Pointer to String to compare.
  @param[in] Buffer2            Pointer to second String to compare.

  @retval 0                     Buffer1 equal to Buffer2.
  @retval <0                    Buffer1 is less than Buffer2.
  @retval >0                    Buffer1 is greater than Buffer2.
**/
INTN
EFIAPI
StringNoCaseCompare (
  IN  CONST VOID             *Buffer1,
  IN  CONST VOID             *Buffer2
  )
{
  EFI_STATUS                Status;
  if (mUnicodeCollation == NULL) {
    Status = gBS->LocateProtocol(
      &gEfiUnicodeCollation2ProtocolGuid,
      NULL,
      (VOID**)&mUnicodeCollation);

    ASSERT_EFI_ERROR(Status);
  }

  return (mUnicodeCollation->StriColl(
    mUnicodeCollation,
    *(CHAR16**)Buffer1,
    *(CHAR16**)Buffer2));
}


/**
  Function to compare 2 strings.

  @param[in] Buffer1            Pointer to String to compare (CHAR16**).
  @param[in] Buffer2            Pointer to second String to compare (CHAR16**).

  @retval 0                     Buffer1 equal to Buffer2.
  @retval <0                    Buffer1 is less than Buffer2.
  @retval >0                    Buffer1 is greater than Buffer2.
**/
INTN
EFIAPI
StringCompare (
  IN  CONST VOID                *Buffer1,
  IN  CONST VOID                *Buffer2
  )
{
  return (StrCmp(
    *(CHAR16**)Buffer1,
    *(CHAR16**)Buffer2));
}
