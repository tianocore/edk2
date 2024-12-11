/** @file
  Entry point to a Standalone MM driver that does not update the stack cookie dynamically.

Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2016 - 2018, ARM Ltd. All rights reserved.<BR>
Copyright (c) 2018, Linaro, Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/StandaloneMmDriverEntryPoint.h>

extern
EFI_STATUS
EFIAPI
_CModuleEntryPoint (
  IN EFI_HANDLE              ImageHandle,
  IN IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  );

/**
  The entry point of PE/COFF Image for a Standalone MM Driver.

  This function is the entry point for a Standalone MM Driver.
  This function must call ProcessLibraryConstructorList() and
  ProcessModuleEntryPointList().
  If the return status from ProcessModuleEntryPointList()
  is an error status, then ProcessLibraryDestructorList() must be called.
  The return value from ProcessModuleEntryPointList() is returned.
  If _gMmRevision is not zero and SystemTable->Hdr.Revision is
  less than _gMmRevision, then return EFI_INCOMPATIBLE_VERSION.

  @param  ImageHandle    The image handle of the Standalone MM Driver.
  @param  MmSystemTable  A pointer to the MM System Table.

  @retval  EFI_SUCCESS               The Standalone MM Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gMmRevision is greater than
                                     MmSystemTable->Hdr.Revision.
  @retval  Other                     Return value from
                                     ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE              ImageHandle,
  IN IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  //
  // Call constructor for all libraries
  //
  return _CModuleEntryPoint (ImageHandle, MmSystemTable);
}
