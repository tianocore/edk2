/** @file
*
*  Copyright (c) 2011-2023, Arm Limited. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARMGIC_H_
#define ARMGIC_H_

#include <Library/ArmGicArchLib.h>

// GIC Distributor
#define ARM_GIC_ICDDCR   0x000        // Distributor Control Register
#define ARM_GIC_ICDICTR  0x004        // Interrupt Controller Type Register
#define ARM_GIC_ICDIIDR  0x008        // Implementer Identification Register

// Each reg base below repeats for Number of interrupts / 4 (see GIC spec)
#define ARM_GIC_ICDISR   0x080        // Interrupt Security Registers
#define ARM_GIC_ICDISER  0x100        // Interrupt Set-Enable Registers
#define ARM_GIC_ICDICER  0x180        // Interrupt Clear-Enable Registers
#define ARM_GIC_ICDSPR   0x200        // Interrupt Set-Pending Registers
#define ARM_GIC_ICDICPR  0x280        // Interrupt Clear-Pending Registers
#define ARM_GIC_ICDABR   0x300        // Active Bit Registers

// Each reg base below repeats for Number of interrupts / 4
#define ARM_GIC_ICDIPR  0x400         // Interrupt Priority Registers

// Each reg base below repeats for Number of interrupts
#define ARM_GIC_ICDIPTR  0x800        // Interrupt Processor Target Registers
#define ARM_GIC_ICDICFR  0xC00        // Interrupt Configuration Registers

#define ARM_GIC_ICDPPISR  0xD00       // PPI Status register

// just one of these
#define ARM_GIC_ICDSGIR  0xF00        // Software Generated Interrupt Register

// GICv3 specific registers
#define ARM_GICD_IROUTER  0x6100       // Interrupt Routing Registers

// GICD_CTLR bits
#define ARM_GIC_ICDDCR_ARE  (1 << 4)     // Affinity Routing Enable (ARE)
#define ARM_GIC_ICDDCR_DS   (1 << 6)     // Disable Security (DS)

// GICD_ICDICFR bits
#define ARM_GIC_ICDICFR_WIDTH            32   // ICDICFR is a 32 bit register
#define ARM_GIC_ICDICFR_BYTES            (ARM_GIC_ICDICFR_WIDTH / 8)
#define ARM_GIC_ICDICFR_F_WIDTH          2    // Each F field is 2 bits
#define ARM_GIC_ICDICFR_F_STRIDE         16   // (32/2) F fields per register
#define ARM_GIC_ICDICFR_F_CONFIG1_BIT    1    // Bit number within F field
#define ARM_GIC_ICDICFR_LEVEL_TRIGGERED  0x0  // Level triggered interrupt
#define ARM_GIC_ICDICFR_EDGE_TRIGGERED   0x1  // Edge triggered interrupt

// GIC Redistributor
#define ARM_GICR_CTLR_FRAME_SIZE          SIZE_64KB
#define ARM_GICR_SGI_PPI_FRAME_SIZE       SIZE_64KB
#define ARM_GICR_SGI_VLPI_FRAME_SIZE      SIZE_64KB
#define ARM_GICR_SGI_RESERVED_FRAME_SIZE  SIZE_64KB

// GIC Redistributor Control frame
#define ARM_GICR_TYPER  0x0008          // Redistributor Type Register

// GIC Redistributor TYPER bit assignments
#define ARM_GICR_TYPER_PLPIS      (1 << 0)                // Physical LPIs
#define ARM_GICR_TYPER_VLPIS      (1 << 1)                // Virtual LPIs
#define ARM_GICR_TYPER_DIRECTLPI  (1 << 3)                // Direct LPIs
#define ARM_GICR_TYPER_LAST       (1 << 4)                // Last Redistributor in series
#define ARM_GICR_TYPER_DPGS       (1 << 5)                // Disable Processor Group
                                                          // Selection Support
#define ARM_GICR_TYPER_PROCNO        (0xFFFF << 8)         // Processor Number
#define ARM_GICR_TYPER_COMMONLPIAFF  (0x3 << 24)           // Common LPI Affinity
#define ARM_GICR_TYPER_AFFINITY      (0xFFFFFFFFULL << 32) // Redistributor Affinity

#define ARM_GICR_TYPER_GET_AFFINITY(TypeReg)  (((TypeReg) & \
                                                ARM_GICR_TYPER_AFFINITY) >> 32)

// GIC SGI & PPI Redistributor frame
#define ARM_GICR_ISENABLER  0x0100      // Interrupt Set-Enable Registers
#define ARM_GICR_ICENABLER  0x0180      // Interrupt Clear-Enable Registers

// GIC Cpu interface
#define ARM_GIC_ICCICR   0x00         // CPU Interface Control Register
#define ARM_GIC_ICCPMR   0x04         // Interrupt Priority Mask Register
#define ARM_GIC_ICCBPR   0x08         // Binary Point Register
#define ARM_GIC_ICCIAR   0x0C         // Interrupt Acknowledge Register
#define ARM_GIC_ICCEIOR  0x10         // End Of Interrupt Register
#define ARM_GIC_ICCRPR   0x14         // Running Priority Register
#define ARM_GIC_ICCPIR   0x18         // Highest Pending Interrupt Register
#define ARM_GIC_ICCABPR  0x1C         // Aliased Binary Point Register
#define ARM_GIC_ICCIIDR  0xFC         // Identification Register

#define ARM_GIC_ICDSGIR_FILTER_TARGETLIST    0x0
#define ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE  0x1
#define ARM_GIC_ICDSGIR_FILTER_ITSELF        0x2

// Bit-masks to configure the CPU Interface Control register
#define ARM_GIC_ICCICR_ENABLE_SECURE         0x01
#define ARM_GIC_ICCICR_ENABLE_NS             0x02
#define ARM_GIC_ICCICR_ACK_CTL               0x04
#define ARM_GIC_ICCICR_SIGNAL_SECURE_TO_FIQ  0x08
#define ARM_GIC_ICCICR_USE_SBPR              0x10

// Bit Mask for GICC_IIDR
#define ARM_GIC_ICCIIDR_GET_PRODUCT_ID(IccIidr)    (((IccIidr) >> 20) & 0xFFF)
#define ARM_GIC_ICCIIDR_GET_ARCH_VERSION(IccIidr)  (((IccIidr) >> 16) & 0xF)
#define ARM_GIC_ICCIIDR_GET_REVISION(IccIidr)      (((IccIidr) >> 12) & 0xF)
#define ARM_GIC_ICCIIDR_GET_IMPLEMENTER(IccIidr)   ((IccIidr) & 0xFFF)

// Bit Mask for
#define ARM_GIC_ICCIAR_ACKINTID  0x3FF

UINT32
EFIAPI
ArmGicGetInterfaceIdentification (
  IN  UINTN  GicInterruptInterfaceBase
  );

// GIC Secure interfaces
VOID
EFIAPI
ArmGicSetupNonSecure (
  IN  UINTN  MpId,
  IN  UINTN  GicDistributorBase,
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicSetSecureInterrupts (
  IN  UINTN  GicDistributorBase,
  IN  UINTN  *GicSecureInterruptMask,
  IN  UINTN  GicSecureInterruptMaskSize
  );

VOID
EFIAPI
ArmGicEnableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicDisableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicEnableDistributor (
  IN  UINTN  GicDistributorBase
  );

VOID
EFIAPI
ArmGicDisableDistributor (
  IN  UINTN  GicDistributorBase
  );

UINTN
EFIAPI
ArmGicGetMaxNumInterrupts (
  IN  UINTN  GicDistributorBase
  );

VOID
EFIAPI
ArmGicSendSgiTo (
  IN  UINTN  GicDistributorBase,
  IN  UINT8  TargetListFilter,
  IN  UINT8  CPUTargetList,
  IN  UINT8  SgiId
  );

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
  IN  UINTN  GicInterruptInterfaceBase,
  OUT UINTN  *InterruptId
  );

VOID
EFIAPI
ArmGicEndOfInterrupt (
  IN  UINTN  GicInterruptInterfaceBase,
  IN UINTN   Source
  );

UINTN
EFIAPI
ArmGicSetPriorityMask (
  IN  UINTN  GicInterruptInterfaceBase,
  IN  INTN   PriorityMask
  );

VOID
EFIAPI
ArmGicSetInterruptPriority (
  IN UINTN   GicDistributorBase,
  IN UINTN   GicRedistributorBase,
  IN UINTN   Source,
  IN UINT32  Priority
  );

VOID
EFIAPI
ArmGicEnableInterrupt (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

VOID
EFIAPI
ArmGicDisableInterrupt (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

BOOLEAN
EFIAPI
ArmGicIsInterruptEnabled (
  IN UINTN  GicDistributorBase,
  IN UINTN  GicRedistributorBase,
  IN UINTN  Source
  );

// GIC revision 2 specific declarations

// Interrupts from 1020 to 1023 are considered as special interrupts
// (eg: spurious interrupts)
#define ARM_GIC_IS_SPECIAL_INTERRUPTS(Interrupt) \
          (((Interrupt) >= 1020) && ((Interrupt) <= 1023))

VOID
EFIAPI
ArmGicV2SetupNonSecure (
  IN  UINTN  MpId,
  IN  UINTN  GicDistributorBase,
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EnableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2DisableInterruptInterface (
  IN  UINTN  GicInterruptInterfaceBase
  );

UINTN
EFIAPI
ArmGicV2AcknowledgeInterrupt (
  IN  UINTN  GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EndOfInterrupt (
  IN UINTN  GicInterruptInterfaceBase,
  IN UINTN  Source
  );

// GIC revision 3 specific declarations

#define ICC_SRE_EL2_SRE  (1 << 0)

#define ARM_GICD_IROUTER_IRM  BIT31

UINT32
EFIAPI
ArmGicV3GetControlSystemRegisterEnable (
  VOID
  );

VOID
EFIAPI
ArmGicV3SetControlSystemRegisterEnable (
  IN UINT32  ControlSystemRegisterEnable
  );

VOID
EFIAPI
ArmGicV3EnableInterruptInterface (
  VOID
  );

VOID
EFIAPI
ArmGicV3DisableInterruptInterface (
  VOID
  );

UINTN
EFIAPI
ArmGicV3AcknowledgeInterrupt (
  VOID
  );

VOID
EFIAPI
ArmGicV3EndOfInterrupt (
  IN UINTN  Source
  );

VOID
ArmGicV3SetBinaryPointer (
  IN UINTN  BinaryPoint
  );

VOID
ArmGicV3SetPriorityMask (
  IN UINTN  Priority
  );

#endif // ARMGIC_H_
