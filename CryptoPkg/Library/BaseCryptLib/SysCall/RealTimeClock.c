/** @file
  C Run-Time Libraries (CRT) Time Management Routines Wrapper Implementation
  for OpenSSL-based Cryptographic Library (used in SMM).

Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <OpenSslSupport.h>

#define PCAT_RTC_ADDRESS_REGISTER 0x70
#define PCAT_RTC_DATA_REGISTER    0x71

#define RTC_ADDRESS_SECONDS           0   // R/W  Range 0..59
#define RTC_ADDRESS_SECONDS_ALARM     1   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES           2   // R/W  Range 0..59
#define RTC_ADDRESS_MINUTES_ALARM     3   // R/W  Range 0..59
#define RTC_ADDRESS_HOURS             4   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_HOURS_ALARM       5   // R/W  Range 1..12 or 0..23 Bit 7 is AM/PM
#define RTC_ADDRESS_DAY_OF_THE_WEEK   6   // R/W  Range 1..7
#define RTC_ADDRESS_DAY_OF_THE_MONTH  7   // R/W  Range 1..31
#define RTC_ADDRESS_MONTH             8   // R/W  Range 1..12
#define RTC_ADDRESS_YEAR              9   // R/W  Range 0..99
#define RTC_ADDRESS_REGISTER_A        10  // R/W[0..6]  R0[7]
#define RTC_ADDRESS_REGISTER_B        11  // R/W
#define RTC_ADDRESS_REGISTER_C        12  // RO
#define RTC_ADDRESS_REGISTER_D        13  // RO
#define RTC_ADDRESS_CENTURY           50  // R/W  Range 19..20 Bit 8 is R/W

//
// Register A
//
typedef struct {
  UINT8 RS : 4;   // Rate Selection Bits
  UINT8 DV : 3;   // Divisor
  UINT8 UIP : 1;  // Update in progress
} RTC_REGISTER_A_BITS;

typedef union {
  RTC_REGISTER_A_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_A;

//
// Register B
//
typedef struct {
  UINT8 DSE : 1;  // 0 - Daylight saving disabled  1 - Daylight savings enabled
  UINT8 MIL : 1;  // 0 - 12 hour mode              1 - 24 hour mode
  UINT8 DM : 1;   // 0 - BCD Format                1 - Binary Format
  UINT8 SQWE : 1; // 0 - Disable SQWE output       1 - Enable SQWE output
  UINT8 UIE : 1;  // 0 - Update INT disabled       1 - Update INT enabled
  UINT8 AIE : 1;  // 0 - Alarm INT disabled        1 - Alarm INT Enabled
  UINT8 PIE : 1;  // 0 - Periodic INT disabled     1 - Periodic INT Enabled
  UINT8 SET : 1;  // 0 - Normal operation.         1 - Updates inhibited
} RTC_REGISTER_B_BITS;

typedef union {
  RTC_REGISTER_B_BITS Bits;
  UINT8               Data;
} RTC_REGISTER_B;

//
// -- Time Management Routines --
//

#define IsLeap(y)   (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define SECSPERMIN  (60)
#define SECSPERHOUR (60 * 60)
#define SECSPERDAY  (24 * SECSPERHOUR)

//
//  The arrays give the cumulative number of days up to the first of the
//  month number used as the index (1 -> 12) for regular and leap years.
//  The value at index 13 is for the whole year.
//
UINTN CumulativeDays[2][14] = {
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

/**
  Read RTC content through its registers.

  @param  Address  Address offset of RTC. It is recommended to use macros such as
                   RTC_ADDRESS_SECONDS.

  @return The data of UINT8 type read from RTC.
**/
UINT8
RtcRead (
  IN  UINT8 Address
  )
{
  IoWrite8 (PCAT_RTC_ADDRESS_REGISTER, (UINT8) (Address | (UINT8) (IoRead8 (PCAT_RTC_ADDRESS_REGISTER) & BIT7)));
  return IoRead8 (PCAT_RTC_DATA_REGISTER);
}

/* Get the system time as seconds elapsed since midnight, January 1, 1970. */
//INTN time(
//  INTN *timer
//  )
time_t time (time_t *timer)
{
  UINT16          Year;
  UINT8           Month;
  UINT8           Day;
  UINT8           Hour;
  UINT8           Minute;
  UINT8           Second;
  UINT8           Century;
  RTC_REGISTER_A  RegisterA;
  RTC_REGISTER_B  RegisterB;
  BOOLEAN         IsPM;
  UINT16          YearIndex;

  RegisterA.Data  = RtcRead (RTC_ADDRESS_REGISTER_A);
  while (RegisterA.Bits.UIP == 1) {
    CpuPause();
    RegisterA.Data = RtcRead (RTC_ADDRESS_REGISTER_A);
  }

  Second  = RtcRead (RTC_ADDRESS_SECONDS);
  Minute  = RtcRead (RTC_ADDRESS_MINUTES);
  Hour    = RtcRead (RTC_ADDRESS_HOURS);
  Day     = RtcRead (RTC_ADDRESS_DAY_OF_THE_MONTH);
  Month   = RtcRead (RTC_ADDRESS_MONTH);
  Year    = RtcRead (RTC_ADDRESS_YEAR);
  Century = RtcRead (RTC_ADDRESS_CENTURY);

  RegisterB.Data = RtcRead (RTC_ADDRESS_REGISTER_B);

  if ((Hour & BIT7) != 0) {
    IsPM = TRUE;
  } else {
    IsPM = FALSE;
  }
  Hour = (UINT8) (Hour & 0x7f);

  if (RegisterB.Bits.DM == 0) {
    Year    = BcdToDecimal8 ((UINT8) Year);
    Month   = BcdToDecimal8 (Month);
    Day     = BcdToDecimal8 (Day);
    Hour    = BcdToDecimal8 (Hour);
    Minute  = BcdToDecimal8 (Minute);
    Second  = BcdToDecimal8 (Second);
  }
  Century   = BcdToDecimal8 (Century);

  Year = (UINT16) (Century * 100 + Year);

  //
  // If time is in 12 hour format, convert it to 24 hour format
  //
  if (RegisterB.Bits.MIL == 0) {
    if (IsPM && Hour < 12) {
      Hour = (UINT8) (Hour + 12);
    }
    if (!IsPM && Hour == 12) {
      Hour = 0;
    }
  }

  //
  // Years Handling
  // UTime should now be set to 00:00:00 on Jan 1 of the current year.
  //
  for (YearIndex = 1970, *timer = 0; YearIndex != Year; YearIndex++) {
    *timer = *timer + (time_t)(CumulativeDays[IsLeap(YearIndex)][13] * SECSPERDAY);
  }

  //
  // Add in number of seconds for current Month, Day, Hour, Minute, Seconds, and TimeZone adjustment
  //
  ASSERT (Month <= 12);
  *timer = *timer + 
           (time_t)(CumulativeDays[IsLeap(Year)][Month] * SECSPERDAY) + 
           (time_t)((Day - 1) * SECSPERDAY) + 
           (time_t)(Hour * SECSPERHOUR) + 
           (time_t)(Minute * 60) + 
           (time_t)Second;

  return *timer;
}

//
// Convert a time value from type time_t to struct tm.
//
struct tm * gmtime (const time_t *timer)
{
  struct tm      *GmTime;
  UINT16         DayNo;
  UINT16         DayRemainder;
  time_t         Year;
  time_t         YearNo;
  UINT16         TotalDays;
  UINT16         MonthNo;

  if (timer == NULL) {
    return NULL;
  }

  GmTime = malloc (sizeof (struct tm));
  if (GmTime == NULL) {
    return NULL;
  }

  ZeroMem ((VOID *) GmTime, (UINTN) sizeof (struct tm));

  DayNo        = (UINT16) (*timer / SECSPERDAY);
  DayRemainder = (UINT16) (*timer % SECSPERDAY);

  GmTime->tm_sec = (int) (DayRemainder % SECSPERMIN);
  GmTime->tm_min = (int) ((DayRemainder % SECSPERHOUR) / SECSPERMIN);
  GmTime->tm_hour = (int) (DayRemainder / SECSPERHOUR);
  GmTime->tm_wday = (int) ((DayNo + 4) % 7);

  for (Year = 1970, YearNo = 0; DayNo > 0; Year++) {  
    TotalDays = (UINT16) (IsLeap (Year) ? 366 : 365);
    if (DayNo >= TotalDays) {
      DayNo = (UINT16) (DayNo - TotalDays);
      YearNo++;
    } else {
      break;
    }
  }

  GmTime->tm_year = (int) (YearNo + (1970 - 1900));
  GmTime->tm_yday = (int) DayNo;

  for (MonthNo = 12; MonthNo > 1; MonthNo--) {
    if (DayNo > CumulativeDays[IsLeap(Year)][MonthNo]) {
      DayNo = (UINT16) (DayNo - (UINT16) (CumulativeDays[IsLeap(Year)][MonthNo]));
      break;
    }
  }

  GmTime->tm_mon  = (int) MonthNo;
  GmTime->tm_mday = (int) DayNo;

  GmTime->tm_isdst  = 0;
  GmTime->tm_gmtoff = 0;
  GmTime->tm_zone   = NULL;

  return GmTime;
}

