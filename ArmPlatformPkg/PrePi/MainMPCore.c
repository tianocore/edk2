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

#include "PrePi.h"

#include <Library/ArmGicLib.h>
#include <Library/ArmMPCoreMailBoxLib.h>
#include <Chipset/ArmV7.h>

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

  // In some cases, the secondary cores are waiting for an SGI from the next stage boot loader toresume their initialization
  if (!FixedPcdGet32(PcdSendSgiToBringUpSecondaryCores)) {
    // Sending SGI to all the Secondary CPU interfaces
    ArmGicSendSgiTo (PcdGet32(PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E);
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
  // Function pointer to Secondary Core entry point
  VOID (*secondary_start)(VOID);
  UINTN secondary_entry_addr=0;

  // Clear Secondary cores MailBox
  ArmClearMPCoreMailbox();

  while (secondary_entry_addr = ArmGetMPCoreMailbox(), secondary_entry_addr == 0) {
    ArmCallWFI();
    // Acknowledge the interrupt and send End of Interrupt signal.
    ArmGicAcknowledgeSgiFrom (PcdGet32(PcdGicInterruptInterfaceBase), PRIMARY_CORE_ID);
  }

  secondary_start = (VOID (*)())secondary_entry_addr;

  // Jump to secondary core entry point.
  secondary_start();

  // The secondaries shouldn't reach here
  ASSERT(FALSE);
}
