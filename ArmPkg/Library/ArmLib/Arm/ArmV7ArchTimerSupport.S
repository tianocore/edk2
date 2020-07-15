#------------------------------------------------------------------------------
#
# Copyright (c) 2011, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>

ASM_FUNC(ArmReadCntFrq)
  mrc    p15, 0, r0, c14, c0, 0    @ Read CNTFRQ
  bx     lr

ASM_FUNC(ArmWriteCntFrq)
  mcr    p15, 0, r0, c14, c0, 0    @ Write to CNTFRQ
  bx     lr

ASM_FUNC(ArmReadCntPct)
  mrrc   p15, 0, r0, r1, c14       @ Read CNTPT (Physical counter register)
  bx     lr

ASM_FUNC(ArmReadCntkCtl)
  mrc    p15, 0, r0, c14, c1, 0    @ Read CNTK_CTL (Timer PL1 Control Register)
  bx     lr

ASM_FUNC(ArmWriteCntkCtl)
  mcr    p15, 0, r0, c14, c1, 0    @ Write to CNTK_CTL (Timer PL1 Control Register)
  bx     lr

ASM_FUNC(ArmReadCntpTval)
  mrc    p15, 0, r0, c14, c2, 0    @ Read CNTP_TVAL (PL1 physical timer value register)
  bx     lr

ASM_FUNC(ArmWriteCntpTval)
  mcr    p15, 0, r0, c14, c2, 0    @ Write to CNTP_TVAL (PL1 physical timer value register)
  bx     lr

ASM_FUNC(ArmReadCntpCtl)
  mrc    p15, 0, r0, c14, c2, 1    @ Read CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

ASM_FUNC(ArmWriteCntpCtl)
  mcr    p15, 0, r0, c14, c2, 1    @ Write to  CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

ASM_FUNC(ArmReadCntvTval)
  mrc    p15, 0, r0, c14, c3, 0    @ Read CNTV_TVAL (Virtual Timer Value register)
  bx     lr

ASM_FUNC(ArmWriteCntvTval)
  mcr    p15, 0, r0, c14, c3, 0    @ Write to CNTV_TVAL (Virtual Timer Value register)
  bx     lr

ASM_FUNC(ArmReadCntvCtl)
  mrc    p15, 0, r0, c14, c3, 1    @ Read CNTV_CTL (Virtual Timer Control Register)
  bx     lr

ASM_FUNC(ArmWriteCntvCtl)
  mcr    p15, 0, r0, c14, c3, 1    @ Write to CNTV_CTL (Virtual Timer Control Register)
  bx     lr

ASM_FUNC(ArmReadCntvCt)
  mrrc   p15, 1, r0, r1, c14       @ Read CNTVCT  (Virtual Count Register)
  bx     lr

ASM_FUNC(ArmReadCntpCval)
  mrrc   p15, 2, r0, r1, c14       @ Read CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

ASM_FUNC(ArmWriteCntpCval)
  mcrr   p15, 2, r0, r1, c14       @ Write to CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

ASM_FUNC(ArmReadCntvCval)
  mrrc   p15, 3, r0, r1, c14       @ Read CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

ASM_FUNC(ArmWriteCntvCval)
  mcrr   p15, 3, r0, r1, c14       @ write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

ASM_FUNC(ArmReadCntvOff)
  mrrc   p15, 4, r0, r1, c14       @ Read CNTVOFF (virtual Offset register)
  bx     lr

ASM_FUNC(ArmWriteCntvOff)
  mcrr   p15, 4, r0, r1, c14       @ Write to CNTVOFF (Virtual Offset register)
  bx     lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
