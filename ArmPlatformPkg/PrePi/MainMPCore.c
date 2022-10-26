/** @file

  Copyright (c) 2011-2014, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PrePi.h"

#include <Library/ArmGicLib.h>

#include <Ppi/ArmMpCoreInfo.h>

VOID
PrimaryMain (
  IN  UINTN   UefiMemoryBase,
  IN  UINTN   StacksBase,
  IN  UINT64  StartTimeStamp
  )
{
  // Enable the GIC Distributor
  ArmGicEnableDistributor (PcdGet64 (PcdGicDistributorBase));

  // In some cases, the secondary cores are waiting for an SGI from the next stage boot loader to resume their initialization
  if (!FixedPcdGet32 (PcdSendSgiToBringUpSecondaryCores)) {
    // Sending SGI to all the Secondary CPU interfaces
    ArmGicSendSgiTo (PcdGet64 (PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E, PcdGet32 (PcdGicSgiIntId));
  }

  PrePiMain (UefiMemoryBase, StacksBase, StartTimeStamp);

  // We must never return
  ASSERT (FALSE);
}
