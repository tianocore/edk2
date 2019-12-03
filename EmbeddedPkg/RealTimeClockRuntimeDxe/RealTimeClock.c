/** @file
  Implement EFI RealTimeClock runtime services via RTC Lib.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/RealTimeClockLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/RealTimeClock.h>

EFI_HANDLE  mHandle = NULL;

//
// These values can be set by SetTime () and need to be returned by GetTime ()
// but cannot usually be kept by the RTC hardware, so we store them in a UEFI
// variable instead.
//
typedef struct {
  INT16           TimeZone;
  UINT8           Daylight;
} NON_VOLATILE_TIME_SETTINGS;

STATIC CONST CHAR16 mTimeSettingsVariableName[] = L"RtcTimeSettings";
STATIC NON_VOLATILE_TIME_SETTINGS mTimeSettings;

STATIC
BOOLEAN
IsValidTimeZone (
  IN  INT16  TimeZone
  )
{
  return TimeZone == EFI_UNSPECIFIED_TIMEZONE ||
         (TimeZone >= -1440 && TimeZone <= 1440);
}

STATIC
BOOLEAN
IsValidDaylight (
  IN  INT8  Daylight
  )
{
  return Daylight == 0 ||
         Daylight == EFI_TIME_ADJUST_DAYLIGHT ||
         Daylight == (EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT);
}

STATIC
BOOLEAN
EFIAPI
IsLeapYear (
  IN EFI_TIME   *Time
  )
{
  if (Time->Year % 4 == 0) {
    if (Time->Year % 100 == 0) {
      if (Time->Year % 400 == 0) {
        return TRUE;
      } else {
        return FALSE;
      }
    } else {
      return TRUE;
    }
  } else {
    return FALSE;
  }
}

STATIC CONST INTN mDayOfMonth[12] = {
  31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

STATIC
BOOLEAN
EFIAPI
IsDayValid (
  IN  EFI_TIME  *Time
  )
{
  ASSERT (Time->Day >= 1);
  ASSERT (Time->Day <= mDayOfMonth[Time->Month - 1]);
  ASSERT (Time->Month != 2 || IsLeapYear (Time) || Time->Day <= 28);

  if (Time->Day < 1 ||
      Time->Day > mDayOfMonth[Time->Month - 1] ||
      (Time->Month == 2 && !IsLeapYear (Time) && Time->Day > 28)) {
    return FALSE;
  }
  return TRUE;
}

STATIC
BOOLEAN
EFIAPI
IsTimeValid(
  IN EFI_TIME *Time
  )
{
  // Check the input parameters are within the range specified by UEFI
  if (Time->Year   < 1900               ||
      Time->Year   > 9999               ||
      Time->Month  < 1                  ||
      Time->Month  > 12                 ||
      !IsDayValid (Time)                ||
      Time->Hour   > 23                 ||
      Time->Minute > 59                 ||
      Time->Second > 59                 ||
      !IsValidTimeZone (Time->TimeZone) ||
      !IsValidDaylight (Time->Daylight)) {
    return FALSE;
  }
  return TRUE;
}

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param  Time                  A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities          An optional pointer to a buffer to receive the real time clock
                                device's capabilities.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware error.

**/
EFI_STATUS
EFIAPI
GetTime (
  OUT EFI_TIME                *Time,
  OUT EFI_TIME_CAPABILITIES   *Capabilities
  )
{
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set these first so the RealTimeClockLib implementation
  // can override them based on its own settings.
  //
  Time->TimeZone = mTimeSettings.TimeZone;
  Time->Daylight = mTimeSettings.Daylight;

  return LibGetTime (Time, Capabilities);
}



/**
  Sets the current local time and date information.

  @param  Time                  A pointer to the current time.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due due to hardware error.

**/
EFI_STATUS
EFIAPI
SetTime (
  IN EFI_TIME                *Time
  )
{
  EFI_STATUS        Status;
  BOOLEAN           TimeSettingsChanged;

  if (Time == NULL || !IsTimeValid (Time)) {
    return EFI_INVALID_PARAMETER;
  }

  TimeSettingsChanged = FALSE;
  if (mTimeSettings.TimeZone != Time->TimeZone ||
      mTimeSettings.Daylight != Time->Daylight) {

    mTimeSettings.TimeZone = Time->TimeZone;
    mTimeSettings.Daylight = Time->Daylight;
    TimeSettingsChanged = TRUE;
  }

  Status = LibSetTime (Time);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (TimeSettingsChanged) {
    Status = EfiSetVariable (
               (CHAR16 *)mTimeSettingsVariableName,
               &gEfiCallerIdGuid,
               EFI_VARIABLE_NON_VOLATILE |
               EFI_VARIABLE_BOOTSERVICE_ACCESS |
               EFI_VARIABLE_RUNTIME_ACCESS,
               sizeof (mTimeSettings),
               (VOID *)&mTimeSettings);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
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
GetWakeupTime (
  OUT BOOLEAN     *Enabled,
  OUT BOOLEAN     *Pending,
  OUT EFI_TIME    *Time
  )
{
  return LibGetWakeupTime (Enabled, Pending, Time);
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
SetWakeupTime (
  IN BOOLEAN      Enabled,
  OUT EFI_TIME    *Time
  )
{
  return LibSetWakeupTime (Enabled, Time);
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
InitializeRealTimeClock (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Size;

  Status = LibRtcInitialize (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Size = sizeof (mTimeSettings);
  Status = EfiGetVariable ((CHAR16 *)mTimeSettingsVariableName,
             &gEfiCallerIdGuid, NULL, &Size, (VOID *)&mTimeSettings);
  if (EFI_ERROR (Status) ||
      !IsValidTimeZone (mTimeSettings.TimeZone) ||
      !IsValidDaylight (mTimeSettings.Daylight)) {
    DEBUG ((DEBUG_WARN, "%a: using default timezone/daylight settings\n",
      __FUNCTION__));

    mTimeSettings.TimeZone = EFI_UNSPECIFIED_TIMEZONE;
    mTimeSettings.Daylight = 0;
  }

  SystemTable->RuntimeServices->GetTime       = GetTime;
  SystemTable->RuntimeServices->SetTime       = SetTime;
  SystemTable->RuntimeServices->GetWakeupTime = GetWakeupTime;
  SystemTable->RuntimeServices->SetWakeupTime = SetWakeupTime;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiRealTimeClockArchProtocolGuid,
                  NULL,
                  NULL
                  );

  return Status;
}

