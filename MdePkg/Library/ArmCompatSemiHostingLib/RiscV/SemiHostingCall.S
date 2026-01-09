/** @file
*
*  Copyright (c) 2025, Ventana Micro Systems Inc. All Rights Reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Base.h>
#include <Register/RiscV64/RiscVImpl.h>

.text
.align 4

//
// Semi hosting calling
//
// @param a0: Operation number.
// @param a1: System block address.
//
// @retval a0 : return value.
ASM_FUNC (SemiHostingCall)
  .option push
  .option norvc

  slli zero, zero, 0x1f
  ebreak
  srai zero, zero, 7

  .option pop
ret

//
// Check if semi hosting connection enabled
//
// @retval a0 : 0 if not enabled, otherwise enabled.
ASM_FUNC (
  SemiHostingConnectionEnabled
  )
  .option push
  .option norvc

j _TestSemihostStrap

_SemiHostingStrap:
  csrr  t1, sepc
  addi t1, t1, 4
  csrw sepc, t1
  // strap happens when semi hosting not supported
  add t2, zero, zero
  sret

_TestSemihostStrap:
  // use t2 for return value
  li  t2, 1
  lla t0, _SemiHostingStrap
  csrrw t0, stvec, t0
  // trying semi hosting call: a0=SYS_ERRNO(0x13), a1=0
  li a0, 0x13
  li a1, 0
  slli zero, zero, 0x1f
  ebreak
  srai zero, zero, 7
  csrw stvec, t0
  addi a0, t2, 0
  .option pop
  ret
