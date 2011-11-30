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

/** Convert an EFI_TIME structure into a C Standard tm structure.

    @param[in]    ET    Pointer to the EFI_TIME structure to convert.
    @param[out]   BT    Pointer to the tm structure to receive the converted time.
*/
void
Efi2Tm(
  IN      EFI_TIME  *ET,
     OUT  struct tm *BT
  )
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

/** Convert an EFI_TIME structure into a time_t value.

    @param[in]  EfiBDtime   Pointer to the EFI_TIME structure to convert.

    @return   The EFI_TIME converted into a time_t value.
*/
time_t
Efi2Time(
  IN  EFI_TIME *EfiBDtime
  )
{
  Efi2Tm( EfiBDtime, &gMD->BDTime);

  return mktime( &gMD->BDTime);
}

/** Convert a C Standard tm structure into an EFI_TIME structure.

    @param[in]    BT    Pointer to the tm structure to convert.
    @param[out]   ET    Pointer to an EFI_TIME structure to receive the converted time.
*/
void
Tm2Efi(
  IN      struct tm *BT,
     OUT  EFI_TIME  *ET
  )
{
  ET->Year        = (UINT16)BT->tm_year + TM_YEAR_BASE;
  ET->Month       = (UINT8)BT->tm_mon + 1;
  ET->Day         = (UINT8)BT->tm_mday;
  ET->Hour        = (UINT8)BT->tm_hour;
  ET->Minute      = (UINT8)BT->tm_min;
  ET->Second      = (UINT8)BT->tm_sec;
  ET->Nanosecond  = (UINT32)BT->tm_Nano;
  ET->TimeZone    = (INT16)BT->tm_zoneoff;
  ET->Daylight    = (UINT8)BT->tm_daylight;
}

/** Convert a time_t value into an EFI_TIME structure.

    @param[in]    CalTime   Calendar time as a time_t value.

    @return   Returns a newly malloced EFI_TIME structure containing
              the converted calendar time.

    @post     It is the responsibility of the caller to free the
              returned structure before the application exits.
*/
EFI_TIME*
Time2Efi(
  IN  time_t CalTime
  )
{
  struct tm *IT;
  EFI_TIME  *ET   = NULL;

  IT = gmtime(&CalTime);
  if(IT != NULL) {
    ET = malloc(sizeof(EFI_TIME));
    if(ET != NULL) {
      Tm2Efi(IT, ET);
    }
  }
  return ET;
}
