/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data to smbios.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"

/**
  This function makes boot time changes to the contents of the
  MiscSystemManufacturer (Type 1).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscSystemManufacturer)
{
  CHAR8                             *OptionalStrStart;
  UINTN                             ManuStrLen;
  UINTN                             VerStrLen;
  UINTN                             PdNameStrLen;
  UINTN                             SerialNumStrLen;
  UINTN                             SKUNumStrLen;
  UINTN                             FamilyStrLen;
  EFI_STATUS                        Status;
  CHAR16                            Manufacturer[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                            ProductName[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                            Version[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                            SerialNumber[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                            SKUNumber[SMBIOS_STRING_MAX_LENGTH];
  CHAR16                            Family[SMBIOS_STRING_MAX_LENGTH];
  EFI_STRING                        ManufacturerPtr;
  EFI_STRING                        ProductNamePtr;
  EFI_STRING                        VersionPtr;
  EFI_STRING                        SerialNumberPtr;
  EFI_STRING                        SKUNumberPtr;
  EFI_STRING                        FamilyPtr;
  STRING_REF                        TokenToGet;
  STRING_REF                        TokenToUpdate;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  SMBIOS_TABLE_TYPE1                *SmbiosRecord;
  EFI_MISC_SYSTEM_MANUFACTURER      *ForType1InputData;

  ForType1InputData = (EFI_MISC_SYSTEM_MANUFACTURER *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Update strings from PCD
  //
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemManufacturer), Manufacturer);
  if (StrLen (Manufacturer) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_MANUFACTURER);
    HiiSetString (mHiiHandle, TokenToUpdate, Manufacturer, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_MANUFACTURER);
  ManufacturerPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen = StrLen(ManufacturerPtr);
  if (ManuStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemProductName), ProductName);
  if (StrLen (ProductName) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_PRODUCT_NAME);
    HiiSetString (mHiiHandle, TokenToUpdate, ProductName, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_PRODUCT_NAME);
  ProductNamePtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  PdNameStrLen = StrLen(ProductNamePtr);
  if (PdNameStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemVersion), Version);
  if (StrLen (Version) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_VERSION);
    HiiSetString (mHiiHandle, TokenToUpdate, Version, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_VERSION);
  VersionPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen = StrLen(VersionPtr);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSerialNumber), SerialNumber);
  if (StrLen (SerialNumber) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_SERIAL_NUMBER);
    HiiSetString (mHiiHandle, TokenToUpdate, SerialNumber, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_SERIAL_NUMBER);
  SerialNumberPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen(SerialNumberPtr);
  if (SerialNumStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemSKUNumber), SKUNumber);
  if (StrLen (SKUNumber) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_SKU_NUMBER);
    HiiSetString (mHiiHandle, TokenToUpdate, SKUNumber, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_SKU_NUMBER);
  SKUNumberPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  SKUNumStrLen = StrLen(SKUNumberPtr);
  if (SKUNumStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSSystemFamily), Family);
  if (StrLen (Family) > 0) {
    TokenToUpdate = STRING_TOKEN (STR_MISC_SYSTEM_FAMILY);
    HiiSetString (mHiiHandle, TokenToUpdate, Family, NULL);
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_FAMILY);
  FamilyPtr = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  FamilyStrLen = StrLen(FamilyPtr);
  if (FamilyStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }
  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE1) + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + SKUNumStrLen + 1 + FamilyStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE1) + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + SKUNumStrLen + 1 + FamilyStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE1);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;
  //
  // Manu will be the 1st optional string following the formatted structure.
  //
  SmbiosRecord->Manufacturer = 1;
  //
  // ProductName will be the 2nd optional string following the formatted structure.
  //
  SmbiosRecord->ProductName = 2;
  //
  // Version will be the 3rd optional string following the formatted structure.
  //
  SmbiosRecord->Version = 3;
  //
  // Serial number will be the 4th optional string following the formatted structure.
  //
  SmbiosRecord->SerialNumber = 4;
  //
  // SKU number will be the 5th optional string following the formatted structure.
  //
  SmbiosRecord->SKUNumber = 5;
  //
  // Family will be the 6th optional string following the formatted structure.
  //
  SmbiosRecord->Family = 6;
  CopyMem ((UINT8 *) (&SmbiosRecord->Uuid), (UINT8 *)PcdGetPtr(PcdSMBIOSSystemUuid),16);
  SmbiosRecord->WakeUpType = (UINT8)ForType1InputData->SystemWakeupType;

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(ManufacturerPtr, OptionalStrStart);
  UnicodeStrToAsciiStr(ProductNamePtr, OptionalStrStart + ManuStrLen + 1);
  UnicodeStrToAsciiStr(VersionPtr, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1);
  UnicodeStrToAsciiStr(SerialNumberPtr, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1);
  UnicodeStrToAsciiStr(SKUNumberPtr, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen+ 1);
  UnicodeStrToAsciiStr(FamilyPtr, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + SKUNumStrLen+ 1);

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios-> Add(
                      Smbios,
                      NULL,
                      &SmbiosHandle,
                      (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                      );
  FreePool(SmbiosRecord);
  return Status;
}
