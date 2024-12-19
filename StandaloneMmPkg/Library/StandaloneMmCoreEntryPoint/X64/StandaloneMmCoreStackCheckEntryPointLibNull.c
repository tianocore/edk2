/** @file
  Entry point to the Standalone Mm Core that does not do any dynamic stack cookie updating.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/StandaloneMmCoreEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>

extern
VOID
EFIAPI
_CModuleEntryPoint (
  IN VOID  *HobStart
  );

/**
  The entry point of PE/COFF Image for the STANDALONE MM Core.

  This function is the entry point for the STANDALONE MM Core. This function is required to call
  ProcessModuleEntryPointList() and ProcessModuleEntryPointList() is never expected to return.
  The STANDALONE MM Core is responsible for calling ProcessLibraryConstructorList() as soon as the EFI
  System Table and the image handle for the STANDALONE MM Core itself have been established.
  If ProcessModuleEntryPointList() returns, then ASSERT() and halt the system.

  @param  HobStart  Pointer to the beginning of the HOB List passed in from the PEI Phase.

**/
VOID
EFIAPI
_ModuleEntryPoint (
  IN VOID  *HobStart
  )
{
  //
  // Call the Standalone MM Core entry point
  //
  _CModuleEntryPoint (HobStart);
}
