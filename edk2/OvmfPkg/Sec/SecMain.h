/** @file
  Header file for SEC code

  Copyright (c) 2008 - 2009, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLATFORM_SECMAIN_H_
#define _PLATFORM_SECMAIN_H_

VOID
EFIAPI
PeiSwitchStacks (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *Context3,  OPTIONAL
  IN      VOID                      *OldTopOfStack,
  IN      VOID                      *NewStack
  );

VOID
EFIAPI
SecSwitchStack (
  IN UINTN   TemporaryMemoryBase,
  IN UINTN   PermanentMemoryBase,
  IN UINTN   CopySize
  );

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

VOID
EFIAPI
FindPeiCoreEntryPoint (
  IN  EFI_FIRMWARE_VOLUME_HEADER       **BootFirmwareVolumePtr,
  OUT VOID                             **PeiCoreEntryPoint
  );

#define INITIAL_TOP_OF_STACK      BASE_512KB

#endif // _PLATFORM_SECMAIN_H_

