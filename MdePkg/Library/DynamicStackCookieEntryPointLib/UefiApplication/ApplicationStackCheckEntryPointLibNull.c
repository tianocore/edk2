/** @file
  Entry point library instance to a UEFI application that does not update the stack cookie dynamically.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

extern
EFI_STATUS
EFIAPI
_CModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Entry point to UEFI Application.

  This function is the entry point for a UEFI Application. This function must call
  ProcessLibraryConstructorList(), ProcessModuleEntryPointList(), and ProcessLibraryDestructorList().
  The return value from ProcessModuleEntryPointList() is returned.
  If _gUefiDriverRevision is not zero and SystemTable->Hdr.Revision is less than _gUefiDriverRevison,
  then return EFI_INCOMPATIBLE_VERSION.

  @param  ImageHandle                The image handle of the UEFI Application.
  @param  SystemTable                A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The UEFI Application exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Call the module's entry point
  //
  return _CModuleEntryPoint (ImageHandle, SystemTable);
}
