/** @file
*
*  Copyright (c) 2024, Canonical Services Ltd
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/BaseRiscVFpuLib.h>
#include <Register/RiscV64/RiscVImpl.h>

//
// Initialize floating point unit
//
ASM_FUNC (RiscVInitializeFpu)
  csrr  a0, CSR_SSTATUS
  li    a1, MSTATUS_FS
  or    a0, a0, a1
  csrw  CSR_SSTATUS, a0
  csrw  CSR_FCSR, x0
  li    a0, 0
  ret
