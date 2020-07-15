//------------------------------------------------------------------------------
//
// Copyright (c) 2011, ARM Limited. All rights reserved.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------


    INCLUDE AsmMacroExport.inc
    PRESERVE8

 RVCT_ASM_EXPORT ArmReadCntFrq
  mrc    p15, 0, r0, c14, c0, 0    ; Read CNTFRQ
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntFrq
  mcr    p15, 0, r0, c14, c0, 0    ; Write to CNTFRQ
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntPct
  mrrc   p15, 0, r0, r1, c14       ; Read CNTPT (Physical counter register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntkCtl
  mrc    p15, 0, r0, c14, c1, 0    ; Read CNTK_CTL (Timer PL1 Control Register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntkCtl
  mcr    p15, 0, r0, c14, c1, 0    ; Write to CNTK_CTL (Timer PL1 Control Register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntpTval
  mrc    p15, 0, r0, c14, c2, 0    ; Read CNTP_TVAL (PL1 physical timer value register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntpTval
  mcr    p15, 0, r0, c14, c2, 0    ; Write to CNTP_TVAL (PL1 physical timer value register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntpCtl
  mrc    p15, 0, r0, c14, c2, 1    ; Read CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntpCtl
  mcr    p15, 0, r0, c14, c2, 1    ; Write to  CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntvTval
  mrc    p15, 0, r0, c14, c3, 0    ; Read CNTV_TVAL (Virtual Timer Value register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntvTval
  mcr    p15, 0, r0, c14, c3, 0    ; Write to CNTV_TVAL (Virtual Timer Value register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntvCtl
  mrc    p15, 0, r0, c14, c3, 1    ; Read CNTV_CTL (Virtual Timer Control Register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntvCtl
  mcr    p15, 0, r0, c14, c3, 1    ; Write to CNTV_CTL (Virtual Timer Control Register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntvCt
  mrrc   p15, 1, r0, r1, c14       ; Read CNTVCT  (Virtual Count Register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntpCval
  mrrc   p15, 2, r0, r1, c14       ; Read CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntpCval
  mcrr   p15, 2, r0, r1, c14       ; Write to CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntvCval
  mrrc   p15, 3, r0, r1, c14       ; Read CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntvCval
  mcrr   p15, 3, r0, r1, c14       ; write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

 RVCT_ASM_EXPORT ArmReadCntvOff
  mrrc   p15, 4, r0, r1, c14       ; Read CNTVOFF (virtual Offset register)
  bx     lr

 RVCT_ASM_EXPORT ArmWriteCntvOff
  mcrr   p15, 4, r0, r1, c14       ; Write to CNTVOFF (Virtual Offset register)
  bx     lr

 END
