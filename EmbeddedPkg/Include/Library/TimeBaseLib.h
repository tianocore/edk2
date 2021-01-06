/** @file
*
*  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2016-2019, Linaro Limited. All rights reserved.
*  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _TIME_BASE_LIB_H_
#define _TIME_BASE_LIB_H_

#include <Uefi/UefiBaseType.h>

//
// Convenience macros to obtain a build date
//
// These macros should work for any compiler that follows ISO/IEC 9899,
// in which case __DATE__ is defined as a "Mmm dd yyyy" 11 chars string,
// but add an explicit filter for compilers that have been validated.
//
#if (defined(__GNUC__) || defined(_MSC_VER) || defined(__clang__))
#define TIME_BUILD_YEAR  (__DATE__[7] == '?' ? 1900 \
          : (((__DATE__[7] - '0') * 1000 )          \
          + (__DATE__[8] - '0') * 100               \
          + (__DATE__[9] - '0') * 10                \
          + __DATE__[10] - '0'))
#define TIME_BUILD_MONTH ( __DATE__ [2] == '?' ? 1  \
          : __DATE__ [2] == 'n' ? (                 \
            __DATE__ [1] == 'a' ? 1 : 6)            \
          : __DATE__ [2] == 'b' ? 2                 \
          : __DATE__ [2] == 'r' ? (                 \
            __DATE__ [0] == 'M' ? 3 : 4)            \
          : __DATE__ [2] == 'y' ? 5                 \
          : __DATE__ [2] == 'l' ? 7                 \
          : __DATE__ [2] == 'g' ? 8                 \
          : __DATE__ [2] == 'p' ? 9                 \
          : __DATE__ [2] == 't' ? 10                \
          : __DATE__ [2] == 'v' ? 11                \
          : 12)
#define TIME_BUILD_DAY ( __DATE__[4] == '?' ? 1     \
          : ((__DATE__[4] == ' ' ? 0 :              \
            ((__DATE__[4] - '0') * 10))             \
          + __DATE__[5] - '0'))
#endif

// Define EPOCH (1970-JANUARY-01) in the Julian Date representation
#define EPOCH_JULIAN_DATE                               2440588

// Seconds per unit
#define SEC_PER_MIN                                     ((UINTN)    60)
#define SEC_PER_HOUR                                    ((UINTN)  3600)
#define SEC_PER_DAY                                     ((UINTN) 86400)

/**
  Check if it is a leap year.

  @param    Time  The UEFI time to be checked.

  @retval   TRUE  It is a leap year.
  @retval   FALSE It is NOT a leap year.

**/
BOOLEAN
EFIAPI
IsLeapYear (
  IN  EFI_TIME  *Time
  );

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
  );

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
  );

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
  );

/**
  Check if the UEFI time is valid.

  @param    Time    The UEFI time to be checked.

  @retval   TRUE    Valid.
  @retval   FALSE   Invalid.

**/
BOOLEAN
EFIAPI
IsTimeValid (
  IN  EFI_TIME  *Time
  );

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
  );

/**
  Converts EFI_TIME to Epoch seconds (elapsed since 1970 JANUARY 01, 00:00:00 UTC).

  @param    Time  The UEFI time to be converted.

  @return   Number of seconds.

**/
UINTN
EFIAPI
EfiTimeToEpoch (
  IN  EFI_TIME  *Time
  );

/**
  Get the day of the week from the UEFI time.

  @param    Time  The UEFI time to be calculated.

  @return   The day of the week: Sunday=0, Monday=1, ... Saturday=6

**/
UINTN
EfiTimeToWday (
  IN  EFI_TIME  *Time
  );

#endif
