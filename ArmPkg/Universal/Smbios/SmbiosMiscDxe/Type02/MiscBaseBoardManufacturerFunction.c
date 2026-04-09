/** @file
  This driver parses the mSmbiosMiscDataTable structure and reports
  any generated data using SMBIOS protocol.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2022, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OemMiscLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "SmbiosMisc.h"

/**
  This function makes boot time changes to the contents of the
  MiscBaseBoardManufacturer (Type 2) record.

  @param  RecordData                 Pointer to SMBIOS table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS table was successfully added.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.
  @retval EFI_OUT_OF_RESOURCES       Failed to allocate required memory.

**/
SMBIOS_MISC_TABLE_FUNCTION (MiscBaseBoardManufacturer) {
  CHAR8               *OptionalStrStart;
  CHAR8               *StrStart;
  UINTN               RecordLength;
  UINTN               ManuStrLen;
  UINTN               ProductNameStrLen;
  UINTN               VerStrLen;
  UINTN               SerialNumStrLen;
  UINTN               AssetTagStrLen;
  UINTN               ChassisLocaStrLen;
  UINTN               HandleCount;
  UINT16              *HandleArray;
  CHAR16              *BaseBoardManufacturer;
  CHAR16              *BaseBoardProductName;
  CHAR16              *Version;
  EFI_STRING          SerialNumber;
  EFI_STRING          AssetTag;
  EFI_STRING          ChassisLocation;
  EFI_STRING_ID       TokenToGet;
  SMBIOS_TABLE_TYPE2  *SmbiosRecord;
  SMBIOS_TABLE_TYPE2  *InputData;
  EFI_STATUS          Status;

  EFI_STRING_ID  TokenToUpdate;

  HandleCount = 0;
  HandleArray = NULL;
  InputData   = NULL;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputData = (SMBIOS_TABLE_TYPE2 *)RecordData;

  BaseBoardManufacturer = (CHAR16 *)PcdGetPtr (PcdBaseBoardManufacturer);
  if (StrLen (BaseBoardManufacturer) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BASE_BOARD_MANUFACTURER);
    HiiSetString (mSmbiosMiscHiiHandle, TokenToUpdate, BaseBoardManufacturer, NULL);
  } else {
    OemUpdateSmbiosInfo (
      mSmbiosMiscHiiHandle,
      STRING_TOKEN (STR_MISC_BASE_BOARD_MANUFACTURER),
      BoardManufacturerType02
      );
  }

  BaseBoardProductName = (CHAR16 *)PcdGetPtr (PcdBaseBoardProductName);
  if (StrLen (BaseBoardProductName) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BASE_BOARD_PRODUCT_NAME);
    HiiSetString (mSmbiosMiscHiiHandle, TokenToUpdate, BaseBoardProductName, NULL);
  } else {
    OemUpdateSmbiosInfo (
      mSmbiosMiscHiiHandle,
      STRING_TOKEN (STR_MISC_BASE_BOARD_PRODUCT_NAME),
      ProductNameType02
      );
  }

  Version = (CHAR16 *)PcdGetPtr (PcdBaseBoardVersion);
  if (StrLen (Version) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_BASE_BOARD_VERSION);
    HiiSetString (mSmbiosMiscHiiHandle, TokenToUpdate, Version, NULL);
  } else {
    OemUpdateSmbiosInfo (
      mSmbiosMiscHiiHandle,
      STRING_TOKEN (STR_MISC_BASE_BOARD_VERSION),
      VersionType02
      );
  }

  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_BASE_BOARD_ASSET_TAG),
    AssetTagType02
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_BASE_BOARD_SERIAL_NUMBER),
    SerialNumberType02
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_BASE_BOARD_SKU_NUMBER),
    SerialNumberType02
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_BASE_BOARD_CHASSIS_LOCATION),
    ChassisLocationType02
    );

  TokenToGet            = STRING_TOKEN (STR_MISC_BASE_BOARD_MANUFACTURER);
  BaseBoardManufacturer = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen            = StrLen (BaseBoardManufacturer);

  TokenToGet           = STRING_TOKEN (STR_MISC_BASE_BOARD_PRODUCT_NAME);
  BaseBoardProductName = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ProductNameStrLen    = StrLen (BaseBoardProductName);

  TokenToGet = STRING_TOKEN (STR_MISC_BASE_BOARD_VERSION);
  Version    = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen  = StrLen (Version);

  TokenToGet      = STRING_TOKEN (STR_MISC_BASE_BOARD_SERIAL_NUMBER);
  SerialNumber    = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen (SerialNumber);

  TokenToGet     = STRING_TOKEN (STR_MISC_BASE_BOARD_ASSET_TAG);
  AssetTag       = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  AssetTagStrLen = StrLen (AssetTag);

  TokenToGet        = STRING_TOKEN (STR_MISC_BASE_BOARD_CHASSIS_LOCATION);
  ChassisLocation   = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ChassisLocaStrLen = StrLen (ChassisLocation);

  //
  // Two zeros following the last string.
  //
  RecordLength = sizeof (SMBIOS_TABLE_TYPE2) +
                 ManuStrLen        + 1 +
                 ProductNameStrLen + 1 +
                 VerStrLen         + 1 +
                 SerialNumStrLen   + 1 +
                 AssetTagStrLen    + 1 +
                 ChassisLocaStrLen + 1 + 1;
  SmbiosRecord = AllocateZeroPool (RecordLength);
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  (VOID)CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE2));
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE2);

  //
  //  Update Contained objects Handle
  //
  SmbiosRecord->NumberOfContainedObjectHandles = 0;
  SmbiosMiscGetLinkTypeHandle (
    EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE,
    &HandleArray,
    &HandleCount
    );
  // It's assumed there's at most a single chassis
  ASSERT (HandleCount < 2);
  if (HandleCount > 0) {
    SmbiosRecord->ChassisHandle = HandleArray[0];
  }

  FreePool (HandleArray);

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStrS (BaseBoardManufacturer, OptionalStrStart, ManuStrLen + 1);

  StrStart = OptionalStrStart + ManuStrLen + 1;
  UnicodeStrToAsciiStrS (BaseBoardProductName, StrStart, ProductNameStrLen + 1);

  StrStart += ProductNameStrLen + 1;
  UnicodeStrToAsciiStrS (Version, StrStart, VerStrLen + 1);

  StrStart += VerStrLen + 1;
  UnicodeStrToAsciiStrS (SerialNumber, StrStart, SerialNumStrLen + 1);

  StrStart += SerialNumStrLen + 1;
  UnicodeStrToAsciiStrS (AssetTag, StrStart, AssetTagStrLen + 1);

  StrStart += AssetTagStrLen + 1;
  UnicodeStrToAsciiStrS (ChassisLocation, StrStart, ChassisLocaStrLen + 1);

  Status = SmbiosMiscAddRecord ((UINT8 *)SmbiosRecord, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Smbios Type02 Table Log Failed! %r \n",
      __func__,
      DEBUG_LINE_NUMBER,
      Status
      ));
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
