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

  TimerCtrlReg  = ArmReadCntvCtl ();
  TimerCtrlReg |= ARM_ARCH_TIMER_ENABLE;
  ArmWriteCntvCtl (TimerCtrlReg);
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

  TimerCtrlReg  = ArmReadCntvCtl ();
  TimerCtrlReg &= ~ARM_ARCH_TIMER_ENABLE;
  ArmWriteCntvCtl (TimerCtrlReg);
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
  return ArmReadCntvTval ();
}

VOID
EFIAPI
ArmGenericTimerSetTimerVal (
  IN   UINTN  Value
  )
{
  ArmWriteCntvTval (Value);
}

UINT64
EFIAPI
ArmGenericTimerGetSystemCount (
  VOID
  )
{
  return ArmReadCntvCt ();
}

UINTN
EFIAPI
ArmGenericTimerGetTimerCtrlReg (
  VOID
  )
{
  return ArmReadCntvCtl ();
}

VOID
EFIAPI
ArmGenericTimerSetTimerCtrlReg (
  UINTN  Value
  )
{
  ArmWriteCntvCtl (Value);
}

UINT64
EFIAPI
ArmGenericTimerGetCompareVal (
  VOID
  )
{
  return ArmReadCntvCval ();
}

VOID
EFIAPI
ArmGenericTimerSetCompareVal (
  IN   UINT64  Value
  )
{
  ArmWriteCntvCval (Value);
}
