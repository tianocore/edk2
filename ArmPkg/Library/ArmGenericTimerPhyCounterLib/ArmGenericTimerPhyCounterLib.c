/** @file

  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/ArmLib.h>

VOID
EFIAPI
ArmGenericTimerEnableTimer (
  VOID
  )
{
  UINTN  TimerCtrlReg;

  TimerCtrlReg  = ArmReadCntpCtl ();
  TimerCtrlReg |= ARM_ARCH_TIMER_ENABLE;
  ArmWriteCntpCtl (TimerCtrlReg);
}

VOID
EFIAPI
ArmGenericTimerReenableTimer (
  VOID
  )
{
}

VOID
EFIAPI
ArmGenericTimerDisableTimer (
  VOID
  )
{
  UINTN  TimerCtrlReg;

  TimerCtrlReg  = ArmReadCntpCtl ();
  TimerCtrlReg &= ~ARM_ARCH_TIMER_ENABLE;
  ArmWriteCntpCtl (TimerCtrlReg);
}

VOID
EFIAPI
ArmGenericTimerSetTimerFreq (
  IN   UINTN  FreqInHz
  )
{
  ArmWriteCntFrq (FreqInHz);
}

UINTN
EFIAPI
ArmGenericTimerGetTimerFreq (
  VOID
  )
{
  return ArmReadCntFrq ();
}

UINTN
EFIAPI
ArmGenericTimerGetTimerVal (
  VOID
  )
{
  return ArmReadCntpTval ();
}

VOID
EFIAPI
ArmGenericTimerSetTimerVal (
  IN   UINTN  Value
  )
{
  ArmWriteCntpTval (Value);
}

UINT64
EFIAPI
ArmGenericTimerGetSystemCount (
  VOID
  )
{
  return ArmReadCntPct ();
}

UINTN
EFIAPI
ArmGenericTimerGetTimerCtrlReg (
  VOID
  )
{
  return ArmReadCntpCtl ();
}

VOID
EFIAPI
ArmGenericTimerSetTimerCtrlReg (
  UINTN  Value
  )
{
  ArmWriteCntpCtl (Value);
}

UINT64
EFIAPI
ArmGenericTimerGetCompareVal (
  VOID
  )
{
  return ArmReadCntpCval ();
}

VOID
EFIAPI
ArmGenericTimerSetCompareVal (
  IN   UINT64  Value
  )
{
  ArmWriteCntpCval (Value);
}
