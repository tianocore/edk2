/** @file
  Clock part of PlatformTpmLib to use TpmLib.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/PlatformTpmLib.h>

/**
  _plat__TimerReset()

  This function sets current system clock time as t0 for counting TPM time.
  This function is called at a power on event to reset the clock. When the clock
  is reset, the indication that the clock was stopped is also set.

**/
VOID
EFIAPI
PlatformTpmLibTimerReset (
  VOID
  )
{
  return;
}

/**
  _plat__TimerRestart()

  This function should be called in order to simulate the restart of the timer
  should it be stopped while power is still applied.

**/
VOID
EFIAPI
PlatformTpmLibTimerRestart (
  VOID
  )
{
  return;
}

/**
  _plat__RealTime()

  This is another, probably futile, attempt to define a portable function
  that will return a 64-bit clock value that has mSec resolution.

  @return   value   realtime

**/
UINT64
EFIAPI
PlatformTpmLibRealTime (
  VOID
  )
{
  return 0;
}

/**
  _plat__TimerRead()

  This function provides access to the tick timer of the platform. The TPM code
  uses this value to drive the TPM Clock.

  The tick timer is supposed to run when power is applied to the device. This timer
  should not be reset by time events including _TPM_Init. It should only be reset
  when TPM power is re-applied.

  If the TPM is run in a protected environment, that environment may provide the
  tick time to the TPM as long as the time provided by the environment is not
  allowed to go backwards. If the time provided by the system can go backwards
  during a power discontinuity, then the _plat__Signal_PowerOn should call
  _plat__TimerReset().

  @return value timeval

**/
UINT64
EFIAPI
PlatformTpmLibTimerRead (
  VOID
  )
{
  return 0;
}

/**
  _plat__TimerWasReset()

  This function is used to interrogate the flag indicating if the tick timer has
  been reset.

  If the resetFlag parameter is SET, then the flag will be CLEAR before the
  function returns.

  @return 0         Timer wasn't reset.
  @return others    Timer was reset.

**/
INT32
EFIAPI
PlatformTpmLibTimerWasReset (
  VOID
  )
{
  return 0;
}

/**
  _plat__TimerWasStopped()

  This function is used to interrogate the flag indicating if the tick timer has
  been stopped. If so, this is typically a reason to roll the nonce.

  This function will CLEAR the s_timerStopped flag before returning. This provides
  functionality that is similar to status register that is cleared when read. This
  is the model used here because it is the one that has the most impact on the TPM
  code as the flag can only be accessed by one entity in the TPM. Any other
  implementation of the hardware can be made to look like a read-once register.

  @return TRUE    timer was stopped
  @return FALSE   timer wasn't stopped

**/
BOOLEAN
EFIAPI
PlatformTpmLibTimerWasStopped (
  VOID
  )
{
  return FALSE;
}

/**
  _plat__ClockAdjustRate()

  ClockRateAdjust uses predefined signal values and encapsulates the platform
  specifics regarding the number of ticks the underlying clock is running at.

  The adjustment must be one of these values. A COARSE adjustment is 1%, MEDIUM
  is 0.1%, and FINE is the smallest amount supported by the platform.  The
  total (cumulative) adjustment is limited to ~15% total.  Attempts to adjust
  the clock further are silently ignored as are any invalid values.  These
  values are defined here to insulate them from spec changes and to avoid
  needing visibility to the doc-generated structure headers.

  @param [in] Adjust  The adjust number. It could be positive

**/
VOID
EFIAPI
PlatformTpmLibClockAdjustRate (
  IN INT32  Adjust
  )
{
  return;
}
