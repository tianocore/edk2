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

#include "PrePi.h"

#include <Library/ArmGicLib.h>

#include <Ppi/ArmMpCoreInfo.h>

VOID
PrimaryMain (
  IN  UINTN                     UefiMemoryBase,
  IN  UINTN                     StacksBase,
  IN  UINTN                     GlobalVariableBase,
  IN  UINT64                    StartTimeStamp
  )
{
  // Enable the GIC Distributor
  ArmGicEnableDistributor(PcdGet32(PcdGicDistributorBase));

  // In some cases, the secondary cores are waiting for an SGI from the next stage boot loader to resume their initialization
  if (!FixedPcdGet32(PcdSendSgiToBringUpSecondaryCores)) {
    // Sending SGI to all the Secondary CPU interfaces
    ArmGicSendSgiTo (PcdGet32(PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E, PcdGet32 (PcdGicSgiIntId));
  }

  PrePiMain (UefiMemoryBase, StacksBase, GlobalVariableBase, StartTimeStamp);

  // We must never return
  ASSERT(FALSE);
}

VOID
SecondaryMain (
  IN  UINTN                     MpId
  )
{
  EFI_STATUS              Status;
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

  // On MP Core Platform we must implement the ARM MP Core Info PPI (gArmMpCoreInfoPpiGuid)
  Status = GetPlatformPpi (&gArmMpCoreInfoPpiGuid, (VOID**)&ArmMpCoreInfoPpi);
  ASSERT_EFI_ERROR (Status);

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
