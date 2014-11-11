/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

#include <Library/ArmGicLib.h>

#include <Ppi/ArmMpCoreInfo.h>

#include "PrePeiCore.h"

/*
 * This is the main function for secondary cores. They loop around until a non Null value is written to
 * SYS_FLAGS register.The SYS_FLAGS register is platform specific.
 * Note:The secondary cores, while executing secondary_main, assumes that:
 *      : SGI 0 is configured as Non-secure interrupt
 *      : Priority Mask is configured to allow SGI 0
 *      : Interrupt Distributor and CPU interfaces are enabled
 *
 */
VOID
EFIAPI
SecondaryMain (
  IN UINTN MpId
  )
{
  EFI_STATUS              Status;
  UINTN                   PpiListSize;
  UINTN                   PpiListCount;
  EFI_PEI_PPI_DESCRIPTOR  *PpiList;
  ARM_MP_CORE_INFO_PPI    *ArmMpCoreInfoPpi;
  UINTN                   Index;
  UINTN                   ArmCoreCount;
  ARM_CORE_INFO           *ArmCoreInfoTable;
  UINT32                  ClusterId;
  UINT32                  CoreId;
  VOID                    (*SecondaryStart)(VOID);
  UINTN                   SecondaryEntryAddr;
  UINTN                   AcknowledgeInterrupt;
  UINTN                   InterruptId;

  ClusterId = GET_CLUSTER_ID(MpId);
  CoreId    = GET_CORE_ID(MpId);

  // Get the gArmMpCoreInfoPpiGuid
  PpiListSize = 0;
  ArmPlatformGetPlatformPpiList (&PpiListSize, &PpiList);
  PpiListCount = PpiListSize / sizeof(EFI_PEI_PPI_DESCRIPTOR);
  for (Index = 0; Index < PpiListCount; Index++, PpiList++) {
    if (CompareGuid (PpiList->Guid, &gArmMpCoreInfoPpiGuid) == TRUE) {
      break;
    }
  }

  // On MP Core Platform we must implement the ARM MP Core Info PPI
  ASSERT (Index != PpiListCount);

  ArmMpCoreInfoPpi = PpiList->Ppi;
  ArmCoreCount = 0;
  Status = ArmMpCoreInfoPpi->GetMpCoreInfo (&ArmCoreCount, &ArmCoreInfoTable);
  ASSERT_EFI_ERROR (Status);

  // Find the core in the ArmCoreTable
  for (Index = 0; Index < ArmCoreCount; Index++) {
    if ((ArmCoreInfoTable[Index].ClusterId == ClusterId) && (ArmCoreInfoTable[Index].CoreId == CoreId)) {
      break;
    }
  }

  // The ARM Core Info Table must define every core
  ASSERT (Index != ArmCoreCount);

  // Clear Secondary cores MailBox
  MmioWrite32 (ArmCoreInfoTable[Index].MailboxClearAddress, ArmCoreInfoTable[Index].MailboxClearValue);

  do {
    ArmCallWFI ();

    // Read the Mailbox
    SecondaryEntryAddr = MmioRead32 (ArmCoreInfoTable[Index].MailboxGetAddress);

    // Acknowledge the interrupt and send End of Interrupt signal.
    AcknowledgeInterrupt = ArmGicAcknowledgeInterrupt (PcdGet32 (PcdGicInterruptInterfaceBase), &InterruptId);
    // Check if it is a valid interrupt ID
    if (InterruptId < ArmGicGetMaxNumInterrupts (PcdGet32 (PcdGicDistributorBase))) {
      // Got a valid SGI number hence signal End of Interrupt
      ArmGicEndOfInterrupt (PcdGet32 (PcdGicInterruptInterfaceBase), AcknowledgeInterrupt);
    }
  } while (SecondaryEntryAddr == 0);

  // Jump to secondary core entry point.
  SecondaryStart = (VOID (*)())SecondaryEntryAddr;
  SecondaryStart();

  // The secondaries shouldn't reach here
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

  // Enable the GIC Distributor
  ArmGicEnableDistributor (PcdGet32(PcdGicDistributorBase));

  // If ArmVe has not been built as Standalone then we need to wake up the secondary cores
  if (FeaturePcdGet (PcdSendSgiToBringUpSecondaryCores)) {
    // Sending SGI to all the Secondary CPU interfaces
    ArmGicSendSgiTo (PcdGet32(PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E, PcdGet32 (PcdGicSgiIntId));
  }

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
  PeiCoreEntryPoint (&SecCoreData, PpiList);
}
