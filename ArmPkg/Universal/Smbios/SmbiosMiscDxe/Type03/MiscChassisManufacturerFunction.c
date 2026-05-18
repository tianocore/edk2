/** @file
  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to smbios.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

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
  MiscChassisManufacturer (Type 3) record.

  @param  RecordData                 Pointer to SMBIOS table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS table was successfully added.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.
  @retval EFI_OUT_OF_RESOURCES       Failed to allocate required memory.

**/
SMBIOS_MISC_TABLE_FUNCTION (MiscChassisManufacturer) {
  CHAR8                *StrStart;
  SMBIOS_TABLE_STRING  *SkuNumberField;
  UINTN                RecordLength;
  UINTN                ManuStrLen;
  UINTN                VerStrLen;
  UINTN                AssertTagStrLen;
  UINTN                SerialNumStrLen;
  UINTN                ChaNumStrLen;
  UINTN                BaseSize;
  UINTN                ExtendLength;
  UINTN                HdrLength;
  EFI_STRING           Manufacturer;
  EFI_STRING           Version;
  EFI_STRING           SerialNumber;
  EFI_STRING           AssertTag;
  EFI_STRING           ChassisSkuNumber;
  EFI_STRING_ID        TokenToGet;
  SMBIOS_TABLE_TYPE3   *SmbiosRecord;
  SMBIOS_TABLE_TYPE3   *InputData;
  EFI_STATUS           Status;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputData = (SMBIOS_TABLE_TYPE3 *)RecordData;

  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_CHASSIS_ASSET_TAG),
    AssetTagType03
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_CHASSIS_SERIAL_NUMBER),
    SerialNumberType03
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_CHASSIS_VERSION),
    VersionType03
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_CHASSIS_MANUFACTURER),
    ManufacturerType03
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_CHASSIS_SKU_NUMBER),
    SkuNumberType03
    );

  TokenToGet   = STRING_TOKEN (STR_MISC_CHASSIS_MANUFACTURER);
  Manufacturer = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen   = StrLen (Manufacturer);

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_VERSION);
  Version    = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen  = StrLen (Version);

  TokenToGet      = STRING_TOKEN (STR_MISC_CHASSIS_SERIAL_NUMBER);
  SerialNumber    = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen (SerialNumber);

  TokenToGet      = STRING_TOKEN (STR_MISC_CHASSIS_ASSET_TAG);
  AssertTag       = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  AssertTagStrLen = StrLen (AssertTag);

  TokenToGet       = STRING_TOKEN (STR_MISC_CHASSIS_SKU_NUMBER);
  ChassisSkuNumber = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ChaNumStrLen     = StrLen (ChassisSkuNumber);

  STATIC_ASSERT (OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements) == 0x15, "Base size of SMBIOS_TABLE_TYPE3 does not meet SMBIOS specification");

  BaseSize     = OFFSET_OF (SMBIOS_TABLE_TYPE3, ContainedElements);
  ExtendLength = (UINTN)InputData->ContainedElementCount * (UINTN)InputData->ContainedElementRecordLength;

  //
  // Length of SMBIOS struct includes ContainedElements and SKUNumber.
  //
  HdrLength = BaseSize + ExtendLength + sizeof (SMBIOS_TABLE_STRING);
  if (HdrLength > MAX_UINT8) {
    ASSERT (HdrLength <= MAX_UINT8);
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Additional zero follows the last string.
  //
  RecordLength = HdrLength       +
                 ManuStrLen      + 1 +
                 VerStrLen       + 1 +
                 SerialNumStrLen + 1 +
                 AssertTagStrLen + 1 +
                 ChaNumStrLen    + 1 + 1;
  SmbiosRecord = AllocatePool (RecordLength);
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  // Copy base record plus ContainedElements.
  (VOID)CopyMem (SmbiosRecord, InputData, BaseSize + ExtendLength);

  SmbiosRecord->Hdr.Length = HdrLength;

  SmbiosRecord->Type = OemGetChassisType ();

  // ChassisSkuNumber
  SkuNumberField = (SMBIOS_TABLE_STRING *)((UINT8 *)SmbiosRecord + BaseSize + ExtendLength);

  // The string numbers in the fixed position portion of the record are populated in the input data.
  *SkuNumberField = 5;

  StrStart = (CHAR8 *)((UINT8 *)SkuNumberField + sizeof (SMBIOS_TABLE_STRING));
  UnicodeStrToAsciiStrS (Manufacturer, StrStart, ManuStrLen + 1);
  StrStart += ManuStrLen + 1;
  UnicodeStrToAsciiStrS (Version, StrStart, VerStrLen + 1);
  StrStart += VerStrLen + 1;
  UnicodeStrToAsciiStrS (SerialNumber, StrStart, SerialNumStrLen + 1);
  StrStart += SerialNumStrLen + 1;
  UnicodeStrToAsciiStrS (AssertTag, StrStart, AssertTagStrLen + 1);
  StrStart += AssertTagStrLen + 1;
  UnicodeStrToAsciiStrS (ChassisSkuNumber, StrStart, ChaNumStrLen + 1);
  StrStart   += ChaNumStrLen + 1;
  *StrStart++ = '\0';

  ASSERT ((UINT8 *)StrStart - (UINT8 *)SmbiosRecord == RecordLength);

  SmbiosRecord->BootupState        = OemGetChassisBootupState ();
  SmbiosRecord->PowerSupplyState   = OemGetChassisPowerSupplyState ();
  SmbiosRecord->ThermalState       = OemGetChassisThermalState ();
  SmbiosRecord->SecurityStatus     = OemGetChassisSecurityStatus ();
  SmbiosRecord->Height             = OemGetChassisHeight ();
  SmbiosRecord->NumberofPowerCords = OemGetChassisNumPowerCords ();

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = SmbiosMiscAddRecord ((UINT8 *)SmbiosRecord, NULL);

  FreePool (SmbiosRecord);

Exit:
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Smbios Type03 Table Log Failed! %r \n",
      __func__,
      DEBUG_LINE_NUMBER,
      Status
      ));
  }

  if (Manufacturer != NULL) {
    FreePool (Manufacturer);
  }

  if (Version != NULL) {
    FreePool (Version);
  }

  if (SerialNumber != NULL) {
    FreePool (SerialNumber);
  }

  if (AssertTag != NULL) {
    FreePool (AssertTag);
  }

  if (ChassisSkuNumber != NULL) {
    FreePool (ChassisSkuNumber);
  }

  return 0;
}
