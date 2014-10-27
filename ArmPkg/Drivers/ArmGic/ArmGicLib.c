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
#include <Library/ArmGicLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>

#include "GicV2/ArmGicV2Lib.h"
#include "GicV3/ArmGicV3Lib.h"

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

/*
 * Acknowledge and return the value of the Interrupt Acknowledge Register
 *
 * InterruptId is returned separately from the register value because in
 * the GICv2 the register value contains the CpuId and InterruptId while
 * in the GICv3 the register value is only the InterruptId.
 *
 * @param GicInterruptInterfaceBase   Base Address of the GIC CPU Interface
 * @param InterruptId                 InterruptId read from the Interrupt Acknowledge Register
 *
 * @retval value returned by the Interrupt Acknowledge Register
 *
 */
UINTN
EFIAPI
ArmGicAcknowledgeInterrupt (
  IN  UINTN          GicInterruptInterfaceBase,
  OUT UINTN          *InterruptId
  )
{
  UINTN Value;
  ARM_GIC_ARCH_REVISION Revision;

  Revision = ArmGicGetSupportedArchRevision ();
  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    Value = ArmGicV2AcknowledgeInterrupt (GicInterruptInterfaceBase);
    // InterruptId is required for the caller to know if a valid or spurious
    // interrupt has been read
    ASSERT (InterruptId != NULL);
    if (InterruptId != NULL) {
      *InterruptId = Value & ARM_GIC_ICCIAR_ACKINTID;
    }
  } else if (Revision == ARM_GIC_ARCH_REVISION_3) {
    Value = ArmGicV3AcknowledgeInterrupt ();
  } else {
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
    // Report Spurious interrupt which is what the above controllers would
    // return if no interrupt was available
    Value = 1023;
  }

  return Value;
}

VOID
EFIAPI
ArmGicEndOfInterrupt (
  IN  UINTN                 GicInterruptInterfaceBase,
  IN UINTN                  Source
  )
{
  ARM_GIC_ARCH_REVISION Revision;

  Revision = ArmGicGetSupportedArchRevision ();
  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    ArmGicV2EndOfInterrupt (GicInterruptInterfaceBase, Source);
  } else if (Revision == ARM_GIC_ARCH_REVISION_3) {
    ArmGicV3EndOfInterrupt (Source);
  } else {
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  }
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

VOID
EFIAPI
ArmGicDisableDistributor (
  IN  INTN          GicDistributorBase
  )
{
  // Disable Gic Distributor
  MmioWrite32 (GicDistributorBase + ARM_GIC_ICDDCR, 0x0);
}

VOID
EFIAPI
ArmGicEnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  )
{
  ARM_GIC_ARCH_REVISION Revision;

  Revision = ArmGicGetSupportedArchRevision ();
  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    ArmGicV2EnableInterruptInterface (GicInterruptInterfaceBase);
  } else if (Revision == ARM_GIC_ARCH_REVISION_3) {
    ArmGicV3EnableInterruptInterface ();
  } else {
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  }
}

VOID
EFIAPI
ArmGicDisableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  )
{
  ARM_GIC_ARCH_REVISION Revision;

  Revision = ArmGicGetSupportedArchRevision ();
  if (Revision == ARM_GIC_ARCH_REVISION_2) {
    ArmGicV2DisableInterruptInterface (GicInterruptInterfaceBase);
  } else if (Revision == ARM_GIC_ARCH_REVISION_3) {
    ArmGicV3DisableInterruptInterface ();
  } else {
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
  }
}
