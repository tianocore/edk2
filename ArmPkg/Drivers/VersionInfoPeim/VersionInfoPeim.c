/** @file
*
*  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2016, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#include <Guid/VersionInfoHobGuid.h>

struct MonthDescription {
  CONST CHAR8* MonthStr;
  UINT32    MonthInt;
} gMonthDescription[] = {
  { "Jan", 1 },
  { "Feb", 2 },
  { "Mar", 3 },
  { "Apr", 4 },
  { "May", 5 },
  { "Jun", 6 },
  { "Jul", 7 },
  { "Aug", 8 },
  { "Sep", 9 },
  { "Oct", 10 },
  { "Nov", 11 },
  { "Dec", 12 },
  { "???", 1 },  // Use 1 as default month
};

VOID GetReleaseTime (EFI_TIME *Time)
{
  CONST CHAR8      *ReleaseDate = __DATE__;
  CONST CHAR8      *ReleaseTime = __TIME__;
  UINTN            i;

  for (i = 0;i < 12;i++) {
    if (AsciiStrnCmp (ReleaseDate, gMonthDescription[i].MonthStr, 3) == 0) {
      break;
    }
  }

  Time->Month = gMonthDescription[i].MonthInt;
  Time->Day = AsciiStrDecimalToUintn(ReleaseDate+4);
  Time->Year = AsciiStrDecimalToUintn(ReleaseDate+7);
  Time->Hour = AsciiStrDecimalToUintn(ReleaseTime);
  Time->Minute = AsciiStrDecimalToUintn(ReleaseTime+3);
  Time->Second = AsciiStrDecimalToUintn(ReleaseTime+6);

  return;
}

EFI_STATUS
EFIAPI
VersionInfoEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  VERSION_INFO *VersionInfo;
  EFI_TIME Time = {0};
  CONST CHAR16 *ReleaseString =
    (CHAR16 *) FixedPcdGetPtr (PcdFirmwareVersionString);

  GetReleaseTime (&Time);

  VersionInfo = BuildGuidHob (&gVersionInfoHobGuid,
                      sizeof (VERSION_INFO) -
                      sizeof (VersionInfo->String) +
                      StrSize (ReleaseString));
  if (VersionInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a]:[%d] Build HOB failed!\n", __FILE__, __LINE__));
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&VersionInfo->BuildTime, &Time, sizeof (EFI_TIME));
  CopyMem (VersionInfo->String, ReleaseString, StrSize (ReleaseString));

  return EFI_SUCCESS;
}
