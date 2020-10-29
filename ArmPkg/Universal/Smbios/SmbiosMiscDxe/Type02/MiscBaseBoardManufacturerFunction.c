/** @file

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MiscBaseBoardManufacturerFunction.c

Abstract:

  This driver parses the mSmbiosMiscDataTable structure and reports
  any generated data using SMBIOS protocol.

Based on files under Nt32Pkg/MiscSubClassPlatformDxe/
**/

#include "SmbiosMisc.h"


/**
  This function makes basic board manufacturer to the contents of the
  Misc Base Board Manufacturer (Type 2).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscBaseBoardManufacturer)
{
  CHAR8                             *OptionalStrStart;
  UINTN                             ManuStrLen;
  UINTN                             ProductNameStrLen;
  UINTN                             VerStrLen;
  UINTN                             SerialNumStrLen;
  UINTN                             AssetTagStrLen;
  UINTN                             ChassisLocaStrLen;
  UINTN                             HandleCount = 0;
  UINT16                            *HandleArray = NULL;
  CHAR16                            *BaseBoardManufacturer;
  CHAR16                            *BaseBoardProductName;
  CHAR16                            *Version;
  EFI_STRING                        SerialNumber;
  EFI_STRING                        AssetTag;
  EFI_STRING                        ChassisLocation;
  EFI_STRING_ID                     TokenToGet;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  SMBIOS_TABLE_TYPE2                *SmbiosRecord;
  SMBIOS_TABLE_TYPE2                *InputData = NULL;
  EFI_STATUS                        Status;

  EFI_STRING_ID                     TokenToUpdate;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputData = (SMBIOS_TABLE_TYPE2*)RecordData;

  BaseBoardManufacturer = (CHAR16 *) PcdGetPtr (PcdBaseBoardManufacturer);
  if (StrLen (BaseBoardManufacturer) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BASE_BOARD_MANUFACTURER);
    HiiSetString (mHiiHandle, TokenToUpdate, BaseBoardManufacturer, NULL);
  }

  BaseBoardProductName = (CHAR16 *) PcdGetPtr (PcdBaseBoardProductName);
  if (StrLen (BaseBoardProductName) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BASE_BOARD_PRODUCT_NAME);
    HiiSetString (mHiiHandle, TokenToUpdate, BaseBoardProductName, NULL);
  }

  Version = (CHAR16 *) PcdGetPtr (PcdBaseBoardVersion);
  if (StrLen (Version) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BASE_BOARD_VERSION);
    HiiSetString (mHiiHandle, TokenToUpdate, Version, NULL);
  }

  UpdateSmbiosInfo (mHiiHandle, STRING_TOKEN (STR_MISC_BASE_BOARD_ASSET_TAG), AssertTagType02);
  UpdateSmbiosInfo (mHiiHandle, STRING_TOKEN (STR_MISC_BASE_BOARD_SERIAL_NUMBER), SrNumType02);
  UpdateSmbiosInfo (mHiiHandle, STRING_TOKEN (STR_MISC_BASE_BOARD_MANUFACTURER), BoardManufacturerType02);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_MANUFACTURER);
  BaseBoardManufacturer = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen = StrLen (BaseBoardManufacturer);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_PRODUCT_NAME);
  BaseBoardProductName = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ProductNameStrLen = StrLen (BaseBoardProductName);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_VERSION);
  Version = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen = StrLen (Version);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_SERIAL_NUMBER);
  SerialNumber = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen (SerialNumber);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_ASSET_TAG);
  AssetTag = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  AssetTagStrLen = StrLen (AssetTag);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_CHASSIS_LOCATION);
  ChassisLocation = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ChassisLocaStrLen = StrLen (ChassisLocation);

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE2) + ManuStrLen        + 1
                                                                + ProductNameStrLen + 1
                                                                + VerStrLen         + 1
                                                                + SerialNumStrLen   + 1
                                                                + AssetTagStrLen    + 1
                                                                + ChassisLocaStrLen + 1 + 1);
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  (VOID)CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE2));
  SmbiosRecord->Hdr.Length        = sizeof (SMBIOS_TABLE_TYPE2);

  //
  //  Update Contained objects Handle
  //
  SmbiosRecord->NumberOfContainedObjectHandles = 0;
  GetLinkTypeHandle (EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE, &HandleArray, &HandleCount);
  if (HandleCount) {
    SmbiosRecord->ChassisHandle = HandleArray[0];
  }

  FreePool(HandleArray);

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStrS (BaseBoardManufacturer, OptionalStrStart, ManuStrLen + 1);
  UnicodeStrToAsciiStrS (BaseBoardProductName, OptionalStrStart + ManuStrLen + 1, ProductNameStrLen + 1);
  UnicodeStrToAsciiStrS (Version, OptionalStrStart + ManuStrLen + 1 + ProductNameStrLen + 1, VerStrLen + 1);
  UnicodeStrToAsciiStrS (SerialNumber, OptionalStrStart + ManuStrLen + 1 + ProductNameStrLen + 1 + VerStrLen + 1, SerialNumStrLen + 1);
  UnicodeStrToAsciiStrS (AssetTag, OptionalStrStart + ManuStrLen + 1 + ProductNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1, AssetTagStrLen + 1);
  UnicodeStrToAsciiStrS (ChassisLocation, OptionalStrStart + ManuStrLen + 1 + ProductNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + AssetTagStrLen + 1, ChassisLocaStrLen + 1);

  Status = LogSmbiosData ((UINT8*)SmbiosRecord, &SmbiosHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]:[%dL] Smbios Type02 Table Log Failed! %r \n", __FUNCTION__, __LINE__, Status));
  }

  FreePool (SmbiosRecord);

Exit:
  if (BaseBoardManufacturer != NULL) {
    FreePool (BaseBoardManufacturer);
  }

  if (BaseBoardProductName != NULL) {
    FreePool (BaseBoardProductName);
  }

  if (Version != NULL) {
    FreePool (Version);
  }

  if (SerialNumber != NULL) {
    FreePool (SerialNumber);
  }

  if (AssetTag != NULL) {
    FreePool (AssetTag);
  }

  if (ChassisLocation != NULL) {
    FreePool (ChassisLocation);
  }

  return 0;
}
