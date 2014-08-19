/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __ARM_ARCH_TIMER_H_
#define __ARM_ARCH_TIMER_H_

UINTN
EFIAPI
ArmReadCntFrq (
  VOID
  );

VOID
EFIAPI
ArmWriteCntFrq (
  UINTN   FreqInHz
  );

UINT64
EFIAPI
ArmReadCntPct (
  VOID
  );

UINTN
EFIAPI
ArmReadCntkCtl (
  VOID
  );

VOID
EFIAPI
ArmWriteCntkCtl (
  UINTN   Val
  );

UINTN
EFIAPI
ArmReadCntpTval (
  VOID
  );

VOID
EFIAPI
ArmWriteCntpTval (
  UINTN   Val
  );

UINTN
EFIAPI
ArmReadCntpCtl (
  VOID
  );

VOID
EFIAPI
ArmWriteCntpCtl (
  UINTN   Val
  );

UINTN
EFIAPI
ArmReadCntvTval (
  VOID
  );

VOID
EFIAPI
ArmWriteCntvTval (
  UINTN   Val
  );

UINTN
EFIAPI
ArmReadCntvCtl (
  VOID
  );

VOID
EFIAPI
ArmWriteCntvCtl (
  UINTN   Val
  );

UINT64
EFIAPI
ArmReadCntvCt (
  VOID
  );

UINT64
EFIAPI
ArmReadCntpCval (
  VOID
  );

VOID
EFIAPI
ArmWriteCntpCval (
  UINT64   Val
  );

UINT64
EFIAPI
ArmReadCntvCval (
  VOID
  );

VOID
EFIAPI
ArmWriteCntvCval (
  UINT64   Val
  );

UINT64
EFIAPI
ArmReadCntvOff (
  VOID
  );

VOID
EFIAPI
ArmWriteCntvOff (
  UINT64   Val
  );

#endif // __ARM_ARCH_TIMER_H_

