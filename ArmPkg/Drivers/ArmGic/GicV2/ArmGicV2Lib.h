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

#ifndef _ARM_GIC_V2_H_
#define _ARM_GIC_V2_H_

//
// GIC definitions
//

//
// GIC Distributor
//
#define ARM_GIC_ICDDCR          0x000 // Distributor Control Register
#define ARM_GIC_ICDICTR         0x004 // Interrupt Controller Type Register
#define ARM_GIC_ICDIIDR         0x008 // Implementer Identification Register

// Each reg base below repeats for VE_NUM_ARM_GIC_REG_PER_INT_BITS (see GIC spec)
#define ARM_GIC_ICDISR          0x080 // Interrupt Security Registers
#define ARM_GIC_ICDISER         0x100 // Interrupt Set-Enable Registers
#define ARM_GIC_ICDICER         0x180 // Interrupt Clear-Enable Registers
#define ARM_GIC_ICDSPR          0x200 // Interrupt Set-Pending Registers
#define ARM_GIC_ICDICPR         0x280 // Interrupt Clear-Pending Registers
#define ARM_GIC_ICDABR          0x300 // Active Bit Registers

// Each reg base below repeats for VE_NUM_ARM_GIC_REG_PER_INT_BYTES
#define ARM_GIC_ICDIPR          0x400 // Interrupt Priority Registers

// Each reg base below repeats for VE_NUM_ARM_GIC_INTERRUPTS
#define ARM_GIC_ICDIPTR         0x800 // Interrupt Processor Target Registers
#define ARM_GIC_ICDICFR         0xC00 // Interrupt Configuration Registers

#define ARM_GIC_ICDPPISR        0xD00 // PPI Status register

// just one of these
#define ARM_GIC_ICDSGIR         0xF00 // Software Generated Interrupt Register

//
// GIC Cpu interface
//
#define ARM_GIC_ICCICR          0x00  // CPU Interface Control Register
#define ARM_GIC_ICCPMR          0x04  // Interrupt Priority Mask Register
#define ARM_GIC_ICCBPR          0x08  // Binary Point Register
#define ARM_GIC_ICCIAR          0x0C  // Interrupt Acknowledge Register
#define ARM_GIC_ICCEIOR         0x10  // End Of Interrupt Register
#define ARM_GIC_ICCRPR          0x14  // Running Priority Register
#define ARM_GIC_ICCPIR          0x18  // Highest Pending Interrupt Register
#define ARM_GIC_ICCABPR         0x1C  // Aliased Binary Point Register
#define ARM_GIC_ICCIIDR         0xFC  // Identification Register

// Bit Mask for
#define ARM_GIC_ICCIAR_ACKINTID                 0x3FF

// Interrupts from 1020 to 1023 are considered as special interrupts (eg: spurious interrupts)
#define ARM_GIC_IS_SPECIAL_INTERRUPTS(Interrupt) (((Interrupt) >= 1020) && ((Interrupt) <= 1023))

VOID
EFIAPI
ArmGicV2SetupNonSecure (
  IN  UINTN         MpId,
  IN  INTN          GicDistributorBase,
  IN  INTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2DisableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  );

UINTN
EFIAPI
ArmGicV2AcknowledgeInterrupt (
  IN  UINTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EndOfInterrupt (
  IN UINTN                  GicInterruptInterfaceBase,
  IN UINTN                  Source
  );

#endif
