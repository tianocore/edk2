/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>

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
