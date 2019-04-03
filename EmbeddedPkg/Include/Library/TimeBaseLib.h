/** @file
*
*  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2016, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef _TIME_BASE_LIB_H_
#define _TIME_BASE_LIB_H_

#include <Uefi/UefiBaseType.h>

// Define EPOCH (1970-JANUARY-01) in the Julian Date representation
#define EPOCH_JULIAN_DATE                               2440588

// Seconds per unit
#define SEC_PER_MIN                                     ((UINTN)    60)
#define SEC_PER_HOUR                                    ((UINTN)  3600)
#define SEC_PER_DAY                                     ((UINTN) 86400)
#define SEC_PER_MONTH                                   ((UINTN)  2,592,000)
#define SEC_PER_YEAR                                    ((UINTN) 31,536,000)

BOOLEAN
EFIAPI
IsLeapYear (
  IN  EFI_TIME  *Time
  );

BOOLEAN
EFIAPI
IsDayValid (
  IN  EFI_TIME  *Time
  );

BOOLEAN
EFIAPI
IsTimeValid (
  IN  EFI_TIME  *Time
  );

/**
  Converts Epoch seconds (elapsed since 1970 JANUARY 01, 00:00:00 UTC) to EFI_TIME
 **/
VOID
EFIAPI
EpochToEfiTime (
  IN  UINTN     EpochSeconds,
  OUT EFI_TIME  *Time
  );

/**
  Converts EFI_TIME to Epoch seconds (elapsed since 1970 JANUARY 01, 00:00:00 UTC)
 **/
UINTN
EFIAPI
EfiTimeToEpoch (
  IN  EFI_TIME  *Time
  );

/**
  returns Day of the week [0-6] 0=Sunday
 **/
UINTN
EfiTimeToWday (
  IN  EFI_TIME  *Time
  );

#endif
