/** @file
  C Run-Time Libraries (CRT) Time Management Routines Wrapper Implementation
  for MbedTLS-based Cryptographic Library (used in DXE & RUNTIME).

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

typedef int time_t;

//
// Structures Definitions
//
struct tm {
  int     tm_sec;    /* seconds after the minute [0-60] */
  int     tm_min;    /* minutes after the hour [0-59] */
  int     tm_hour;   /* hours since midnight [0-23] */
  int     tm_mday;   /* day of the month [1-31] */
  int     tm_mon;    /* months since January [0-11] */
  int     tm_year;   /* years since 1900 */
  int     tm_wday;   /* days since Sunday [0-6] */
  int     tm_yday;   /* days since January 1 [0-365] */
  int     tm_isdst;  /* Daylight Savings Time flag */
  long    tm_gmtoff; /* offset from CUT in seconds */
  char    *tm_zone;  /* timezone abbreviation */
};

//
// -- Time Management Routines --
//

#define IsLeap(y)  (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
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

/** Get the system time as seconds elapsed since midnight, January 1, 1970. **/
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

/** Convert a time value from type time_t to struct tm. **/
struct tm *
gmtime (
  const time_t  *timer
  )
{
  struct tm  *GmTime;
  UINT16     DayNo;
  UINT16     DayRemainder;
  time_t     Year;
  time_t     YearNo;
  UINT16     TotalDays;
  UINT16     MonthNo;

  if (timer == NULL) {
    return NULL;
  }

  GmTime = AllocateZeroPool (sizeof (struct tm));
  if (GmTime == NULL) {
    return NULL;
  }

  ZeroMem ((VOID *)GmTime, (UINTN)sizeof (struct tm));

  DayNo        = (UINT16)(*timer / SECSPERDAY);
  DayRemainder = (UINT16)(*timer % SECSPERDAY);

  GmTime->tm_sec  = (int)(DayRemainder % SECSPERMIN);
  GmTime->tm_min  = (int)((DayRemainder % SECSPERHOUR) / SECSPERMIN);
  GmTime->tm_hour = (int)(DayRemainder / SECSPERHOUR);
  GmTime->tm_wday = (int)((DayNo + 4) % 7);

  for (Year = 1970, YearNo = 0; DayNo > 0; Year++) {
    TotalDays = (UINT16)(IsLeap (Year) ? 366 : 365);
    if (DayNo >= TotalDays) {
      DayNo = (UINT16)(DayNo - TotalDays);
      YearNo++;
    } else {
      break;
    }
  }

  GmTime->tm_year = (int)(YearNo + (1970 - 1900));
  GmTime->tm_yday = (int)DayNo;

  for (MonthNo = 12; MonthNo > 1; MonthNo--) {
    if (DayNo >= CumulativeDays[IsLeap (Year)][MonthNo]) {
      DayNo = (UINT16)(DayNo - (UINT16)(CumulativeDays[IsLeap (Year)][MonthNo]));
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

/**_time64 function. **/
time_t
_time64 (
  time_t  *t
  )
{
  return time (t);
}
