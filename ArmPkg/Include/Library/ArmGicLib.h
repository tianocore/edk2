/** @file
*
*  Copyright (c) 2011-2023, Arm Limited. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARMGIC_H_
#define ARMGIC_H_

//
// GIC SPI and extended SPI ranges
//
#define ARM_GIC_ARCH_SPI_MIN      32
#define ARM_GIC_ARCH_SPI_MAX      1019
#define ARM_GIC_ARCH_EXT_SPI_MIN  4096
#define ARM_GIC_ARCH_EXT_SPI_MAX  5119

// GIC Distributor
#define ARM_GIC_ICDDCR   0x000        // Distributor Control Register
#define ARM_GIC_ICDICTR  0x004        // Interrupt Controller Type Register
#define ARM_GIC_ICDIIDR  0x008        // Implementer Identification Register

// ICDICTR is also called GICD_TYPER.

// Intids per LSB for EXT_SPI_RANGE and ITLINES.
#define ARM_GIC_ICDICTR_INTID_RANGE_RESOLUTION  (32)

// Converts an register range of IntIds to the maximum IntId using Base as an
// offset.
#define ARM_GIC_ICDICTR_INTID_RANGE_TO_MAX_INTID(Range, Base) \
  (ARM_GIC_ICDICTR_INTID_RANGE_RESOLUTION * ((Range) + 1) - 1 + (Base))

#define ARM_GIC_ICDICTR_ITLINES_MASK   (0x1F)
#define ARM_GIC_ICDICTR_ITLINES_SHIFT  (0)

// Gets the range for SPI IntIds from TypeReg.
#define ARM_GIC_ICDICTR_GET_SPI_RANGE(TypeReg) \
  (((TypeReg) >> ARM_GIC_ICDICTR_ITLINES_SHIFT) & ARM_GIC_ICDICTR_ITLINES_MASK)

// Converts a range of SPI IntIds to the maximum SPI IntId.
#define ARM_GIC_ICDICTR_SPI_RANGE_TO_MAX_INTID(SpiRange) \
  (((SpiRange) == ARM_GIC_ICDICTR_ITLINES_MASK)          \
       ? ARM_GIC_ARCH_SPI_MAX                   \
       : ARM_GIC_ICDICTR_INTID_RANGE_TO_MAX_INTID(SpiRange, 0))

// Extracts the maximum SPI IntId from TypeReg.
#define ARM_GIC_ICDICTR_GET_SPI_MAX_INTID(TypeReg) \
  ARM_GIC_ICDICTR_SPI_RANGE_TO_MAX_INTID(ARM_GIC_ICDICTR_GET_SPI_RANGE(TypeReg))

#define ARM_GIC_ICDICTR_EXT_SPI_ENABLED      (1 << 8) // Extended SPI enabled bit.
#define ARM_GIC_ICDICTR_EXT_SPI_RANGE_SHIFT  (27)     // Extended SPI range position.
#define ARM_GIC_ICDICTR_EXT_SPI_RANGE_MASK   (0x1F)   // Extended SPI range mask.
#define ARM_GIC_ICDICTR_GET_EXT_SPI_RANGE(TypeReg)      \
  (((TypeReg) >> ARM_GIC_ICDICTR_EXT_SPI_RANGE_SHIFT) & \
   ARM_GIC_ICDICTR_EXT_SPI_RANGE_MASK)

// Extracts the maximum EXT SPI IntId from TypeReg.
#define ARM_GIC_ICDICTR_GET_EXT_SPI_MAX_INTID(TypeReg) \
  ARM_GIC_ICDICTR_INTID_RANGE_TO_MAX_INTID(            \
      ARM_GIC_ICDICTR_GET_EXT_SPI_RANGE(TypeReg), ARM_GIC_ARCH_EXT_SPI_MIN)

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
#define ARM_GICD_IROUTER    0x6100    // Interrupt Routing Registers
#define ARM_GICD_IROUTER_E  0x8000    // Interrupt Routing Registers

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

// GICD ESPI registers
//  These registers follow the same bit pattern as the SPI registers.
#define ARM_GIC_ICDISR_E   0x1000   // Interrupt Security Registers
#define ARM_GIC_ICDISER_E  0x1200   // Interrupt Set-Enable for ESPI
#define ARM_GIC_ICDICER_E  0x1400   // Interrupt Clear-Enable Registers
#define ARM_GIC_ICDSPR_E   0x1600   // Interrupt Set-Pending Registers
#define ARM_GIC_ICDICPR_E  0x1800   // Interrupt Clear-Pending Registers
#define ARM_GIC_ICDIPR_E   0x2000   // Interrupt Priority Registers
#define ARM_GIC_ICDICFR_E  0x3000   // Interrupt Configuration Registers

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

//
// GIC SPI and extended SPI ranges
//
#define ARM_GIC_ARCH_SPI_MIN      32
#define ARM_GIC_ARCH_SPI_MAX      1019
#define ARM_GIC_ARCH_EXT_SPI_MIN  4096
#define ARM_GIC_ARCH_EXT_SPI_MAX  5119

// GIC revision 3 specific declarations

#define ICC_SRE_EL2_SRE   (1 << 0)
#define ICC_CTLR_EOImode  (1 << 1)

#define ARM_GICD_IROUTER_IRM  BIT31

// GIC revision 5 specific declarations

#define ARM_GICV5_ARCH_SPI_MIN  0x60000000
#define ARM_GICV5_ARCH_SPI_MAX  0x60ffffff

#endif // ARMGIC_H_
