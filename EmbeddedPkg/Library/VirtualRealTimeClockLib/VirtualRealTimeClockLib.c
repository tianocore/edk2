/** @file
 *
 *  Implement virtual EFI RealTimeClock runtime services.
 *
 *  Copyright (c) 2019, Pete Batard <pete@akeo.ie>
 *  Copyright (c) 2018, Andrei Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) 2011-2021, ARM Ltd. All rights reserved.
 *  Copyright (c) 2008-2010, Apple Inc. All rights reserved.
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 *  Based on ArmPlatformPkg/Library/PL031RealTimeClockLib/PL031RealTimeClockLib.inf
 *
 **/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/TimerLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/UefiRuntimeLib.h>

STATIC CONST CHAR16  mEpochVariableName[]    = L"RtcEpochSeconds";
STATIC CONST CHAR16  mTimeZoneVariableName[] = L"RtcTimeZone";
STATIC CONST CHAR16  mDaylightVariableName[] = L"RtcDaylight";

/**
   Returns the current time and date information, and the time-keeping capabilities
   of the virtual RTC.

   @param  Time                  A pointer to storage to receive a snapshot of the current time.
   @param  Capabilities          An optional pointer to a buffer to receive the real time clock
                                 device's capabilities.

   @retval EFI_SUCCESS           The operation completed successfully.
   @retval EFI_INVALID_PARAMETER Time is NULL.
   @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware error.

**/
EFI_STATUS
EFIAPI
LibGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities
  )
{
  EFI_STATUS  Status;
  INT16       TimeZone;
  UINT8       Daylight;
  UINT64      Freq;
  UINT64      Counter;
  UINT64      Remainder;
  UINTN       EpochSeconds;
  UINTN       Size;

  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Get the counter frequency
  Freq = GetPerformanceCounterProperties (NULL, NULL);
  if (Freq == 0) {
    return EFI_DEVICE_ERROR;
  }

  // Get the epoch time from non-volatile storage
  Size         = sizeof (UINTN);
  EpochSeconds = 0;
  Status       = EfiGetVariable (
                   (CHAR16 *)mEpochVariableName,
                   &gEfiCallerIdGuid,
                   NULL,
                   &Size,
                   (VOID *)&EpochSeconds
                   );
  // Fall back to compilation-time epoch if not set
  if (EFI_ERROR (Status)) {
    ASSERT (Status != EFI_INVALID_PARAMETER);
    ASSERT (Status != EFI_BUFFER_TOO_SMALL);

    EpochSeconds = BUILD_EPOCH;
    DEBUG ((
      DEBUG_INFO,
      "LibGetTime: %s non volatile variable was not found - Using compilation time epoch.\n",
      mEpochVariableName
      ));

    EfiSetVariable (
      (CHAR16 *)mEpochVariableName,
      &gEfiCallerIdGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      sizeof (EpochSeconds),
      &EpochSeconds
      );
  }

  Counter       = GetPerformanceCounter ();
  EpochSeconds += DivU64x64Remainder (Counter, Freq, &Remainder);

  // Get the current time zone information from non-volatile storage
  Size   = sizeof (TimeZone);
  Status = EfiGetVariable (
             (CHAR16 *)mTimeZoneVariableName,
             &gEfiCallerIdGuid,
             NULL,
             &Size,
             (VOID *)&TimeZone
             );

  if (EFI_ERROR (Status)) {
    ASSERT (Status != EFI_INVALID_PARAMETER);
    ASSERT (Status != EFI_BUFFER_TOO_SMALL);

    if (Status != EFI_NOT_FOUND) {
      return Status;
    }

    // The time zone variable does not exist in non-volatile storage, so create it.
    Time->TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    // Store it
    Status = EfiSetVariable (
               (CHAR16 *)mTimeZoneVariableName,
               &gEfiCallerIdGuid,
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
               Size,
               (VOID *)&(Time->TimeZone)
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "LibGetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
        mTimeZoneVariableName,
        Status
        ));
      return Status;
    }
  } else {
    // Got the time zone
    Time->TimeZone = TimeZone;

    // Check TimeZone bounds: -1440 to 1440 or 2047
    if (  ((Time->TimeZone < -1440) || (Time->TimeZone > 1440))
       && (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE))
    {
      Time->TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    }

    // Adjust for the correct time zone
    if (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE) {
      EpochSeconds += Time->TimeZone * SEC_PER_MIN;
    }
  }

  // Get the current daylight information from non-volatile storage
  Size   = sizeof (Daylight);
  Status = EfiGetVariable (
             (CHAR16 *)mDaylightVariableName,
             &gEfiCallerIdGuid,
             NULL,
             &Size,
             (VOID *)&Daylight
             );

  if (EFI_ERROR (Status)) {
    ASSERT (Status != EFI_INVALID_PARAMETER);
    ASSERT (Status != EFI_BUFFER_TOO_SMALL);

    if (Status != EFI_NOT_FOUND) {
      return Status;
    }

    // The daylight variable does not exist in non-volatile storage, so create it.
    Time->Daylight = 0;
    // Store it
    Status = EfiSetVariable (
               (CHAR16 *)mDaylightVariableName,
               &gEfiCallerIdGuid,
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
               Size,
               (VOID *)&(Time->Daylight)
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "LibGetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
        mDaylightVariableName,
        Status
        ));
      return Status;
    }
  } else {
    // Got the daylight information
    Time->Daylight = Daylight;

    // Adjust for the correct period
    if ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT) {
      // Convert to adjusted time, i.e. spring forwards one hour
      EpochSeconds += SEC_PER_HOUR;
    }
  }

  EpochToEfiTime (EpochSeconds, Time);

  // Because we use the performance counter, we can fill the Nanosecond attribute
  // provided that the remainder doesn't overflow 64-bit during multiplication.
  if (Remainder <= 18446744073U) {
    Time->Nanosecond = (UINT32)(MultU64x64 (Remainder, 1000000000U) / Freq);
  } else {
    DEBUG ((DEBUG_WARN, "LibGetTime: Nanosecond value not set (64-bit overflow).\n"));
  }

  if (Capabilities) {
    Capabilities->Accuracy   = 0;
    Capabilities->Resolution = 1;
    Capabilities->SetsToZero = FALSE;
  }

  return EFI_SUCCESS;
}

/**
   Sets the current local time and date information.

   @param  Time                  A pointer to the current time.

   @retval EFI_SUCCESS           The operation completed successfully.
   @retval EFI_INVALID_PARAMETER A time field is out of range.
   @retval EFI_DEVICE_ERROR      The time could not be set due to hardware error.

**/
EFI_STATUS
EFIAPI
LibSetTime (
  IN EFI_TIME  *Time
  )
{
  EFI_STATUS  Status;
  UINT64      Freq;
  UINT64      Counter;
  UINT64      Remainder;
  UINTN       EpochSeconds;

  if (!IsTimeValid (Time)) {
    return EFI_INVALID_PARAMETER;
  }

  EpochSeconds = EfiTimeToEpoch (Time);

  // Adjust for the correct time zone, i.e. convert to UTC time zone
  if (  (Time->TimeZone != EFI_UNSPECIFIED_TIMEZONE)
     && (EpochSeconds > Time->TimeZone * SEC_PER_MIN))
  {
    EpochSeconds -= Time->TimeZone * SEC_PER_MIN;
  }

  // Adjust for the correct period
  if (  ((Time->Daylight & EFI_TIME_IN_DAYLIGHT) == EFI_TIME_IN_DAYLIGHT)
     && (EpochSeconds > SEC_PER_HOUR))
  {
    // Convert to un-adjusted time, i.e. fall back one hour
    EpochSeconds -= SEC_PER_HOUR;
  }

  // Use the performance counter to subtract the number of seconds
  // since platform reset. Without this, setting time from the shell
  // and immediately reading it back would result in a forward time
  // offset, of the duration during which the platform has been up.
  Freq = GetPerformanceCounterProperties (NULL, NULL);
  if (Freq != 0) {
    Counter = GetPerformanceCounter ();
    if (EpochSeconds > DivU64x64Remainder (Counter, Freq, &Remainder)) {
      EpochSeconds -= DivU64x64Remainder (Counter, Freq, &Remainder);
    }
  }

  // Save the current time zone information into non-volatile storage
  Status = EfiSetVariable (
             (CHAR16 *)mTimeZoneVariableName,
             &gEfiCallerIdGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             sizeof (Time->TimeZone),
             (VOID *)&(Time->TimeZone)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "LibSetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
      mTimeZoneVariableName,
      Status
      ));
    return Status;
  }

  // Save the current daylight information into non-volatile storage
  Status = EfiSetVariable (
             (CHAR16 *)mDaylightVariableName,
             &gEfiCallerIdGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             sizeof (Time->Daylight),
             (VOID *)&(Time->Daylight)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "LibSetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
      mDaylightVariableName,
      Status
      ));
    return Status;
  }

  Status = EfiSetVariable (
             (CHAR16 *)mEpochVariableName,
             &gEfiCallerIdGuid,
             EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
             sizeof (EpochSeconds),
             &EpochSeconds
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "LibSetTime: Failed to save %s variable to non-volatile storage, Status = %r\n",
      mEpochVariableName,
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
   Returns the current wakeup alarm clock setting.

   @param  Enabled               Indicates if the alarm is currently enabled or disabled.
   @param  Pending               Indicates if the alarm signal is pending and requires acknowledgement.
   @param  Time                  The current alarm setting.

   @retval EFI_SUCCESS           The alarm settings were returned.
   @retval EFI_INVALID_PARAMETER Any parameter is NULL.
   @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.

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

   @param  Enabled               Enable or disable the wakeup alarm.
   @param  Time                  If Enable is TRUE, the time to set the wakeup alarm for.

   @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was enabled. If
   Enable is FALSE, then the wakeup alarm was disabled.
   @retval EFI_INVALID_PARAMETER A time field is out of range.
   @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a hardware error.
   @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
LibSetWakeupTime (
  IN  BOOLEAN   Enabled,
  OUT EFI_TIME  *Time
  )
{
  return EFI_UNSUPPORTED;
}

/**
   This is the declaration of an EFI image entry point. This can be the entry point to an application
   written to this specification, an EFI boot service driver, or an EFI runtime driver.

   @param  ImageHandle           Handle that identifies the loaded image.
   @param  SystemTable           System Table for this image.

   @retval EFI_SUCCESS           The operation completed successfully.

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
