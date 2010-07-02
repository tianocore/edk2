/** @file
  C Entry point for the SEC. First C code after the reset vector.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <ArmEb/ArmEb.h>

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

VOID
SecSwitchStack (
  INTN    StackDelta
  );

TEMPORARY_RAM_SUPPORT_PPI   mSecTemporaryRamSupportPpi = {SecTemporaryRamSupport};

EFI_PEI_PPI_DESCRIPTOR      gSecPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiTemporaryRamSupportPpiGuid,
    &mSecTemporaryRamSupportPpi
  }
};


VOID
EFIAPI 
_ModuleEntryPoint(
  VOID
  );

VOID
CEntryPoint (
  IN  UINTN                     TempRamBase,
  IN  UINTN                     TempRamSize,
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;

  // Turn off remapping NOR to 0. We can will now see DRAM in low memory (although it is not yet initialized)
  // note: this makes SEC platform-specific for the EB platform
  MmioOr32 (0x10001000 ,BIT8); //EB_SP810_CTRL_BASE
  
  //
  // Bind this information into the SEC hand-off state
  // Note: this must be in sync with the stuff in the asm file
  // Note also:  HOBs (pei temp ram) MUST be above stack
  //
  SecCoreData.DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = (VOID *)(UINTN)PcdGet32 (PcdEmbeddedFdBaseAddress);
  SecCoreData.BootFirmwareVolumeSize = PcdGet32 (PcdEmbeddedFdSize);
  SecCoreData.TemporaryRamBase       = (VOID*)(UINTN)TempRamBase; 
  SecCoreData.TemporaryRamSize       = TempRamSize;
  SecCoreData.PeiTemporaryRamBase    = (VOID *)(UINTN)(SecCoreData.TemporaryRamBase + (SecCoreData.TemporaryRamSize / 2));
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize / 2;
  SecCoreData.StackBase              = (VOID *)(UINTN)(SecCoreData.TemporaryRamBase);
  SecCoreData.StackSize              = SecCoreData.TemporaryRamSize - SecCoreData.PeiTemporaryRamSize;
  
  // jump to pei core entry point
  (PeiCoreEntryPoint)(&SecCoreData, (VOID *)&gSecPpiTable);
}

EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  //
  // Migrate the whole temporary memory to permenent memory.
  // 
  CopyMem (
    (VOID*)(UINTN)PermanentMemoryBase, 
    (VOID*)(UINTN)TemporaryMemoryBase, 
    CopySize
    );

  SecSwitchStack((UINTN)(PermanentMemoryBase - TemporaryMemoryBase));

  //
  // We need *not* fix the return address because currently, 
  // The PeiCore is excuted in flash.
  //

  //
  // Simulate to invalid temporary memory, terminate temporary memory
  // 
  //ZeroMem ((VOID*)(UINTN)TemporaryMemoryBase, CopySize);
  
  return EFI_SUCCESS;
}

