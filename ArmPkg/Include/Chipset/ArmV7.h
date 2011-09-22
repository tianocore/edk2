/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_V7_H__
#define __ARM_V7_H__

#include <Chipset/ArmV7Mmu.h>

// Domain Access Control Register
#define DOMAIN_ACCESS_CONTROL_MASK(a)     (3UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_NONE(a)     (0UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_CLIENT(a)   (1UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_RESERVED(a) (2UL << (2 * (a)))
#define DOMAIN_ACCESS_CONTROL_MANAGER(a)  (3UL << (2 * (a)))

// Cortex A9 feature bit definitions
#define A9_FEATURE_PARITY  (1<<9)
#define A9_FEATURE_AOW     (1<<8)
#define A9_FEATURE_EXCL    (1<<7)
#define A9_FEATURE_SMP     (1<<6)
#define A9_FEATURE_FOZ     (1<<3)
#define A9_FEATURE_DPREF   (1<<2)
#define A9_FEATURE_HINT    (1<<1)
#define A9_FEATURE_FWD     (1<<0)

// SCU register offsets & masks
#define SCU_CONTROL_OFFSET       0x0
#define SCU_CONFIG_OFFSET        0x4
#define SCU_INVALL_OFFSET        0xC
#define SCU_FILT_START_OFFSET    0x40
#define SCU_FILT_END_OFFSET      0x44
#define SCU_SACR_OFFSET          0x50
#define SCU_SSACR_OFFSET         0x54

#define SMP_GIC_CPUIF_BASE       0x100
#define SMP_GIC_DIST_BASE        0x1000

// CPACR - Coprocessor Access Control Register definitions
#define CPACR_CP_DENIED(cp)     0x00
#define CPACR_CP_PRIV(cp)       ((0x1 << ((cp) << 1)) & 0x0FFFFFFF)
#define CPACR_CP_FULL(cp)       ((0x3 << ((cp) << 1)) & 0x0FFFFFFF)
#define CPACR_ASEDIS            (1 << 31)
#define CPACR_D32DIS            (1 << 30)
#define CPACR_CP_FULL_ACCESS    0x0FFFFFFF

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

VOID
EFIAPI
ArmEnableSWPInstruction (
  VOID
  );

VOID
EFIAPI
ArmWriteNsacr (
  IN  UINT32   SetWayFormat
  );

VOID
EFIAPI
ArmWriteScr (
  IN  UINT32   SetWayFormat
  );

VOID
EFIAPI
ArmWriteVMBar (
  IN  UINT32   SetWayFormat
  );

VOID
EFIAPI
ArmWriteVBar (
  IN  UINT32   SetWayFormat
  );

UINT32
EFIAPI
ArmReadVBar (
  VOID
  );

VOID
EFIAPI
ArmWriteCPACR (
  IN  UINT32   SetWayFormat
  );

VOID
EFIAPI
ArmEnableVFP (
  VOID
  );

VOID
EFIAPI
ArmCallWFI (
  VOID
  );

VOID
EFIAPI
ArmInvalidScu (
  VOID
  );

UINTN
EFIAPI
ArmGetScuBaseAddress (
  VOID
  );

UINT32
EFIAPI
ArmIsScuEnable (
  VOID
  );

VOID
EFIAPI
ArmWriteAuxCr (
  IN  UINT32    Bit
  );

UINT32
EFIAPI
ArmReadAuxCr (
  VOID
  );

VOID
EFIAPI
ArmSetAuxCrBit (
  IN  UINT32    Bits
  );

VOID
EFIAPI
ArmSetupSmpNonSecure (
  IN  UINTN                     CoreId
  );

UINTN 
EFIAPI
ArmReadCbar (
  VOID
  );

VOID
EFIAPI
ArmInvalidateInstructionAndDataTlb (
  VOID
  );


UINTN
EFIAPI
ArmReadMpidr (
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

#endif // __ARM_V7_H__
