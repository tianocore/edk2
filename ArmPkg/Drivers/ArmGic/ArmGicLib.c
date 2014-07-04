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

#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/ArmGicLib.h>
#include <Library/PcdLib.h>

UINTN
EFIAPI
ArmGicGetInterfaceIdentification (
  IN  INTN          GicInterruptInterfaceBase
  )
{
  // Read the GIC Identification Register
  return MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIIDR);
}

UINTN
EFIAPI
ArmGicGetMaxNumInterrupts (
  IN  INTN          GicDistributorBase
  )
{
  return 32 * ((MmioRead32 (GicDistributorBase + ARM_GIC_ICDICTR) & 0x1F) + 1);
}

VOID
EFIAPI
ArmGicSendSgiTo (
  IN  INTN          GicDistributorBase,
  IN  INTN          TargetListFilter,
  IN  INTN          CPUTargetList,
  IN  INTN          SgiId
  )
{
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDSGIR, ((TargetListFilter & 0x3) << 24) | ((CPUTargetList & 0xFF) << 16) | SgiId);
}

UINTN
EFIAPI
ArmGicAcknowledgeInterrupt (
  IN  UINTN          GicInterruptInterfaceBase
  )
{
  // Read the Interrupt Acknowledge Register
  return MmioRead32 (GicInterruptInterfaceBase + ARM_GIC_ICCIAR);
}

VOID
EFIAPI
ArmGicEndOfInterrupt (
  IN  UINTN                 GicInterruptInterfaceBase,
  IN UINTN                  Source
  )
{
  MmioWrite32 (GicInterruptInterfaceBase + ARM_GIC_ICCEIOR, Source);
}

VOID
EFIAPI
ArmGicEnableInterrupt (
  IN UINTN                  GicDistributorBase,
  IN UINTN                  Source
  )
{
  UINT32    RegOffset;
  UINTN     RegShift;

  // Calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  // Write set-enable register
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDISER + (4 * RegOffset), 1 << RegShift);
}

VOID
EFIAPI
ArmGicDisableInterrupt (
  IN UINTN                  GicDistributorBase,
  IN UINTN                  Source
  )
{
  UINT32    RegOffset;
  UINTN     RegShift;

  // Calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  // Write clear-enable register
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDICER + (4 * RegOffset), 1 << RegShift);
}

BOOLEAN
EFIAPI
ArmGicIsInterruptEnabled (
  IN UINTN                  GicDistributorBase,
  IN UINTN                  Source
  )
{
  UINT32    RegOffset;
  UINTN     RegShift;

  // Calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  return ((MmioRead32 (GicDistributorBase + ARM_GIC_ICDISER + (4 * RegOffset)) & (1 << RegShift)) != 0);
}
