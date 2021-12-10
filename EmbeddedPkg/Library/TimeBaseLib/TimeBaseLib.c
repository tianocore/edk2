/** @file
*
*  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2016-2019, Linaro Limited. All rights reserved.
*  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>
#include <Library/DebugLib.h>
#include <Library/TimeBaseLib.h>

/**
  Converts Epoch seconds (elapsed since 1970 JANUARY 01, 00:00:00 UTC) to EFI_TIME.

  @param  EpochSeconds   Epoch seconds.
  @param  Time           The time converted to UEFI format.

**/
VOID
EFIAPI
EpochToEfiTime (
  IN  UINTN     EpochSeconds,
  OUT EFI_TIME  *Time
  )
{
  UINTN  a;
  UINTN  b;
  UINTN  c;
  UINTN  d;
  UINTN  g;
  UINTN  j;
  UINTN  m;
  UINTN  y;
  UINTN  da;
  UINTN  db;
  UINTN  dc;
  UINTN  dg;
  UINTN  hh;
  UINTN  mm;
  UINTN  ss;
  UINTN  J;

  J  = (EpochSeconds / 86400) + 2440588;
  j  = J + 32044;
  g  = j / 146097;
  dg = j % 146097;
  c  = (((dg / 36524) + 1) * 3) / 4;
  dc = dg - (c * 36524);
  b  = dc / 1461;
  db = dc % 1461;
  a  = (((db / 365) + 1) * 3) / 4;
  da = db - (a * 365);
  y  = (g * 400) + (c * 100) + (b * 4) + a;
  m  = (((da * 5) + 308) / 153) - 2;
  d  = da - (((m + 4) * 153) / 5) + 122;

  Time->Year  = (UINT16)(y - 4800 + ((m + 2) / 12));
  Time->Month = ((m + 2) % 12) + 1;
  Time->Day   = (UINT8)(d + 1);

  ss = EpochSeconds % 60;
  a  = (EpochSeconds - ss) / 60;
  mm = a % 60;
  b  = (a - mm) / 60;
  hh = b % 24;

  Time->Hour       = (UINT8)hh;
  Time->Minute     = (UINT8)mm;
  Time->Second     = (UINT8)ss;
  Time->Nanosecond = 0;
}

/**
  Calculate Epoch days.

  @param    Time  The UEFI time to be calculated.

  @return   Number of days.

**/
UINTN
EFIAPI
EfiGetEpochDays (
  IN  EFI_TIME  *Time
  )
{
  UINTN  a;
  UINTN  y;
  UINTN  m;
  UINTN  JulianDate; // Absolute Julian Date representation of the supplied Time
  UINTN  EpochDays;  // Number of days elapsed since EPOCH_JULIAN_DAY

  a = (14 - Time->Month) / 12;
  y = Time->Year + 4800 - a;
  m = Time->Month + (12*a) - 3;

  JulianDate = Time->Day + ((153*m + 2)/5) + (365*y) + (y/4) - (y/100) + (y/400) - 32045;

  ASSERT (JulianDate >= EPOCH_JULIAN_DATE);
  EpochDays = JulianDate - EPOCH_JULIAN_DATE;

  return EpochDays;
}

/**
  Converts EFI_TIME to Epoch seconds (elapsed since 1970 JANUARY 01, 00:00:00 UTC).

  @param    Time  The UEFI time to be converted.

  @return   Number of seconds.

**/
UINTN
EFIAPI
EfiTimeToEpoch (
  IN  EFI_TIME  *Time
  )
{
  UINTN  EpochDays;  // Number of days elapsed since EPOCH_JULIAN_DAY
  UINTN  EpochSeconds;

  EpochDays = EfiGetEpochDays (Time);

  EpochSeconds = (EpochDays * SEC_PER_DAY) + ((UINTN)Time->Hour * SEC_PER_HOUR) + (Time->Minute * SEC_PER_MIN) + Time->Second;

  return EpochSeconds;
}

/**
  Get the day of the week from the UEFI time.

  @param    Time  The UEFI time to be calculated.

  @return   The day of the week: Sunday=0, Monday=1, ... Saturday=6

**/
UINTN
EfiTimeToWday (
  IN  EFI_TIME  *Time
  )
{
  UINTN  EpochDays;  // Number of days elapsed since EPOCH_JULIAN_DAY

  EpochDays = EfiGetEpochDays (Time);

  // 4=1/1/1970 was a Thursday

  return (EpochDays + 4) % 7;
}

/**
  Check if it is a leap year.

  @param    Time  The UEFI time to be checked.

  @retval   TRUE  It is a leap year.
  @retval   FALSE It is NOT a leap year.

**/
BOOLEAN
EFIAPI
IsLeapYear (
  IN EFI_TIME  *Time
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

/**
  Check if the day in the UEFI time is valid.

  @param    Time    The UEFI time to be checked.

  @retval   TRUE    Valid.
  @retval   FALSE   Invalid.

**/
BOOLEAN
EFIAPI
IsDayValid (
  IN  EFI_TIME  *Time
  )
{
  STATIC CONST INTN  DayOfMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if ((Time->Day < 1) ||
      (Time->Day > DayOfMonth[Time->Month - 1]) ||
      ((Time->Month == 2) && (!IsLeapYear (Time) && (Time->Day > 28)))
      )
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Check if the time zone is valid.
  Valid values are between -1440 and 1440 or 2047 (EFI_UNSPECIFIED_TIMEZONE).

  @param    TimeZone    The time zone to be checked.

  @retval   TRUE    Valid.
  @retval   FALSE   Invalid.

**/
BOOLEAN
EFIAPI
IsValidTimeZone (
  IN  INT16  TimeZone
  )
{
  return TimeZone == EFI_UNSPECIFIED_TIMEZONE ||
         (TimeZone >= -1440 && TimeZone <= 1440);
}

/**
  Check if the daylight is valid.
  Valid values are:
    0 : Time is not affected.
    1 : Time is affected, and has not been adjusted for daylight savings.
    3 : Time is affected, and has been adjusted for daylight savings.
  All other values are invalid.

  @param    Daylight    The daylight to be checked.

  @retval   TRUE    Valid.
  @retval   FALSE   Invalid.

**/
BOOLEAN
EFIAPI
IsValidDaylight (
  IN  INT8  Daylight
  )
{
  return Daylight == 0 ||
         Daylight == EFI_TIME_ADJUST_DAYLIGHT ||
         Daylight == (EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT);
}

/**
  Check if the UEFI time is valid.

  @param    Time    The UEFI time to be checked.

  @retval   TRUE    Valid.
  @retval   FALSE   Invalid.

**/
BOOLEAN
EFIAPI
IsTimeValid (
  IN EFI_TIME  *Time
  )
{
  // Check the input parameters are within the range specified by UEFI
  if ((Time->Year  < 2000)              ||
      (Time->Year   > 2099)              ||
      (Time->Month  < 1)              ||
      (Time->Month  > 12)              ||
      (!IsDayValid (Time))              ||
      (Time->Hour   > 23)              ||
      (Time->Minute > 59)              ||
      (Time->Second > 59)              ||
      (Time->Nanosecond > 999999999)     ||
      (!IsValidTimeZone (Time->TimeZone)) ||
      (!IsValidDaylight (Time->Daylight)))
  {
    return FALSE;
  }

  return TRUE;
}
