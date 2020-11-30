/** @file
  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

  Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"
#include <Library/HobLib.h>
#include <Guid/FirmwareVersionInfoHobGuid.h>


/**
 * Field Filling Function. Transform an EFI_EXP_BASE2_DATA to a byte, with '64k'
 * as the unit.
 *
 * @param  Value              Pointer to Base2_Data
 *
 * @retval
 *
**/
UINT8
Base2ToByteWith64KUnit (
  IN  UINTN  Value
  )
{
  UINT8 Size;

  Size = Value / SIZE_64KB + (Value % SIZE_64KB + SIZE_64KB - 1) / SIZE_64KB;

  return Size;
}


/**
 * Fetches the firmware ('BIOS') release date from the
 * FirmwareVersionInfo HOB.
 *
 * @return The release date as a UTF-16 string
**/
VOID *
GetBiosReleaseDate (
  VOID
  )
{
  CHAR16                  *ReleaseDate = NULL;
  FIRMWARE_VERSION_INFO   *Version;
  VOID                    *Hob;

  ReleaseDate = AllocateZeroPool ((sizeof (CHAR16)) * SMBIOS_STRING_MAX_LENGTH);
  if (ReleaseDate == NULL) {
      return NULL;
  }

  Hob = GetFirstGuidHob (&gFirmwareVersionInfoHobGuid);
  if (Hob == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a:%d] Version info HOB not found!\n", __FUNCTION__, __LINE__));
    return NULL;
  }

  Version = GET_GUID_HOB_DATA (Hob);
  (VOID)UnicodeSPrintAsciiFormat (ReleaseDate,
                        (sizeof (CHAR16)) * SMBIOS_STRING_MAX_LENGTH,
                        "%02d/%02d/%4d",
                        Version->BuildTime.Month,
                        Version->BuildTime.Day,
                        Version->BuildTime.Year
                        );

  return ReleaseDate;
}

/**
 * Fetches the firmware ('BIOS') version from the
 * FirmwareVersionInfo HOB.
 *
 * @return The version as a UTF-16 string
**/
VOID *
GetBiosVersion (
  VOID
  )
{
  FIRMWARE_VERSION_INFO   *Version;
  VOID                    *Hob;

  Hob = GetFirstGuidHob (&gFirmwareVersionInfoHobGuid);
  if (Hob == NULL) {
    DEBUG ((DEBUG_ERROR, "[%a:%d] Version info HOB not found!\n",
            __FUNCTION__, __LINE__));
    return NULL;
  }

  Version = GET_GUID_HOB_DATA (Hob);
  return Version->String;
}


/**
 * This function makes boot time changes to the contents of the
 * MiscBiosVendor (Type 0).
 *
 * @param  RecordData                 Pointer to copy of RecordData from the Data Table.
 *
 * @retval EFI_SUCCESS                All parameters were valid.
 * @retval EFI_UNSUPPORTED            Unexpected RecordType value.
 * @retval EFI_INVALID_PARAMETER      Invalid parameter was found.
 *
**/
MISC_SMBIOS_TABLE_FUNCTION(MiscBiosVendor)
{
  CHAR8                 *OptionalStrStart;
  CHAR8                 *StrStart;
  UINTN                 VendorStrLen;
  UINTN                 VerStrLen;
  UINTN                 DateStrLen;
  UINTN                 BiosPhysicalSizeHexValue;
  CHAR16                *Vendor;
  CHAR16                *Version;
  CHAR16                *ReleaseDate;
  CHAR16                *Char16String;
  EFI_STATUS            Status;
  EFI_STRING_ID         TokenToUpdate;
  EFI_STRING_ID         TokenToGet;
  SMBIOS_TABLE_TYPE0    *SmbiosRecord;
  EFI_SMBIOS_HANDLE     SmbiosHandle;
  SMBIOS_TABLE_TYPE0    *InputData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputData = (SMBIOS_TABLE_TYPE0 *)RecordData;

  Vendor = (CHAR16 *) PcdGetPtr (PcdFirmwareVendor);

  if (StrLen (Vendor) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BIOS_VENDOR);
    HiiSetString (mHiiHandle, TokenToUpdate, Vendor, NULL);
  }

  Version = GetBiosVersion();

  if (StrLen (Version) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BIOS_VERSION);
    HiiSetString (mHiiHandle, TokenToUpdate, Version, NULL);
  } else {
    Version = (CHAR16 *) PcdGetPtr (PcdFirmwareVersionString);
    if (StrLen (Version) > 0) {
      TokenToUpdate = STRING_TOKEN (STR_MISC_BIOS_VERSION);
      HiiSetString (mHiiHandle, TokenToUpdate, Version, NULL);
    }
  }

  Char16String = GetBiosReleaseDate ();
  if (StrLen(Char16String) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BIOS_RELEASE_DATE);
    HiiSetString (mHiiHandle, TokenToUpdate, Char16String, NULL);
  }

  TokenToGet = STRING_TOKEN (STR_MISC_BIOS_VENDOR);
  Vendor = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  VendorStrLen = StrLen (Vendor);

  TokenToGet = STRING_TOKEN (STR_MISC_BIOS_VERSION);
  Version = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen = StrLen (Version);

  TokenToGet = STRING_TOKEN (STR_MISC_BIOS_RELEASE_DATE);
  ReleaseDate = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  DateStrLen = StrLen (ReleaseDate);

  //
  // Now update the BiosPhysicalSize
  //
  BiosPhysicalSizeHexValue = FixedPcdGet32 (PcdFdSize);

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE0) + VendorStrLen + 1 +
                                   VerStrLen + 1 +
                                   DateStrLen + 1 + 1);
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  (VOID)CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE0));

  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE0);
  SmbiosRecord->BiosSegment = (UINT16)(FixedPcdGet32 (PcdFdBaseAddress) / SIZE_64KB);
  SmbiosRecord->BiosSize = Base2ToByteWith64KUnit (BiosPhysicalSizeHexValue) - 1;

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStrS (Vendor, OptionalStrStart, VendorStrLen + 1);
  StrStart = OptionalStrStart + VendorStrLen + 1;
  UnicodeStrToAsciiStrS (Version, StrStart, VerStrLen + 1);
  StrStart += VerStrLen + 1;
  UnicodeStrToAsciiStrS (ReleaseDate, StrStart, DateStrLen + 1);
  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = LogSmbiosData ((UINT8*)SmbiosRecord, &SmbiosHandle);
  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[%a]:[%dL] Smbios Type00 Table Log Failed! %r \n",
              __FUNCTION__, __LINE__, Status));
  }

  FreePool (SmbiosRecord);

Exit:
  if (Vendor != NULL) {
    FreePool (Vendor);
  }

  if (Version != NULL) {
    FreePool (Version);
  }

  if (ReleaseDate != NULL) {
    FreePool (ReleaseDate);
  }

  if (Char16String != NULL) {
    FreePool (Char16String);
  }

  return Status;
}
