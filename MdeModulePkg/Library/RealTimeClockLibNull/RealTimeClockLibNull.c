/** @file
  Null implementation of RealTimeClockLib.

  This library provides stub implementations of the RealTimeClock
  library class functions. Platform code that does not require
  real-time clock hardware can link against this instance to
  satisfy the library class dependency.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/RealTimeClockLib.h>

/**
  Returns the current time and date information, and the time-keeping
  capabilities of the hardware platform.

  @param[out]  Time          A pointer to storage to receive a snapshot
                             of the current time.
  @param[out]  Capabilities  An optional pointer to a buffer to receive
                             the real time clock device's capabilities.

  @retval EFI_UNSUPPORTED  This function is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets the current local time and date information.

  @param[in]  Time  A pointer to the current time.

  @retval EFI_UNSUPPORTED  This function is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibSetTime (
  IN EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Returns the current wakeup alarm clock setting.

  @param[out]  Enabled  Indicates if the alarm is currently enabled or
                        disabled.
  @param[out]  Pending  Indicates if the alarm signal is pending and
                        requires acknowledgement.
  @param[out]  Time     The current alarm setting.

  @retval EFI_UNSUPPORTED  This function is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibGetWakeupTime (
  OUT BOOLEAN   *Enabled,
  OUT BOOLEAN   *Pending,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sets the system wakeup alarm clock time.

  @param[in]   Enabled  Enable or disable the wakeup alarm.
  @param[out]  Time     If Enable is TRUE, the time to set the wakeup
                        alarm for.

  @retval EFI_UNSUPPORTED  This function is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN BOOLEAN    Enabled,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  used to initialize the RealTimeClock library.

  @param[in]  ImageHandle  Handle that identifies the loaded image.
  @param[in]  SystemTable  System Table for this image.

  @retval EFI_SUCCESS  The operation completed successfully.

**/
EFI_STATUS
EFIAPI
LibRtcInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
