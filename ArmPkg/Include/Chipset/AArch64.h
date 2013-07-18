/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __AARCH64_H__
#define __AARCH64_H__

#include <Chipset/AArch64Mmu.h>
#include <Chipset/ArmArchTimer.h>

// ARM Interrupt ID in Exception Table
#define ARM_ARCH_EXCEPTION_IRQ            EXCEPT_AARCH64_IRQ

// CPACR - Coprocessor Access Control Register definitions
#define CPACR_TTA_EN            (1UL << 28)
#define CPACR_FPEN_EL1          (1UL << 20)
#define CPACR_FPEN_FULL         (3UL << 20)
#define CPACR_CP_FULL_ACCESS    0x300000

// Coprocessor Trap Register (CPTR)
#define AARCH64_CPTR_TFP        (1 << 10)

// ID_AA64PFR0 - AArch64 Processor Feature Register 0 definitions
#define AARCH64_PFR0_FP         (0xF << 16)

// NSACR - Non-Secure Access Control Register definitions
#define NSACR_CP(cp)            ((1 << (cp)) & 0x3FFF)
#define NSACR_NSD32DIS          (1 << 14)
#define NSACR_NSASEDIS          (1 << 15)
#define NSACR_PLE               (1 << 16)
#define NSACR_TL                (1 << 17)
#define NSACR_NS_SMP            (1 << 18)
#define NSACR_RFR               (1 << 19)

// SCR - Secure Configuration Register definitions
#define SCR_NS                  (1 << 0)
#define SCR_IRQ                 (1 << 1)
#define SCR_FIQ                 (1 << 2)
#define SCR_EA                  (1 << 3)
#define SCR_FW                  (1 << 4)
#define SCR_AW                  (1 << 5)

// MIDR - Main ID Register definitions
#define ARM_CPU_TYPE_MASK       0xFFF
#define ARM_CPU_TYPE_AEMv8      0xD0F
#define ARM_CPU_TYPE_A15        0xC0F
#define ARM_CPU_TYPE_A9         0xC09
#define ARM_CPU_TYPE_A5         0xC05

// Hypervisor Configuration Register
#define ARM_HCR_FMO				BIT3
#define ARM_HCR_IMO				BIT4
#define ARM_HCR_AMO				BIT5
#define ARM_HCR_TGE				BIT27

// AArch64 Exception Level
#define AARCH64_EL3       0xC
#define AARCH64_EL2       0x8
#define AARCH64_EL1       0x4

#define ARM_VECTOR_TABLE_ALIGNMENT ((1 << 11)-1)

VOID
EFIAPI
ArmEnableSWPInstruction (
  VOID
  );

UINTN
EFIAPI
ArmReadCbar (
  VOID
  );

UINTN
EFIAPI
ArmReadTpidrurw (
  VOID
  );

VOID
EFIAPI
ArmWriteTpidrurw (
  UINTN Value
  );

UINTN
EFIAPI
ArmIsArchTimerImplemented (
  VOID
  );

UINTN
EFIAPI
ArmReadIdPfr0 (
  VOID
  );

UINTN
EFIAPI
ArmReadIdPfr1 (
  VOID
  );

UINTN
EFIAPI
ArmGetTCR (
  VOID
  );

VOID
EFIAPI
ArmSetTCR (
  UINTN Value
  );

UINTN
EFIAPI
ArmGetMAIR (
  VOID
  );

VOID
EFIAPI
ArmSetMAIR (
  UINTN Value
  );

VOID
EFIAPI
ArmDisableAlignmentCheck (
  VOID
  );


VOID
EFIAPI
ArmEnableAlignmentCheck (
  VOID
  );

VOID
EFIAPI
ArmDisableAllExceptions (
  VOID
  );

VOID
ArmWriteHcr (
  IN UINTN Hcr
  );

UINTN
ArmReadCurrentEL (
  VOID
  );

UINT64
PageAttributeToGcdAttribute (
  IN UINT64 PageAttributes
  );

UINT64
GcdAttributeToPageAttribute (
  IN UINT64 GcdAttributes
  );

#endif // __AARCH64_H__
