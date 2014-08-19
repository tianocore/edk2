//------------------------------------------------------------------------------
//
// Copyright (c) 2011, ARM Limited. All rights reserved.
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

    EXPORT  ArmReadCntFrq
    EXPORT  ArmWriteCntFrq
    EXPORT  ArmReadCntPct
    EXPORT  ArmReadCntkCtl
    EXPORT  ArmWriteCntkCtl
    EXPORT  ArmReadCntpTval
    EXPORT  ArmWriteCntpTval
    EXPORT  ArmReadCntpCtl
    EXPORT  ArmWriteCntpCtl
    EXPORT  ArmReadCntvTval
    EXPORT  ArmWriteCntvTval
    EXPORT  ArmReadCntvCtl
    EXPORT  ArmWriteCntvCtl
    EXPORT  ArmReadCntvCt
    EXPORT  ArmReadCntpCval
    EXPORT  ArmWriteCntpCval
    EXPORT  ArmReadCntvCval
    EXPORT  ArmWriteCntvCval
    EXPORT  ArmReadCntvOff
    EXPORT  ArmWriteCntvOff

    AREA    ArmV7ArchTimerSupport, CODE, READONLY
    PRESERVE8

ArmReadCntFrq
  mrc    p15, 0, r0, c14, c0, 0    ; Read CNTFRQ
  bx     lr

ArmWriteCntFrq
  mcr    p15, 0, r0, c14, c0, 0    ; Write to CNTFRQ
  bx     lr

ArmReadCntPct
  mrrc   p15, 0, r0, r1, c14       ; Read CNTPT (Physical counter register)
  bx     lr

ArmReadCntkCtl
  mrc    p15, 0, r0, c14, c1, 0    ; Read CNTK_CTL (Timer PL1 Control Register)
  bx     lr

ArmWriteCntkCtl
  mcr    p15, 0, r0, c14, c1, 0    ; Write to CNTK_CTL (Timer PL1 Control Register)
  bx     lr

ArmReadCntpTval
  mrc    p15, 0, r0, c14, c2, 0    ; Read CNTP_TVAL (PL1 physical timer value register)
  bx     lr

ArmWriteCntpTval
  mcr    p15, 0, r0, c14, c2, 0    ; Write to CNTP_TVAL (PL1 physical timer value register)
  bx     lr

ArmReadCntpCtl
  mrc    p15, 0, r0, c14, c2, 1    ; Read CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

ArmWriteCntpCtl
  mcr    p15, 0, r0, c14, c2, 1    ; Write to  CNTP_CTL (PL1 Physical Timer Control Register)
  bx     lr

ArmReadCntvTval
  mrc    p15, 0, r0, c14, c3, 0    ; Read CNTV_TVAL (Virtual Timer Value register)
  bx     lr

ArmWriteCntvTval
  mcr    p15, 0, r0, c14, c3, 0    ; Write to CNTV_TVAL (Virtual Timer Value register)
  bx     lr

ArmReadCntvCtl
  mrc    p15, 0, r0, c14, c3, 1    ; Read CNTV_CTL (Virtual Timer Control Register)
  bx     lr

ArmWriteCntvCtl
  mcr    p15, 0, r0, c14, c3, 1    ; Write to CNTV_CTL (Virtual Timer Control Register)
  bx     lr

ArmReadCntvCt
  mrrc   p15, 1, r0, r1, c14       ; Read CNTVCT  (Virtual Count Register)
  bx     lr

ArmReadCntpCval
  mrrc   p15, 2, r0, r1, c14       ; Read CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

ArmWriteCntpCval
  mcrr   p15, 2, r0, r1, c14       ; Write to CNTP_CTVAL (Physical Timer Compare Value Register)
  bx     lr

ArmReadCntvCval
  mrrc   p15, 3, r0, r1, c14       ; Read CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

ArmWriteCntvCval
  mcrr   p15, 3, r0, r1, c14       ; write to  CNTV_CTVAL (Virtual Timer Compare Value Register)
  bx     lr

ArmReadCntvOff
  mrrc   p15, 4, r0, r1, c14       ; Read CNTVOFF (virtual Offset register)
  bx     lr

ArmWriteCntvOff
  mcrr   p15, 4, r0, r1, c14       ; Write to CNTVOFF (Virtual Offset register)
  bx     lr

 END
