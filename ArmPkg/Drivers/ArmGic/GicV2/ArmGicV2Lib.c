/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/ArmGicLib.h>
#include <Library/IoLib.h>

UINTN
EFIAPI
ArmGicV2AcknowledgeInterrupt (
  IN  UINTN  GicInterruptInterfaceBase
  )
{
  // Read the Interrupt Acknowledge Register
  return MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIAR);
}

VOID
EFIAPI
ArmGicV2EndOfInterrupt (
  IN  UINTN  GicInterruptInterfaceBase,
  IN UINTN   Source
  )
{
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCEIOR, Source);
}
