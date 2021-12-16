/** @file
  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to smbios.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
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
  MiscSystemManufacturer (Type 1) record.

  @param  RecordData                 Pointer to SMBIOS table with default values.
  @param  Smbios                     SMBIOS protocol.

  @retval EFI_SUCCESS                The SMBIOS table was successfully added.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.
  @retval EFI_OUT_OF_RESOURCES       Failed to allocate required memory.

**/
SMBIOS_MISC_TABLE_FUNCTION (MiscSystemManufacturer) {
  CHAR8               *OptionalStrStart;
  CHAR8               *StrStart;
  UINTN               ManuStrLen;
  UINTN               VerStrLen;
  UINTN               PdNameStrLen;
  UINTN               SerialNumStrLen;
  UINTN               SKUNumStrLen;
  UINTN               FamilyStrLen;
  UINTN               RecordLength;
  EFI_STRING          Manufacturer;
  EFI_STRING          ProductName;
  EFI_STRING          Version;
  EFI_STRING          SerialNumber;
  EFI_STRING          SKUNumber;
  EFI_STRING          Family;
  EFI_STRING_ID       TokenToGet;
  SMBIOS_TABLE_TYPE1  *SmbiosRecord;
  SMBIOS_TABLE_TYPE1  *InputData;
  EFI_STATUS          Status;
  EFI_STRING_ID       TokenToUpdate;
  CHAR16              *Product;
  CHAR16              *pVersion;

  Status = EFI_SUCCESS;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputData = (SMBIOS_TABLE_TYPE1 *)RecordData;

  Product = (CHAR16 *)PcdGetPtr (PcdSystemProductName);
  if (StrLen (Product) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_PRODUCT_NAME);
    HiiSetString (mSmbiosMiscHiiHandle, TokenToUpdate, Product, NULL);
  } else {
    OemUpdateSmbiosInfo (
      mSmbiosMiscHiiHandle,
      STRING_TOKEN (STR_MISC_SYSTEM_PRODUCT_NAME),
      ProductNameType01
      );
  }

  pVersion = (CHAR16 *)PcdGetPtr (PcdSystemVersion);
  if (StrLen (pVersion) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_VERSION);
    HiiSetString (mSmbiosMiscHiiHandle, TokenToUpdate, pVersion, NULL);
  } else {
    OemUpdateSmbiosInfo (
      mSmbiosMiscHiiHandle,
      STRING_TOKEN (STR_MISC_SYSTEM_VERSION),
      VersionType01
      );
  }

  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_SYSTEM_SERIAL_NUMBER),
    SerialNumType01
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_SYSTEM_MANUFACTURER),
    SystemManufacturerType01
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_SYSTEM_SKU_NUMBER),
    SkuNumberType01
    );
  OemUpdateSmbiosInfo (
    mSmbiosMiscHiiHandle,
    STRING_TOKEN (STR_MISC_SYSTEM_FAMILY),
    FamilyType01
    );

  TokenToGet   = STRING_TOKEN (STR_MISC_SYSTEM_MANUFACTURER);
  Manufacturer = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen   = StrLen (Manufacturer);

  TokenToGet   = STRING_TOKEN (STR_MISC_SYSTEM_PRODUCT_NAME);
  ProductName  = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  PdNameStrLen = StrLen (ProductName);

  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_VERSION);
  Version    = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen  = StrLen (Version);

  TokenToGet      = STRING_TOKEN (STR_MISC_SYSTEM_SERIAL_NUMBER);
  SerialNumber    = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen (SerialNumber);

  TokenToGet   = STRING_TOKEN (STR_MISC_SYSTEM_SKU_NUMBER);
  SKUNumber    = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  SKUNumStrLen = StrLen (SKUNumber);

  TokenToGet   = STRING_TOKEN (STR_MISC_SYSTEM_FAMILY);
  Family       = HiiGetPackageString (&gEfiCallerIdGuid, TokenToGet, NULL);
  FamilyStrLen = StrLen (Family);

  //
  // Two zeros following the last string.
  //
  RecordLength = sizeof (SMBIOS_TABLE_TYPE1) +
                 ManuStrLen      + 1 +
                 PdNameStrLen    + 1 +
                 VerStrLen       + 1 +
                 SerialNumStrLen + 1 +
                 SKUNumStrLen    + 1 +
                 FamilyStrLen    + 1 + 1;
  SmbiosRecord = AllocateZeroPool (RecordLength);

  if (SmbiosRecord == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  (VOID)CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE1));

  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE1);

  CopyGuid (&SmbiosRecord->Uuid, &InputData->Uuid);

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStrS (Manufacturer, OptionalStrStart, ManuStrLen + 1);
  StrStart = OptionalStrStart + ManuStrLen + 1;
  UnicodeStrToAsciiStrS (ProductName, StrStart, PdNameStrLen + 1);
  StrStart += PdNameStrLen + 1;
  UnicodeStrToAsciiStrS (Version, StrStart, VerStrLen + 1);
  StrStart += VerStrLen + 1;
  UnicodeStrToAsciiStrS (SerialNumber, StrStart, SerialNumStrLen + 1);
  StrStart += SerialNumStrLen + 1;
  UnicodeStrToAsciiStrS (SKUNumber, StrStart, SKUNumStrLen + 1);
  StrStart += SKUNumStrLen + 1;
  UnicodeStrToAsciiStrS (Family, StrStart, FamilyStrLen + 1);

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = SmbiosMiscAddRecord ((UINT8 *)SmbiosRecord, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "[%a]:[%dL] Smbios Type01 Table Log Failed! %r \n",
      __FUNCTION__,
      DEBUG_LINE_NUMBER,
      Status
      ));
  }

  FreePool (SmbiosRecord);

Exit:
  if (Manufacturer != NULL) {
    FreePool (Manufacturer);
  }

  if (ProductName != NULL) {
    FreePool (ProductName);
  }

  if (Version != NULL) {
    FreePool (Version);
  }

  if (SerialNumber != NULL) {
    FreePool (SerialNumber);
  }

  if (SKUNumber != NULL) {
    FreePool (SKUNumber);
  }

  if (Family != NULL) {
    FreePool (Family);
  }

  return Status;
}
