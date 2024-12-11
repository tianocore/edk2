/** @file
  Entry point to a EFI/DXE driver that does not update the stack cookie dynamically.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Base.h>

extern
EFI_STATUS
EFIAPI
_CModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  The entry point of PE/COFF Image for a DXE Driver, DXE Runtime Driver, DXE SMM
  Driver, or UEFI Driver.

  This function does not do any stack cookie manipulation, but just directly calls
  _CModuleEntryPoint() to do the real work.

  @param  ImageHandle  The image handle of the DXE Driver, DXE Runtime Driver,
                       DXE SMM Driver, or UEFI Driver.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The DXE Driver, DXE Runtime Driver, DXE SMM
                                     Driver, or UEFI Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than
                                    SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return _CModuleEntryPoint (ImageHandle, SystemTable);
}
