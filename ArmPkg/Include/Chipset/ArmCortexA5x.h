/** @file

  Copyright (c) 2012-2014, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
