/** @file
  C Run-Time Libraries (CRT) Time Management Routines Wrapper Implementation
  for OpenSSL-based Cryptographic Library (used in DXE & RUNTIME).

Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <CrtLibSupport.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// -- Time Management Routines --
//

#define SECSPERMIN   (60)
#define SECSPERHOUR  (60 * 60)
#define SECSPERDAY   (24 * SECSPERHOUR)

//
//  The arrays give the cumulative number of days up to the first of the
//  month number used as the index (1 -> 12) for regular and leap years.
//  The value at index 13 is for the whole year.
//
UINTN  CumulativeDays[2][14] = {
  {
    0,
    0,
    31,
    31 + 28,
    31 + 28 + 31,
    31 + 28 + 31 + 30,
    31 + 28 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
  },
  {
    0,
    0,
    31,
    31 + 29,
    31 + 29 + 31,
    31 + 29 + 31 + 30,
    31 + 29 + 31 + 30 + 31,
    31 + 29 + 31 + 30 + 31 + 30,
    31 + 29 + 31 + 30 + 31 + 30 + 31,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
    31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31
  }
};

/* Check the year is leap or not. */
// BOOLEAN IsLeap(
//  INTN timer
//  )
BOOLEAN
IsLeap (
  time_t  timer
  )
{
  INT64  Remainder1;
  INT64  Remainder2;
  INT64  Remainder3;

  DivS64x64Remainder (timer, 4, &Remainder1);
  DivS64x64Remainder (timer, 100, &Remainder2);
  DivS64x64Remainder (timer, 400, &Remainder3);

  return (Remainder1 == 0 && (Remainder2 != 0 || Remainder3 == 0));
}

/* Get the system time as seconds elapsed since midnight, January 1, 1970. */
// INTN time(
//  INTN *timer
//  )
time_t
time (
  time_t  *timer
  )
{
  EFI_STATUS  Status;
  EFI_TIME    Time;
  time_t      CalTime;
  UINTN       Year;

  //
  // Get the current time and date information
  //
  Status = gRT->GetTime (&Time, NULL);
  if (EFI_ERROR (Status) || (Time.Year < 1970)) {
    return 0;
  }

  //
  // Years Handling
  // UTime should now be set to 00:00:00 on Jan 1 of the current year.
  //
  for (Year = 1970, CalTime = 0; Year != Time.Year; Year++) {
    CalTime = CalTime + (time_t)(CumulativeDays[IsLeap (Year)][13] * SECSPERDAY);
  }

  //
  // Add in number of seconds for current Month, Day, Hour, Minute, Seconds, and TimeZone adjustment
  //
  CalTime = CalTime +
            (time_t)((Time.TimeZone != EFI_UNSPECIFIED_TIMEZONE) ? (Time.TimeZone * 60) : 0) +
            (time_t)(CumulativeDays[IsLeap (Time.Year)][Time.Month] * SECSPERDAY) +
            (time_t)(((Time.Day > 0) ? Time.Day - 1 : 0) * SECSPERDAY) +
            (time_t)(Time.Hour * SECSPERHOUR) +
            (time_t)(Time.Minute * 60) +
            (time_t)Time.Second;

  if (timer != NULL) {
    *timer = CalTime;
  }

  return CalTime;
}

//
// Convert a time value from type time_t to struct tm.
//
struct tm *
gmtime (
  const time_t  *timer
  )
{
  struct tm  *GmTime;
  UINT64     DayNo;
  UINT64     DayRemainder;
  time_t     Year;
  time_t     YearNo;
  UINT32     TotalDays;
  UINT32     MonthNo;
  INT64      Remainder;

  if (timer == NULL) {
    return NULL;
  }

  GmTime = malloc (sizeof (struct tm));
  if (GmTime == NULL) {
    return NULL;
  }

  ZeroMem ((VOID *)GmTime, (UINTN)sizeof (struct tm));

  DayNo        = (UINT64)DivS64x64Remainder (*timer, SECSPERDAY, &Remainder);
  DayRemainder = (UINT64)Remainder;

  DivS64x64Remainder (DayRemainder, SECSPERMIN, &Remainder);
  GmTime->tm_sec = (int)Remainder;
  DivS64x64Remainder (DayRemainder, SECSPERHOUR, &Remainder);
  GmTime->tm_min  = (int)DivS64x64Remainder (Remainder, SECSPERMIN, NULL);
  GmTime->tm_hour = (int)DivS64x64Remainder (DayRemainder, SECSPERHOUR, NULL);
  DivS64x64Remainder ((DayNo + 4), 7, &Remainder);
  GmTime->tm_wday = (int)Remainder;

  for (Year = 1970, YearNo = 0; DayNo > 0; Year++) {
    TotalDays = (UINT32)(IsLeap (Year) ? 366 : 365);
    if (DayNo >= TotalDays) {
      DayNo = (UINT64)(DayNo - TotalDays);
      YearNo++;
    } else {
      break;
    }
  }

  GmTime->tm_year = (int)(YearNo + (1970 - 1900));
  GmTime->tm_yday = (int)DayNo;

  for (MonthNo = 12; MonthNo > 1; MonthNo--) {
    if (DayNo >= CumulativeDays[IsLeap (Year)][MonthNo]) {
      DayNo = (UINT64)(DayNo - (UINT32)(CumulativeDays[IsLeap (Year)][MonthNo]));
      break;
    }
  }

  GmTime->tm_mon  = (int)MonthNo - 1;
  GmTime->tm_mday = (int)DayNo + 1;

  GmTime->tm_isdst  = 0;
  GmTime->tm_gmtoff = 0;
  GmTime->tm_zone   = NULL;

  return GmTime;
}
