/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Chipset/ArmV7.h>

#include "PrePeiCore.h"

VOID
EFIAPI
SecondaryMain (
  IN UINTN MpId
  )
{
  ASSERT(FALSE);
}

VOID
EFIAPI
PrimaryMain (
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;


  //
  // Bind this information into the SEC hand-off state
  // Note: this must be in sync with the stuff in the asm file
  // Note also:  HOBs (pei temp ram) MUST be above stack
  //
  SecCoreData.DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = (VOID *)(UINTN)PcdGet32 (PcdFvBaseAddress);
  SecCoreData.BootFirmwareVolumeSize = PcdGet32 (PcdFvSize);
  SecCoreData.TemporaryRamBase       = (VOID *)(UINTN)PcdGet32 (PcdCPUCorePrimaryStackSize); // We consider we run on the primary core (and so we use the first stack)
  SecCoreData.TemporaryRamSize       = (UINTN)(UINTN)PcdGet32 (PcdCPUCorePrimaryStackSize);
  SecCoreData.PeiTemporaryRamBase    = (VOID *)((UINTN)(SecCoreData.TemporaryRamBase) + (SecCoreData.TemporaryRamSize / 2));
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize / 2;
  SecCoreData.StackBase              = (VOID *)((UINTN)(SecCoreData.TemporaryRamBase) + (SecCoreData.TemporaryRamSize/2));
  SecCoreData.StackSize              = SecCoreData.TemporaryRamSize / 2;

  // jump to pei core entry point
  (PeiCoreEntryPoint)(&SecCoreData, (VOID *)&gSecPpiTable);
}
