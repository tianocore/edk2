/** @file

  Copyright (c) 2012 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_CORTEX_A5X_H_
#define ARM_CORTEX_A5X_H_

//
// Cortex A5x feature bit definitions
//
#define A5X_FEATURE_SMP  (1 << 6)

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
  IN  UINT64  Val
  );

VOID
EFIAPI
ArmSetCpuExCrBit (
  IN  UINT64  Bits
  );

VOID
EFIAPI
ArmUnsetCpuExCrBit (
  IN  UINT64  Bits
  );

#endif // ARM_CORTEX_A5X_H_
