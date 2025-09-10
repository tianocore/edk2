/** @file
  RISC-V Processor supervisor mode trap handler

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include "ExceptionHandler.h"

  .align 3
  .section .entry, "ax", %progbits
  .globl SupervisorModeTrap
SupervisorModeTrap:
  addi sp, sp, -SMODE_TRAP_REGS_SIZE

  /* Save all general regisers except SP */
  sd    t0, SMODE_TRAP_REGS_OFFSET(t0)(sp)

  csrr  t0, CSR_SSTATUS
  sd    t0, SMODE_TRAP_REGS_OFFSET(sstatus)(sp)
  csrr  t0, CSR_SEPC
  sd    t0, SMODE_TRAP_REGS_OFFSET(sepc)(sp)
  csrr  t0, CSR_STVAL
  sd    t0, SMODE_TRAP_REGS_OFFSET(stval)(sp)
  ld    t0, SMODE_TRAP_REGS_OFFSET(t0)(sp)

  sd    zero, SMODE_TRAP_REGS_OFFSET(zero)(sp)
  sd    ra, SMODE_TRAP_REGS_OFFSET(ra)(sp)
  sd    gp, SMODE_TRAP_REGS_OFFSET(gp)(sp)
  sd    tp, SMODE_TRAP_REGS_OFFSET(tp)(sp)
  sd    t1, SMODE_TRAP_REGS_OFFSET(t1)(sp)
  sd    t2, SMODE_TRAP_REGS_OFFSET(t2)(sp)
  sd    s0, SMODE_TRAP_REGS_OFFSET(s0)(sp)
  sd    s1, SMODE_TRAP_REGS_OFFSET(s1)(sp)
  sd    a0, SMODE_TRAP_REGS_OFFSET(a0)(sp)
  sd    a1, SMODE_TRAP_REGS_OFFSET(a1)(sp)
  sd    a2, SMODE_TRAP_REGS_OFFSET(a2)(sp)
  sd    a3, SMODE_TRAP_REGS_OFFSET(a3)(sp)
  sd    a4, SMODE_TRAP_REGS_OFFSET(a4)(sp)
  sd    a5, SMODE_TRAP_REGS_OFFSET(a5)(sp)
  sd    a6, SMODE_TRAP_REGS_OFFSET(a6)(sp)
  sd    a7, SMODE_TRAP_REGS_OFFSET(a7)(sp)
  sd    s2, SMODE_TRAP_REGS_OFFSET(s2)(sp)
  sd    s3, SMODE_TRAP_REGS_OFFSET(s3)(sp)
  sd    s4, SMODE_TRAP_REGS_OFFSET(s4)(sp)
  sd    s5, SMODE_TRAP_REGS_OFFSET(s5)(sp)
  sd    s6, SMODE_TRAP_REGS_OFFSET(s6)(sp)
  sd    s7, SMODE_TRAP_REGS_OFFSET(s7)(sp)
  sd    s8, SMODE_TRAP_REGS_OFFSET(s8)(sp)
  sd    s9, SMODE_TRAP_REGS_OFFSET(s9)(sp)
  sd    s10, SMODE_TRAP_REGS_OFFSET(s10)(sp)
  sd    s11, SMODE_TRAP_REGS_OFFSET(s11)(sp)
  sd    t3, SMODE_TRAP_REGS_OFFSET(t3)(sp)
  sd    t4, SMODE_TRAP_REGS_OFFSET(t4)(sp)
  sd    t5, SMODE_TRAP_REGS_OFFSET(t5)(sp)
  sd    t6, SMODE_TRAP_REGS_OFFSET(t6)(sp)

  /* Call to Supervisor mode trap handler in CpuExceptionHandlerLib.c */
  mv    a0, sp
  call  RiscVSupervisorModeTrapHandler

  /* Restore all general regisers except SP */
  ld    ra, SMODE_TRAP_REGS_OFFSET(ra)(sp)
  ld    gp, SMODE_TRAP_REGS_OFFSET(gp)(sp)
  ld    tp, SMODE_TRAP_REGS_OFFSET(tp)(sp)
  ld    t2, SMODE_TRAP_REGS_OFFSET(t2)(sp)
  ld    t1, SMODE_TRAP_REGS_OFFSET(t1)(sp)
  ld    s0, SMODE_TRAP_REGS_OFFSET(s0)(sp)
  ld    s1, SMODE_TRAP_REGS_OFFSET(s1)(sp)
  ld    a0, SMODE_TRAP_REGS_OFFSET(a0)(sp)
  ld    a1, SMODE_TRAP_REGS_OFFSET(a1)(sp)
  ld    a2, SMODE_TRAP_REGS_OFFSET(a2)(sp)
  ld    a3, SMODE_TRAP_REGS_OFFSET(a3)(sp)
  ld    a4, SMODE_TRAP_REGS_OFFSET(a4)(sp)
  ld    a5, SMODE_TRAP_REGS_OFFSET(a5)(sp)
  ld    a6, SMODE_TRAP_REGS_OFFSET(a6)(sp)
  ld    a7, SMODE_TRAP_REGS_OFFSET(a7)(sp)
  ld    s2, SMODE_TRAP_REGS_OFFSET(s2)(sp)
  ld    s3, SMODE_TRAP_REGS_OFFSET(s3)(sp)
  ld    s4, SMODE_TRAP_REGS_OFFSET(s4)(sp)
  ld    s5, SMODE_TRAP_REGS_OFFSET(s5)(sp)
  ld    s6, SMODE_TRAP_REGS_OFFSET(s6)(sp)
  ld    s7, SMODE_TRAP_REGS_OFFSET(s7)(sp)
  ld    s8, SMODE_TRAP_REGS_OFFSET(s8)(sp)
  ld    s9, SMODE_TRAP_REGS_OFFSET(s9)(sp)
  ld    s10, SMODE_TRAP_REGS_OFFSET(s10)(sp)
  ld    s11, SMODE_TRAP_REGS_OFFSET(s11)(sp)
  ld    t3, SMODE_TRAP_REGS_OFFSET(t3)(sp)
  ld    t4, SMODE_TRAP_REGS_OFFSET(t4)(sp)
  ld    t5, SMODE_TRAP_REGS_OFFSET(t5)(sp)
  ld    t6, SMODE_TRAP_REGS_OFFSET(t6)(sp)

  ld    t0, SMODE_TRAP_REGS_OFFSET(sepc)(sp)
  csrw  CSR_SEPC, t0
  ld    t0, SMODE_TRAP_REGS_OFFSET(sstatus)(sp)
  csrw  CSR_SSTATUS, t0
  ld    t0, SMODE_TRAP_REGS_OFFSET(stval)(sp)
  csrw  CSR_STVAL, t0
  ld    t0, SMODE_TRAP_REGS_OFFSET(t0)(sp)
  addi  sp, sp, SMODE_TRAP_REGS_SIZE
  sret
