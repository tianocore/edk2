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

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>

#include "GicV2/ArmGicV2Lib.h"

/*
 * This function configures the interrupts set by the mask to be secure.
 *
 */
VOID
EFIAPI
ArmGicSetSecureInterrupts (
  IN  UINTN         GicDistributorBase,
  IN  UINTN*        GicSecureInterruptMask,
  IN  UINTN         GicSecureInterruptMaskSize
  )
{
  UINTN  Index;
  UINT32 InterruptStatus;

  // We must not have more interrupts defined by the mask than the number of available interrupts
  ASSERT(GicSecureInterruptMaskSize <= (ArmGicGetMaxNumInterrupts (GicDistributorBase) / 32));

  // Set all the interrupts defined by the mask as Secure
  for (Index = 0; Index < GicSecureInterruptMaskSize; Index++) {
    InterruptStatus = MmioRead32 (GicDistributorBase + ARM_GIC_ICDISR + (Index * 4));
    MmioWrite32 (GicDistributorBase + ARM_GIC_ICDISR + (Index * 4), InterruptStatus & (~GicSecureInterruptMask[Index]));
  }
}

VOID
EFIAPI
ArmGicEnableDistributor (
  IN  INTN          GicDistributorBase
  )
{
  // Turn on the GIC distributor
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 1);
}

VOID
EFIAPI
ArmGicSetupNonSecure (
  IN  UINTN         MpId,
  IN  INTN          GicDistributorBase,
  IN  INTN          GicInterruptInterfaceBase
  )
{
  ArmGicV2SetupNonSecure (MpId, GicDistributorBase, GicInterruptInterfaceBase);
}
