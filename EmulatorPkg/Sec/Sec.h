/*++ @file
  Stub SEC that is called from the OS application that is the root of the emulator.

  The OS application will call the SEC with the PEI Entry Point API.

Copyright (c) 2011, Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SEC_H___
#define __SEC_H___

#include <PiPei.h>
#include <Library/EmuMagicPageLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/BaseMemoryLib.h>

#include <Ppi/TemporaryRamSupport.h>

//
// I think this should be defined in a MdePkg include file?
//
VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  );

#endif
