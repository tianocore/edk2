#------------------------------------------------------------------------------
#
# Copyright (c) 2011, ARM Limited. All rights reserved.
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

.text
.align 2

GCC_ASM_EXPORT (ArmReadCntFrq)
GCC_ASM_EXPORT (ArmWriteCntFrq)
GCC_ASM_EXPORT (ArmReadCntPct)
GCC_ASM_EXPORT (ArmReadCntkCtl)
GCC_ASM_EXPORT (ArmWriteCntkCtl)
GCC_ASM_EXPORT (ArmReadCntpTval)
GCC_ASM_EXPORT (ArmWriteCntpTval)
GCC_ASM_EXPORT (ArmReadCntpCtl)
GCC_ASM_EXPORT (ArmWriteCntpCtl)
GCC_ASM_EXPORT (ArmReadCntvTval)
GCC_ASM_EXPORT (ArmWriteCntvTval)
GCC_ASM_EXPORT (ArmReadCntvCtl)
GCC_ASM_EXPORT (ArmWriteCntvCtl)
GCC_ASM_EXPORT (ArmReadCntvCt)
GCC_ASM_EXPORT (ArmReadCntpCval)
GCC_ASM_EXPORT (ArmWriteCntpCval)
GCC_ASM_EXPORT (ArmReadCntvCval)
GCC_ASM_EXPORT (ArmWriteCntvCval)
GCC_ASM_EXPORT (ArmReadCntvOff)
GCC_ASM_EXPORT (ArmWriteCntvOff)

ASM_PFX(ArmReadCntFrq):
  mrc    p15, 0, r0, c14, c0, 0    @ Read CNTFRQ
  bx     lr

ASM_PFX(ArmWriteCntFrq):
  mcr    p15, 0, r0, c14, c0, 0    @ Write to CNTFRQ
  bx     lr

ASM_PFX(ArmReadCntPct):
  mrrc   p15, 0, r0, r1, c14       @ Read CNTPT (Physical counter register)
  bx     lr

ASM_PFX(ArmReadCntkCtl):
  mrc    p15, 0, r0, c14, c1, 0    @ Read CNTK_CTL (Timer PL1 Control Register)
  bx     lr

ASM_PFX(ArmWriteCntkCtl):
  mcr    p15, 0, r0, c14, c1, 0    @ Write to CNTK_CTL (Timer PL1 Control Register)
  bx     lr

ASM_PFX(ArmReadCntpTval):
  mrc    p15, 0, r0, c14, c2, 0    @ Read CNTP_TVAL (PL1 physical timer value register)
  bx     lr

ASM_PFX(ArmWriteCntpTval):
  mcr    p15, 0, r0, c14, c2, 0    @ Write to CNTP_TVAL (PL1 physical timer value register)
  bx     lr

ASM_PFX(ArmReadCntpCtl):
  mrc    p15, 0, r0, c14, c2, 1    @ Read CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

ASM_PFX(ArmWriteCntpCtl):
  mcr    p15, 0, r0, c14, c2, 1    @ Write to  CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

ASM_PFX(ArmReadCntvTval):
  mrc    p15, 0, r0, c14, c3, 0    @ Read CNTV_TVAL (Virtual Timer Value register)
  bx     lr

ASM_PFX(ArmWriteCntvTval):
  mcr    p15, 0, r0, c14, c3, 0    @ Write to CNTV_TVAL (Virtual Timer Value register)
  bx     lr

ASM_PFX(ArmReadCntvCtl):
  mrc    p15, 0, r0, c14, c3, 1    @ Read CNTV_CTL (Virtual Timer Control Register)
  bx     lr

ASM_PFX(ArmWriteCntvCtl):
  mcr    p15, 0, r0, c14, c3, 1    @ Write to CNTV_CTL (Virtual Timer Control Register)
  bx     lr

ASM_PFX(ArmReadCntvCt):
  mrrc   p15, 1, r0, r1, c14       @ Read CNTVCT  (Virtual Count Register)
  bx     lr

ASM_PFX(ArmReadCntpCval):
  mrrc   p15, 2, r0, r1, c14       @ Read CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

ASM_PFX(ArmWriteCntpCval):
  mcrr   p15, 2, r0, r1, c14       @ Write to CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

ASM_PFX(ArmReadCntvCval):
  mrrc   p15, 3, r0, r1, c14       @ Read CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

ASM_PFX(ArmWriteCntvCval):
  mcrr   p15, 3, r0, r1, c14       @ write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

ASM_PFX(ArmReadCntvOff):
  mrrc   p15, 4, r0, r1, c14       @ Read CNTVOFF (virtual Offset register)
  bx     lr

ASM_PFX(ArmWriteCntvOff):
  mcrr   p15, 4, r0, r1, c14       @ Write to CNTVOFF (Virtual Offset register)
  bx     lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
