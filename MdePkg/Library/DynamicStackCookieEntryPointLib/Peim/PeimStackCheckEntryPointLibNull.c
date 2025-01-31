/** @file
  Entry point to a PEIM that does not update the stack cookie dynamically.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/DebugLib.h>

extern
EFI_STATUS
EFIAPI
_CModuleEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  );

/**
  The entry point of PE/COFF Image for a PEIM.

  This function is the entry point for a PEIM.  This function must call ProcessLibraryConstructorList()
  and ProcessModuleEntryPointList().  The return value from ProcessModuleEntryPointList() is returned.
  If _gPeimRevision is not zero and PeiServices->Hdr.Revision is less than _gPeimRevison, then ASSERT().

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval  EFI_SUCCESS   The PEIM executed normally.
  @retval  !EFI_SUCCESS  The PEIM failed to execute normally.
**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  //
  // Call the driver entry point
  //
  return _CModuleEntryPoint (FileHandle, PeiServices);
}
