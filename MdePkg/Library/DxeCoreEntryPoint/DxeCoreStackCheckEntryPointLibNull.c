/** @file
  Entry point to the DXE Core that does not update the stack cookie dynamically.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DxeCoreEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

extern
VOID
EFIAPI
_CModuleEntryPoint (
  IN VOID  *HobStart
  );

/**
  The entry point of PE/COFF Image for the DXE Core.

  This function is the entry point for the DXE Core. This function is required to call
  ProcessModuleEntryPointList() and ProcessModuleEntryPointList() is never expected to return.
  The DXE Core is responsible for calling ProcessLibraryConstructorList() as soon as the EFI
  System Table and the image handle for the DXE Core itself have been established.
  If ProcessModuleEntryPointList() returns, then ASSERT() and halt the system.

  @param  HobStart  The pointer to the beginning of the HOB List passed in from the PEI Phase.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID  *HobStart
  )
{
  //
  // Call the DXE Core entry point
  //
  _CModuleEntryPoint (HobStart);
}
