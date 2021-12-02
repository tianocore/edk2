/** @file

  Copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_GENERIC_TIMER_COUNTER_LIB_H_
#define ARM_GENERIC_TIMER_COUNTER_LIB_H_

VOID
EFIAPI
ArmGenericTimerEnableTimer (
  VOID
  );

VOID
EFIAPI
ArmGenericTimerReenableTimer (
  VOID
  );

VOID
EFIAPI
ArmGenericTimerDisableTimer (
  VOID
  );

VOID
EFIAPI
ArmGenericTimerSetTimerFreq (
  IN   UINTN  FreqInHz
  );

UINTN
EFIAPI
ArmGenericTimerGetTimerFreq (
  VOID
  );

VOID
EFIAPI
ArmGenericTimerSetTimerVal (
  IN   UINTN  Value
  );

UINTN
EFIAPI
ArmGenericTimerGetTimerVal (
  VOID
  );

UINT64
EFIAPI
ArmGenericTimerGetSystemCount (
  VOID
  );

UINTN
EFIAPI
ArmGenericTimerGetTimerCtrlReg (
  VOID
  );

VOID
EFIAPI
ArmGenericTimerSetTimerCtrlReg (
  UINTN  Value
  );

UINT64
EFIAPI
ArmGenericTimerGetCompareVal (
  VOID
  );

VOID
EFIAPI
ArmGenericTimerSetCompareVal (
  IN   UINT64  Value
  );

#endif // ARM_GENERIC_TIMER_COUNTER_LIB_H_
