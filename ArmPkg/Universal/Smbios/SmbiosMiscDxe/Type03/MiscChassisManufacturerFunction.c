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
  CHAR8               *OptionalStrStart;
  CHAR8               *StrStart;
  UINT8               *SkuNumberField;
  UINTN               RecordLength;
  UINTN               ManuStrLen;
  UINTN               VerStrLen;
  UINTN               AssertTagStrLen;
  UINTN               SerialNumStrLen;
  UINTN               ChaNumStrLen;
  EFI_STRING          Manufacturer;
  EFI_STRING          Version;
  EFI_STRING          SerialNumber;
  EFI_STRING          AssertTag;
  EFI_STRING          ChassisSkuNumber;
  EFI_STRING_ID       TokenToGet;
  SMBIOS_TABLE_TYPE3  *SmbiosRecord;
  SMBIOS_TABLE_TYPE3  *InputData;
  EFI_STATUS          Status;

  UINT8              ContainedElementCount;
  CONTAINED_ELEMENT  ContainedElements;
  UINT8              ExtendLength;

  ExtendLength = 0;

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

  ContainedElementCount = InputData->ContainedElementCount;
  ExtendLength          = ContainedElementCount * sizeof (CONTAINED_ELEMENT);

  //
  // Two zeros following the last string.
  //
  RecordLength = sizeof (SMBIOS_TABLE_TYPE3) +
                 ExtendLength    + 1 +
                 ManuStrLen      + 1 +
                 VerStrLen       + 1 +
                 SerialNumStrLen + 1 +
                 AssertTagStrLen + 1 +
                 ChaNumStrLen    + 1 + 1;
  SmbiosRecord = AllocateZeroPool (RecordLength);
  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  (VOID)CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE3));

  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE3) + ExtendLength + 1;

  SmbiosRecord->Type = OemGetChassisType ();

  // ContainedElements
  ASSERT (ContainedElementCount < 2);
  (VOID)CopyMem (SmbiosRecord + 1, &ContainedElements, ExtendLength);

  // ChassisSkuNumber
  SkuNumberField = (UINT8 *)SmbiosRecord +
                   sizeof (SMBIOS_TABLE_TYPE3) -
                   sizeof (CONTAINED_ELEMENT) + ExtendLength;

  *SkuNumberField = 5;

  OptionalStrStart = (CHAR8 *)((UINT8 *)SmbiosRecord + sizeof (SMBIOS_TABLE_TYPE3) +
                               ExtendLength + 1);
  UnicodeStrToAsciiStrS (Manufacturer, OptionalStrStart, ManuStrLen + 1);
  StrStart = OptionalStrStart + ManuStrLen + 1;
  UnicodeStrToAsciiStrS (Version, StrStart, VerStrLen + 1);
  StrStart += VerStrLen + 1;
  UnicodeStrToAsciiStrS (SerialNumber, StrStart, SerialNumStrLen + 1);
  StrStart += SerialNumStrLen + 1;
  UnicodeStrToAsciiStrS (AssertTag, StrStart, AssertTagStrLen + 1);
  StrStart += AssertTagStrLen + 1;
  UnicodeStrToAsciiStrS (ChassisSkuNumber, StrStart, ChaNumStrLen + 1);

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
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Smbios Type03 Table Log Failed! %r \n",
      __func__,
      DEBUG_LINE_NUMBER,
      Status
      ));
  }

  FreePool (SmbiosRecord);

Exit:
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
