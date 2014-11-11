/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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
  UINTN                       PpiListSize;
  EFI_PEI_PPI_DESCRIPTOR      *PpiList;
  UINTN                       TemporaryRamBase;
  UINTN                       TemporaryRamSize;

  CreatePpiList (&PpiListSize, &PpiList);

  // Adjust the Temporary Ram as the new Ppi List (Common + Platform Ppi Lists) is created at
  // the base of the primary core stack
  PpiListSize = ALIGN_VALUE(PpiListSize, 0x4);
  TemporaryRamBase = (UINTN)PcdGet64 (PcdCPUCoresStackBase) + PpiListSize;
  TemporaryRamSize = (UINTN)PcdGet32 (PcdCPUCorePrimaryStackSize) - PpiListSize;

  // Make sure the size is 8-byte aligned. Once divided by 2, the size should be 4-byte aligned
  // to ensure the stack pointer is 4-byte aligned.
  TemporaryRamSize = TemporaryRamSize - (TemporaryRamSize & (0x8-1));

  //
  // Bind this information into the SEC hand-off state
  // Note: this must be in sync with the stuff in the asm file
  // Note also:  HOBs (pei temp ram) MUST be above stack
  //
  SecCoreData.DataSize               = sizeof(EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = (VOID *)(UINTN)PcdGet64 (PcdFvBaseAddress);
  SecCoreData.BootFirmwareVolumeSize = PcdGet32 (PcdFvSize);
  SecCoreData.TemporaryRamBase       = (VOID *)TemporaryRamBase; // We run on the primary core (and so we use the first stack)
  SecCoreData.TemporaryRamSize       = TemporaryRamSize;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize / 2;
  SecCoreData.StackBase              = (VOID *)ALIGN_VALUE((UINTN)(SecCoreData.TemporaryRamBase) + SecCoreData.PeiTemporaryRamSize, 0x4);
  SecCoreData.StackSize              = (TemporaryRamBase + TemporaryRamSize) - (UINTN)SecCoreData.StackBase;

  // Jump to PEI core entry point
  (PeiCoreEntryPoint)(&SecCoreData, PpiList);
}
