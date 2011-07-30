/** @file
  Transformations between the EFI_TIME structure and struct tm or time_t.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <Uefi.h>

#include  <LibConfig.h>

#include  <time.h>
#include  "tzfile.h"
#include  <MainData.h>

/* Convert an EFI_TIME structure into a C Standard tm structure. */
void
Efi2Tm( EFI_TIME *ET, struct tm *BT)
{
  // Convert EFI time to broken-down time.
  BT->tm_year = ET->Year - TM_YEAR_BASE;
  BT->tm_mon = ET->Month - 1;   // BD time is zero based, EFI is 1 based
  BT->tm_mday = ET->Day;
  BT->tm_hour = ET->Hour;
  BT->tm_min = ET->Minute;
  BT->tm_sec = ET->Second;
  BT->tm_isdst = -1;
  BT->tm_zoneoff = ET->TimeZone;
  BT->tm_daylight = ET->Daylight;
  BT->tm_Nano = ET->Nanosecond;
}

/* Convert an EFI_TIME structure into a time_t value. */
time_t
Efi2Time( EFI_TIME *EfiBDtime)
{
  Efi2Tm( EfiBDtime, &gMD->BDTime);

  return mktime( &gMD->BDTime);
}
