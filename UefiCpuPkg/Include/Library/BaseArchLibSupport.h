/** @file

  Base Architecture Libraries Support.

  Copyright 2024 Google LLC

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef BASE_ARCH_LIB_SUPPORT_H
#define BASE_ARCH_LIB_SUPPORT_H

#include <Base.h>

/**
  Report the Physical Address size supported by Processor.

  For X86, returns the actual Physical Address size reported by CPUID.
  For AArch64, returns the actual Physical Address range reported by
    ID_ARCH64MMFR0_EL1.

  @return                         Physical Address Bits

**/
UINT8
ArchGetPhysicalAddressBits (
  VOID
  );

#endif
