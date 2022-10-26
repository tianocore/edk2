/** @file

  Copyright (c) 2011-2014, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmGicLib.h>

#include <Ppi/ArmMpCoreInfo.h>

#include "PrePeiCore.h"

VOID
EFIAPI
PrimaryMain (
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  EFI_SEC_PEI_HAND_OFF    SecCoreData;
  UINTN                   PpiListSize;
  EFI_PEI_PPI_DESCRIPTOR  *PpiList;
  UINTN                   TemporaryRamBase;
  UINTN                   TemporaryRamSize;

  CreatePpiList (&PpiListSize, &PpiList);

  // Enable the GIC Distributor
  ArmGicEnableDistributor (PcdGet64 (PcdGicDistributorBase));

  // If ArmVe has not been built as Standalone then we need to wake up the secondary cores
  if (FeaturePcdGet (PcdSendSgiToBringUpSecondaryCores)) {
    // Sending SGI to all the Secondary CPU interfaces
    ArmGicSendSgiTo (PcdGet64 (PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E, PcdGet32 (PcdGicSgiIntId));
  }

  // Adjust the Temporary Ram as the new Ppi List (Common + Platform Ppi Lists) is created at
  // the base of the primary core stack
  PpiListSize      = ALIGN_VALUE (PpiListSize, CPU_STACK_ALIGNMENT);
  TemporaryRamBase = (UINTN)PcdGet64 (PcdCPUCoresStackBase) + PpiListSize;
  TemporaryRamSize = (UINTN)PcdGet32 (PcdCPUCorePrimaryStackSize) - PpiListSize;

  //
  // Bind this information into the SEC hand-off state
  // Note: this must be in sync with the stuff in the asm file
  // Note also:  HOBs (pei temp ram) MUST be above stack
  //
  SecCoreData.DataSize               = sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = (VOID *)(UINTN)PcdGet64 (PcdFvBaseAddress);
  SecCoreData.BootFirmwareVolumeSize = PcdGet32 (PcdFvSize);
  SecCoreData.TemporaryRamBase       = (VOID *)TemporaryRamBase; // We run on the primary core (and so we use the first stack)
  SecCoreData.TemporaryRamSize       = TemporaryRamSize;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = ALIGN_VALUE (SecCoreData.TemporaryRamSize / 2, CPU_STACK_ALIGNMENT);
  SecCoreData.StackBase              = (VOID *)((UINTN)SecCoreData.TemporaryRamBase + SecCoreData.PeiTemporaryRamSize);
  SecCoreData.StackSize              = (TemporaryRamBase + TemporaryRamSize) - (UINTN)SecCoreData.StackBase;

  // Jump to PEI core entry point
  PeiCoreEntryPoint (&SecCoreData, PpiList);
}
