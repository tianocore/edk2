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
#include <Library/ArmArchTimer.h>

VOID
EFIAPI
ArmGenericTimerEnableTimer (
  VOID
  )
{
  UINTN TimerCtrlReg;

  ArmArchTimerReadReg (CntvCtl, (VOID *)&TimerCtrlReg);
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
  ArmArchTimerWriteReg (CntvCtl, (VOID *)&TimerCtrlReg);
}

VOID
EFIAPI
ArmGenericTimerDisableTimer (
  VOID
  )
{
  UINTN TimerCtrlReg;

  ArmArchTimerReadReg (CntvCtl, (VOID *)&TimerCtrlReg);
  TimerCtrlReg &= ~ARM_ARCH_TIMER_ENABLE;
  ArmArchTimerWriteReg (CntvCtl, (VOID *)&TimerCtrlReg);
}

VOID
EFIAPI
ArmGenericTimerSetTimerFreq (
  IN   UINTN  FreqInHz
  )
{
  ArmArchTimerWriteReg (CntFrq, (VOID *)&FreqInHz);
}

UINTN
EFIAPI
ArmGenericTimerGetTimerFreq (
  VOID
  )
{
  UINTN ArchTimerFreq = 0;
  ArmArchTimerReadReg (CntFrq, (VOID *)&ArchTimerFreq);
  return ArchTimerFreq;
}

UINTN
EFIAPI
ArmGenericTimerGetTimerVal (
  VOID
  )
{
  UINTN ArchTimerValue;
  ArmArchTimerReadReg (CntvTval, (VOID *)&ArchTimerValue);

  return ArchTimerValue;
}


VOID
EFIAPI
ArmGenericTimerSetTimerVal (
  IN   UINTN   Value
  )
{
  ArmArchTimerWriteReg (CntvTval, (VOID *)&Value);
}

UINT64
EFIAPI
ArmGenericTimerGetSystemCount (
  VOID
  )
{
  UINT64 SystemCount;
  ArmArchTimerReadReg (CntvCt, (VOID *)&SystemCount);

  return SystemCount;
}

UINTN
EFIAPI
ArmGenericTimerGetTimerCtrlReg (
  VOID
  )
{
  UINTN  Value;
  ArmArchTimerReadReg (CntvCtl, (VOID *)&Value);

  return Value;
}

VOID
EFIAPI
ArmGenericTimerSetTimerCtrlReg (
  UINTN Value
  )
{
  ArmArchTimerWriteReg (CntvCtl, (VOID *)&Value);
}

UINT64
EFIAPI
ArmGenericTimerGetCompareVal (
  VOID
  )
{
  UINT64  Value;
  ArmArchTimerReadReg (CntvCval, (VOID *)&Value);

  return Value;
}

VOID
EFIAPI
ArmGenericTimerSetCompareVal (
  IN   UINT64   Value
  )
{
  ArmArchTimerWriteReg (CntvCval, (VOID *)&Value);
}
