/** @file
  This library retrieve the EFI_BOOT_SERVICES pointer from EFI system table in
  library's constructor.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

VOID  *gHobList = NULL;

/**
  Local implementation of GUID comparasion that doesn't depend on DebugLib::ASSERT().

  This function compares Guid1 to Guid2.  If the GUIDs are identical then TRUE is returned.
  If there are any bit differences in the two GUIDs, then FALSE is returned.

  @param  Guid1       A pointer to a 128 bit GUID.
  @param  Guid2       A pointer to a 128 bit GUID.

  @retval TRUE        Guid1 and Guid2 are identical.
  @retval FALSE       Guid1 and Guid2 are not identical.
**/
BOOLEAN
LocalCompareGuid (
  IN CONST GUID  *Guid1,
  IN CONST GUID  *Guid2
  )
{
  UINT64  *Left;
  UINT64  *Right;

  Left  = (UINT64 *)Guid1;
  Right = (UINT64 *)Guid2;

  return (BOOLEAN)(Left[0] == Right[0] && Left[1] == Right[1]);
}

/**
  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeHobListLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN  Index;

  for (Index = 0; Index < SystemTable->NumberOfTableEntries; Index++) {
    if (LocalCompareGuid (&gEfiHobListGuid, &SystemTable->ConfigurationTable[Index].VendorGuid)) {
      gHobList = SystemTable->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
