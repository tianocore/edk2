/** @file

  Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_ARCH_TIMER_H__
#define __ARM_ARCH_TIMER_H__

#define ARM_ARCH_TIMER_ENABLE           (1 << 0)
#define ARM_ARCH_TIMER_IMASK            (1 << 1)
#define ARM_ARCH_TIMER_ISTATUS          (1 << 2)

typedef enum {
  CntFrq = 0,
  CntPct,
  CntkCtl,
  CntpTval,
  CntpCtl,
  CntvTval,
  CntvCtl,
  CntvCt,
  CntpCval,
  CntvCval,
  CntvOff,
  CnthCtl,
  CnthpTval,
  CnthpCtl,
  CnthpCval,
  RegMaximum
} ARM_ARCH_TIMER_REGS;

VOID
EFIAPI
ArmArchTimerReadReg (
  IN   ARM_ARCH_TIMER_REGS   Reg,
  OUT  VOID                  *DstBuf
  );

VOID
EFIAPI
ArmArchTimerWriteReg (
  IN   ARM_ARCH_TIMER_REGS   Reg,
  IN   VOID                  *SrcBuf
  );

#endif // __ARM_ARCH_TIMER_H__
