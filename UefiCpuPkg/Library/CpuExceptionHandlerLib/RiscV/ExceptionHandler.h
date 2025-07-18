/** @file

  RISC-V Exception Handler library definition file.

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ExceptionHandler_h_
#define ExceptionHandler_h_

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
  UINTN    zero;
  UINTN    ra;
  UINTN    sp;
  UINTN    gp;
  UINTN    tp;
  UINTN    t0;
  UINTN    t1;
  UINTN    t2;
  UINTN    s0;
  UINTN    s1;
  UINTN    a0;
  UINTN    a1;
  UINTN    a2;
  UINTN    a3;
  UINTN    a4;
  UINTN    a5;
  UINTN    a6;
  UINTN    a7;
  UINTN    s2;
  UINTN    s3;
  UINTN    s4;
  UINTN    s5;
  UINTN    s6;
  UINTN    s7;
  UINTN    s8;
  UINTN    s9;
  UINTN    s10;
  UINTN    s11;
  UINTN    t3;
  UINTN    t4;
  UINTN    t5;
  UINTN    t6;
  UINTN    sepc;
  UINTN    sstatus;
  UINTN    stval;
} SMODE_TRAP_REGISTERS;
#pragma pack()

#endif /* ExceptionHandler_h_ */
