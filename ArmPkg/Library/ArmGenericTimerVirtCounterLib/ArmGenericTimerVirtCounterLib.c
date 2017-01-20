/** @file

  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/ArmGenericTimerCounterLib.h>
#include <Library/ArmLib.h>

VOID
EFIAPI
ArmGenericTimerEnableTimer (
  VOID
  )
{
  UINTN TimerCtrlReg;

  TimerCtrlReg = ArmReadCntvCtl ();
  TimerCtrlReg |= ARM_ARCH_TIMER_ENABLE;

  //
  // When running under KVM, we need to unmask the interrupt on the timer side
  // as KVM will mask it when servicing the interrupt at the hypervisor level
  // and delivering the virtual timer interrupt to the guest. Otherwise, the
  // interrupt will fire again, trapping into the hypervisor again, etc. etc.
  // This is scheduled to be fixed on the KVM side, but there is no harm in
  // leaving this in once KVM gets fixed.
  //
  TimerCtrlReg &= ~ARM_ARCH_TIMER_IMASK;
  ArmWriteCntvCtl (TimerCtrlReg);
}

VOID
EFIAPI
ArmGenericTimerDisableTimer (
  VOID
  )
{
  UINTN TimerCtrlReg;

  TimerCtrlReg = ArmReadCntvCtl ();
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
  IN   UINTN   Value
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
  UINTN Value
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
  IN   UINT64   Value
  )
{
  ArmWriteCntvCval (Value);
}
