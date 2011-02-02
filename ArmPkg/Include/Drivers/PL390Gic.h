/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#ifndef __PL390GIC_H
#define __PL390GIC_H

//
// GIC definitions
//

// Distributor
#define GIC_ICDDCR          0x000 // Distributor Control Register
#define GIC_ICDICTR         0x004 // Interrupt Controller Type Register
#define GIC_ICDIIDR         0x008 // Implementer Identification Register

// each reg base below repeats for VE_NUM_GIC_REG_PER_INT_BITS (see GIC spec)
#define GIC_ICDISR          0x080 // Interrupt Security Registers
#define GIC_ICDISER         0x100 // Interrupt Set-Enable Registers
#define GIC_ICDICER         0x180 // Interrupt Clear-Enable Registers
#define GIC_ICDSPR          0x200 // Interrupt Set-Pending Registers
#define GIC_ICDICPR         0x280 // Interrupt Clear-Pending Registers
#define GIC_ICDABR          0x300 // Active Bit Registers

// each reg base below repeats for VE_NUM_GIC_REG_PER_INT_BYTES
#define GIC_ICDIPR          0x400 // Interrupt Priority Registers

// each reg base below repeats for VE_NUM_GIC_INTERRUPTS
#define GIC_ICDIPTR         0x800 // Interrupt Processor Target Registers
#define GIC_ICDICFR         0xC00 // Interrupt Configuration Registers

// just one of these
#define GIC_ICDSGIR         0xF00 // Software Generated Interrupt Register

// Cpu interface
#define GIC_ICCICR          0x00  // CPU Interface Control Register
#define GIC_ICCPMR          0x04  // Interrupt Priority Mask Register
#define GIC_ICCBPR          0x08  // Binary Point Register
#define GIC_ICCIAR          0x0C  // Interrupt Acknowledge Register
#define GIC_ICCEIOR         0x10  // End Of Interrupt Register
#define GIC_ICCRPR          0x14  // Running Priority Register
#define GIC_ICCPIR          0x18  // Highest Pending Interrupt Register
#define GIC_ICCABPR         0x1C  // Aliased Binary Point Register
#define GIC_ICCIDR          0xFC  // Identification Register

#define GIC_ICDSGIR_FILTER_TARGETLIST       0x0
#define GIC_ICDSGIR_FILTER_EVERYONEELSE     0x1
#define GIC_ICDSGIR_FILTER_ITSELF           0x2

//Bit-masks to configure the CPU Interface Control register
#define GIC_ICCICR_ENABLE_SECURE(a)       ((a << 0) & 0x01)
#define GIC_ICCICR_ENABLE_NS(a)           ((a << 1) & 0x02)
#define GIC_ICCICR_ACK_CTL(a)             ((a << 2) & 0x04)
#define GIC_ICCICR_SIGNAL_SECURE_TO_FIQ(a)((a << 3) & 0x08)
#define GIC_ICCICR_USE_SBPR(a)            ((a << 4) & 0x10)


//
// GIC SEC interfaces
//
VOID
EFIAPI
PL390GicSetupNonSecure (
  IN  INTN          GicDistributorBase,
  IN  INTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
PL390GicEnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
PL390GicEnableDistributor (
  IN  INTN          GicDistributorBase
  );

VOID
EFIAPI
PL390GicSendSgiTo (
  IN  INTN          GicDistributorBase,
  IN  INTN          TargetListFilter,
  IN  INTN          CPUTargetList
  );

UINT32
EFIAPI
PL390GicAcknowledgeSgiFrom (
  IN  INTN          GicInterruptInterfaceBase,
  IN  INTN          CoreId
  );

UINT32
EFIAPI
PL390GicAcknowledgeSgi2From (
  IN  INTN          GicInterruptInterfaceBase,
  IN  INTN          CoreId,
  IN  INTN          SgiId
  );

UINTN
EFIAPI
PL390GicSetPriorityMask (
  IN  INTN          GicInterruptInterfaceBase,
  IN  INTN          PriorityMask
  );

#endif
