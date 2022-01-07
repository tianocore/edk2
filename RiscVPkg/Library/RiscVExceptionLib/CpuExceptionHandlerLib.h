/** @file

  RISC-V Exception Handler library definition file.

  Copyright (c) 2019-2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISCV_CPU_EXECPTION_HANDLER_LIB_H_
#define RISCV_CPU_EXECPTION_HANDLER_LIB_H_

extern void
SupervisorModeTrap (
  void
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
#define SMODE_TRAP_REGS_sie      34
#define SMODE_TRAP_REGS_last     35

#define SMODE_TRAP_REGS_OFFSET(x)  ((SMODE_TRAP_REGS_##x) * __SIZEOF_POINTER__)
#define SMODE_TRAP_REGS_SIZE  SMODE_TRAP_REGS_OFFSET(last)

#pragma pack(1)
typedef struct {
  //
  // Below are follow the format of EFI_SYSTEM_CONTEXT
  //
  RISC_V_REGS_PROTOTYPE    zero;
  RISC_V_REGS_PROTOTYPE    ra;
  RISC_V_REGS_PROTOTYPE    sp;
  RISC_V_REGS_PROTOTYPE    gp;
  RISC_V_REGS_PROTOTYPE    tp;
  RISC_V_REGS_PROTOTYPE    t0;
  RISC_V_REGS_PROTOTYPE    t1;
  RISC_V_REGS_PROTOTYPE    t2;
  RISC_V_REGS_PROTOTYPE    s0;
  RISC_V_REGS_PROTOTYPE    s1;
  RISC_V_REGS_PROTOTYPE    a0;
  RISC_V_REGS_PROTOTYPE    a1;
  RISC_V_REGS_PROTOTYPE    a2;
  RISC_V_REGS_PROTOTYPE    a3;
  RISC_V_REGS_PROTOTYPE    a4;
  RISC_V_REGS_PROTOTYPE    a5;
  RISC_V_REGS_PROTOTYPE    a6;
  RISC_V_REGS_PROTOTYPE    a7;
  RISC_V_REGS_PROTOTYPE    s2;
  RISC_V_REGS_PROTOTYPE    s3;
  RISC_V_REGS_PROTOTYPE    s4;
  RISC_V_REGS_PROTOTYPE    s5;
  RISC_V_REGS_PROTOTYPE    s6;
  RISC_V_REGS_PROTOTYPE    s7;
  RISC_V_REGS_PROTOTYPE    s8;
  RISC_V_REGS_PROTOTYPE    s9;
  RISC_V_REGS_PROTOTYPE    s10;
  RISC_V_REGS_PROTOTYPE    s11;
  RISC_V_REGS_PROTOTYPE    t3;
  RISC_V_REGS_PROTOTYPE    t4;
  RISC_V_REGS_PROTOTYPE    t5;
  RISC_V_REGS_PROTOTYPE    t6;
  //
  // Below are the additional information to
  // EFI_SYSTEM_CONTEXT, private to supervisor mode trap
  // and not public to EFI environment.
  //
  RISC_V_REGS_PROTOTYPE    sepc;
  RISC_V_REGS_PROTOTYPE    sstatus;
  RISC_V_REGS_PROTOTYPE    sie;
} SMODE_TRAP_REGISTERS;
#pragma pack()

#endif
