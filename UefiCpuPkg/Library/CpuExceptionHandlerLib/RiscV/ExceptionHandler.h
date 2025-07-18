/** @file

  RISC-V Exception Handler library definition file.

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EXCEPTION_HANDLER_H_
#define EXCEPTION_HANDLER_H_

#include <Register/RiscV64/RiscVImpl.h>

/**
  Trap Handler for S-mode
**/
VOID
EFIAPI
SupervisorModeTrap (
  VOID
  );

//
// Index of SMode trap register
//
#define SMODE_TRAP_REGS_zero     0
#define SMODE_TRAP_REGS_ra       1
#define SMODE_TRAP_REGS_sp       2
#define SMODE_TRAP_REGS_gp       3
#define SMODE_TRAP_REGS_tp       4
#define SMODE_TRAP_REGS_t0       5
#define SMODE_TRAP_REGS_t1       6
#define SMODE_TRAP_REGS_t2       7
#define SMODE_TRAP_REGS_s0       8
#define SMODE_TRAP_REGS_s1       9
#define SMODE_TRAP_REGS_a0       10
#define SMODE_TRAP_REGS_a1       11
#define SMODE_TRAP_REGS_a2       12
#define SMODE_TRAP_REGS_a3       13
#define SMODE_TRAP_REGS_a4       14
#define SMODE_TRAP_REGS_a5       15
#define SMODE_TRAP_REGS_a6       16
#define SMODE_TRAP_REGS_a7       17
#define SMODE_TRAP_REGS_s2       18
#define SMODE_TRAP_REGS_s3       19
#define SMODE_TRAP_REGS_s4       20
#define SMODE_TRAP_REGS_s5       21
#define SMODE_TRAP_REGS_s6       22
#define SMODE_TRAP_REGS_s7       23
#define SMODE_TRAP_REGS_s8       24
#define SMODE_TRAP_REGS_s9       25
#define SMODE_TRAP_REGS_s10      26
#define SMODE_TRAP_REGS_s11      27
#define SMODE_TRAP_REGS_t3       28
#define SMODE_TRAP_REGS_t4       29
#define SMODE_TRAP_REGS_t5       30
#define SMODE_TRAP_REGS_t6       31
#define SMODE_TRAP_REGS_sepc     32
#define SMODE_TRAP_REGS_sstatus  33
#define SMODE_TRAP_REGS_stval    34
#define SMODE_TRAP_REGS_last     35

#define SMODE_TRAP_REGS_OFFSET(x)  ((SMODE_TRAP_REGS_##x) * __SIZEOF_POINTER__)
#define SMODE_TRAP_REGS_SIZE  SMODE_TRAP_REGS_OFFSET(last)

#pragma pack(1)
typedef struct {
  //
  // Below follow the format of EFI_SYSTEM_CONTEXT.
  //
  UINT64    zero;
  UINT64    ra;
  UINT64    sp;
  UINT64    gp;
  UINT64    tp;
  UINT64    t0;
  UINT64    t1;
  UINT64    t2;
  UINT64    s0;
  UINT64    s1;
  UINT64    a0;
  UINT64    a1;
  UINT64    a2;
  UINT64    a3;
  UINT64    a4;
  UINT64    a5;
  UINT64    a6;
  UINT64    a7;
  UINT64    s2;
  UINT64    s3;
  UINT64    s4;
  UINT64    s5;
  UINT64    s6;
  UINT64    s7;
  UINT64    s8;
  UINT64    s9;
  UINT64    s10;
  UINT64    s11;
  UINT64    t3;
  UINT64    t4;
  UINT64    t5;
  UINT64    t6;
  UINT64    sepc;
  UINT64    sstatus;
  UINT64    stval;
} SMODE_TRAP_REGISTERS;
#pragma pack()

#endif /* EXCEPTION_HANDLER_H_ */
