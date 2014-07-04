/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
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
#include <Library/IoLib.h>

UINTN
EFIAPI
ArmGicV2AcknowledgeInterrupt (
  IN  UINTN          GicInterruptInterfaceBase
  )
{
  // Read the Interrupt Acknowledge Register
  return MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIAR);
}

VOID
EFIAPI
ArmGicV2EndOfInterrupt (
  IN  UINTN                 GicInterruptInterfaceBase,
  IN UINTN                  Source
  )
{
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCEIOR, Source);
}
