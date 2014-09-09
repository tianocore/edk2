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

#ifndef __ARM_GENERIC_TIMER_COUNTER_LIB_H__
#define __ARM_GENERIC_TIMER_COUNTER_LIB_H__

VOID
EFIAPI
ArmGenericTimerEnableTimer (
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
  IN   UINTN   Value
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
  UINTN Value
  );

UINT64
EFIAPI
ArmGenericTimerGetCompareVal (
  VOID
  );

VOID
EFIAPI
ArmGenericTimerSetCompareVal (
  IN   UINT64   Value
  );

#endif
