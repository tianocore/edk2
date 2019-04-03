/** @file
*
*  Copyright (c) 2011-2018, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Base.h>
#include <Library/ArmGicLib.h>
#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

// In GICv3, there are 2 x 64KB frames:
// Redistributor control frame + SGI Control & Generation frame
#define GIC_V3_REDISTRIBUTOR_GRANULARITY  (ARM_GICR_CTLR_FRAME_SIZE           \
                                           + ARM_GICR_SGI_PPI_FRAME_SIZE)

// In GICv4, there are 2 additional 64KB frames:
// VLPI frame + Reserved page frame
#define GIC_V4_REDISTRIBUTOR_GRANULARITY  (GIC_V3_REDISTRIBUTOR_GRANULARITY   \
                                           + ARM_GICR_SGI_VLPI_FRAME_SIZE     \
                                           + ARM_GICR_SGI_RESERVED_FRAME_SIZE)

#define ISENABLER_ADDRESS(base,offset) ((base) + \
          ARM_GICR_CTLR_FRAME_SIZE +  ARM_GICR_ISENABLER + (4 * offset))

#define ICENABLER_ADDRESS(base,offset) ((base) + \
          ARM_GICR_CTLR_FRAME_SIZE +  ARM_GICR_ICENABLER + (4 * offset))

/**
 *
 * Return whether the Source interrupt index refers to a shared interrupt (SPI)
 */
STATIC
BOOLEAN
SourceIsSpi (
  IN UINTN  Source
  )
{
  return Source >= 32 && Source < 1020;
}

/**
 * Return the base address of the GIC redistributor for the current CPU
 *
 * @param Revision  GIC Revision. The GIC redistributor might have a different
 *                  granularity following the GIC revision.
 *
 * @retval Base address of the associated GIC Redistributor
 */
STATIC
UINTN
GicGetCpuRedistributorBase (
  IN UINTN                 GicRedistributorBase,
  IN ARM_GIC_ARCH_REVISION Revision
  )
{
  UINTN MpId;
  UINTN CpuAffinity;
  UINTN Affinity;
  UINTN GicCpuRedistributorBase;
  UINT64 TypeRegister;

  MpId = ArmReadMpidr ();
  // Define CPU affinity as:
  // Affinity0[0:8], Affinity1[9:15], Affinity2[16:23], Affinity3[24:32]
  // whereas Affinity3 is defined at [32:39] in MPIDR
  CpuAffinity = (MpId & (ARM_CORE_AFF0 | ARM_CORE_AFF1 | ARM_CORE_AFF2)) |
                ((MpId & ARM_CORE_AFF3) >> 8);

  if (Revision < ARM_GIC_ARCH_REVISION_3) {
    ASSERT_EFI_ERROR (EFI_UNSUPPORTED);
    return 0;
  }

  GicCpuRedistributorBase = GicRedistributorBase;

  do {
    TypeRegister = MmioRead64 (GicCpuRedistributorBase + ARM_GICR_TYPER);
    Affinity = ARM_GICR_TYPER_GET_AFFINITY (TypeRegister);
    if (Affinity == CpuAffinity) {
      return GicCpuRedistributorBase;
    }

    // Move to the next GIC Redistributor frame.
    // The GIC specification does not forbid a mixture of redistributors
    // with or without support for virtual LPIs, so we test Virtual LPIs
    // Support (VLPIS) bit for each frame to decide the granularity.
    // Note: The assumption here is that the redistributors are adjacent
    // for all CPUs. However this may not be the case for NUMA systems.
    GicCpuRedistributorBase += (((ARM_GICR_TYPER_VLPIS & TypeRegister) != 0)
                                ? GIC_V4_REDISTRIBUTOR_GRANULARITY
                                : GIC_V3_REDISTRIBUTOR_GRANULARITY);
  } while ((TypeRegister & ARM_GICR_TYPER_LAST) == 0);

  // The Redistributor has not been found for the current CPU
  ASSERT_EFI_ERROR (EFI_NOT_FOUND);
  return 0;
}

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
  MmioWrite32 (
    GicDistributorBase + ARM_GIC_ICDSGIR,
    ((TargetListFilter & 0x3) << 24) | ((CPUTargetList & 0xFF) << 16) | SgiId
    );
}

/*
 * Acknowledge and return the value of the Interrupt Acknowledge Register
 *
 * InterruptId is returned separately from the register value because in
 * the GICv2 the register value contains the CpuId and InterruptId while
 * in the GICv3 the register value is only the InterruptId.
 *
 * @param GicInterruptInterfaceBase   Base Address of the GIC CPU Interface
 * @param InterruptId                 InterruptId read from the Interrupt
 *                                    Acknowledge Register
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
  IN UINTN                  GicRedistributorBase,
  IN UINTN                  Source
  )
{
  UINT32                RegOffset;
  UINTN                 RegShift;
  ARM_GIC_ARCH_REVISION Revision;
  UINTN                 GicCpuRedistributorBase;

  // Calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  Revision = ArmGicGetSupportedArchRevision ();
  if ((Revision == ARM_GIC_ARCH_REVISION_2) ||
      FeaturePcdGet (PcdArmGicV3WithV2Legacy) ||
      SourceIsSpi (Source)) {
    // Write set-enable register
    MmioWrite32 (
      GicDistributorBase + ARM_GIC_ICDISER + (4 * RegOffset),
      1 << RegShift
      );
  } else {
    GicCpuRedistributorBase = GicGetCpuRedistributorBase (
                                GicRedistributorBase,
                                Revision
                                );
    if (GicCpuRedistributorBase == 0) {
      ASSERT_EFI_ERROR (EFI_NOT_FOUND);
      return;
    }

    // Write set-enable register
    MmioWrite32 (
      ISENABLER_ADDRESS(GicCpuRedistributorBase, RegOffset),
      1 << RegShift
      );
  }
}

VOID
EFIAPI
ArmGicDisableInterrupt (
  IN UINTN                  GicDistributorBase,
  IN UINTN                  GicRedistributorBase,
  IN UINTN                  Source
  )
{
  UINT32                RegOffset;
  UINTN                 RegShift;
  ARM_GIC_ARCH_REVISION Revision;
  UINTN                 GicCpuRedistributorBase;

  // Calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  Revision = ArmGicGetSupportedArchRevision ();
  if ((Revision == ARM_GIC_ARCH_REVISION_2) ||
      FeaturePcdGet (PcdArmGicV3WithV2Legacy) ||
      SourceIsSpi (Source)) {
    // Write clear-enable register
    MmioWrite32 (
      GicDistributorBase + ARM_GIC_ICDICER + (4 * RegOffset),
      1 << RegShift
      );
  } else {
    GicCpuRedistributorBase = GicGetCpuRedistributorBase (
      GicRedistributorBase,
      Revision
      );
    if (GicCpuRedistributorBase == 0) {
      return;
    }

    // Write clear-enable register
    MmioWrite32 (
      ICENABLER_ADDRESS(GicCpuRedistributorBase, RegOffset),
      1 << RegShift
      );
  }
}

BOOLEAN
EFIAPI
ArmGicIsInterruptEnabled (
  IN UINTN                  GicDistributorBase,
  IN UINTN                  GicRedistributorBase,
  IN UINTN                  Source
  )
{
  UINT32                RegOffset;
  UINTN                 RegShift;
  ARM_GIC_ARCH_REVISION Revision;
  UINTN                 GicCpuRedistributorBase;
  UINT32                Interrupts;

  // Calculate enable register offset and bit position
  RegOffset = Source / 32;
  RegShift = Source % 32;

  Revision = ArmGicGetSupportedArchRevision ();
  if ((Revision == ARM_GIC_ARCH_REVISION_2) ||
      FeaturePcdGet (PcdArmGicV3WithV2Legacy) ||
      SourceIsSpi (Source)) {
    Interrupts = ((MmioRead32 (
                     GicDistributorBase + ARM_GIC_ICDISER + (4 * RegOffset)
                     )
                  & (1 << RegShift)) != 0);
  } else {
    GicCpuRedistributorBase = GicGetCpuRedistributorBase (
                                GicRedistributorBase,
                                Revision
                                );
    if (GicCpuRedistributorBase == 0) {
      return 0;
    }

    // Read set-enable register
    Interrupts = MmioRead32 (
                   ISENABLER_ADDRESS(GicCpuRedistributorBase, RegOffset)
                   );
  }

  return ((Interrupts & (1 << RegShift)) != 0);
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
