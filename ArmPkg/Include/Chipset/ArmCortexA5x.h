/** @file

  Copyright (c) 2012-2014, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ARM_CORTEX_A5x_H__
#define __ARM_CORTEX_A5x_H__

//
// Cortex A5x feature bit definitions
//
#define A5X_FEATURE_SMP     (1 << 6)

//
// Helper functions to access CPU Extended Control Register
//
UINT64
EFIAPI
ArmReadCpuExCr (
  VOID
  );

VOID
EFIAPI
ArmWriteCpuExCr (
  IN  UINT64 Val
  );

VOID
EFIAPI
ArmSetCpuExCrBit (
  IN  UINT64    Bits
  );

VOID
EFIAPI
ArmUnsetCpuExCrBit (
  IN  UINT64    Bits
  );

#endif
