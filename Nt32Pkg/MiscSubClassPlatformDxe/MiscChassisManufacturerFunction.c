/** @file
  Chassis manufacturer information boot time changes.
  SMBIOS type 3.
  
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2017 Hewlett Packard Enterprise Development LP<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "MiscSubclassDriver.h"

/**
  This function makes boot time changes to the contents of the
  MiscChassisManufacturer (Type 3).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.  

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscChassisManufacturer)
{
  CHAR8                           *OptionalStrStart;
  UINTN                           ManuStrLen;
  UINTN                           VerStrLen;
  UINTN                           AssetTagStrLen;
  UINTN                           SerialNumStrLen;
  UINTN                           SkuNumberStrLen;
  EFI_STATUS                      Status;
  EFI_STRING                      Manufacturer;
  EFI_STRING                      Version;
  EFI_STRING                      SerialNumber;
  EFI_STRING                      AssetTag;
  EFI_STRING                      SkuNumber;
  STRING_REF                      TokenToGet;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  SMBIOS_TABLE_TYPE3              *SmbiosRecord;
  EFI_MISC_CHASSIS_MANUFACTURER   *ForType3InputData;
  UINT8                           *Buffer;

  ForType3InputData = (EFI_MISC_CHASSIS_MANUFACTURER *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_MANUFACTURER);
  Manufacturer = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  ManuStrLen = StrLen(Manufacturer);
  if (ManuStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_VERSION);
  Version = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  VerStrLen = StrLen(Version);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_SERIAL_NUMBER);
  SerialNumber = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  SerialNumStrLen = StrLen(SerialNumber);
  if (SerialNumStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_ASSET_TAG);
  AssetTag = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  AssetTagStrLen = StrLen(AssetTag);
  if (AssetTagStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_SKU_NUMBER);
  SkuNumber = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  SkuNumberStrLen = StrLen(SkuNumber);
  if (SkuNumberStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  // Since we set ContainedElementCount = 0 and ContainedElementRecordLength = 0,
  // remove sizeof (CONTAINED_ELEMENT) for ContainedElements[1].
  //
  // Add sizeof (SMBIOS_TABLE_STRING) for SKU Number, since not contained in SMBIOS_TABLE_TYPE3.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE3) - sizeof (CONTAINED_ELEMENT) + sizeof (SMBIOS_TABLE_STRING) + ManuStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + AssetTagStrLen + 1 + SkuNumberStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE3) - sizeof (CONTAINED_ELEMENT) + sizeof (SMBIOS_TABLE_STRING) + ManuStrLen + 1  + VerStrLen + 1 + SerialNumStrLen + 1 + AssetTagStrLen + 1 + SkuNumberStrLen + 1 + 1);

  Buffer = (UINT8 *) SmbiosRecord;

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE;
  //
  // Since we set ContainedElementCount = 0 and ContainedElementRecordLength = 0,
  // remove sizeof (CONTAINED_ELEMENT) for ContainedElements[1].
  //
  // Add sizeof (SMBIOS_TABLE_STRING) for SKU Number, since not contained in SMBIOS_TABLE_TYPE3.
  //
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE3) - sizeof (CONTAINED_ELEMENT) + sizeof (SMBIOS_TABLE_STRING);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;  
  //
  // Manu will be the 1st optional string following the formatted structure.
  // 
  SmbiosRecord->Manufacturer = 1;  
  SmbiosRecord->Type = (UINT8)ForType3InputData->ChassisType.ChassisType;
  //
  // Version will be the 2nd optional string following the formatted structure.
  //
  SmbiosRecord->Version = 2;  
  //
  // SerialNumber will be the 3rd optional string following the formatted structure.
  //
  SmbiosRecord->SerialNumber = 3;  
  //
  // AssetTag will be the 4th optional string following the formatted structure.
  //
  SmbiosRecord->AssetTag = 4;

  SmbiosRecord->BootupState = (UINT8)ForType3InputData->ChassisBootupState;
  SmbiosRecord->PowerSupplyState = (UINT8)ForType3InputData->ChassisPowerSupplyState;
  SmbiosRecord->ThermalState = (UINT8)ForType3InputData->ChassisThermalState;
  SmbiosRecord->SecurityStatus = (UINT8)ForType3InputData->ChassisSecurityState;
  CopyMem (SmbiosRecord->OemDefined,(UINT8*)&ForType3InputData->ChassisOemDefined, 4);
  SmbiosRecord->Height = (UINT8)ForType3InputData->ChassisHeight;
  SmbiosRecord->NumberofPowerCords = (UINT8)ForType3InputData->ChassisNumberPowerCords;
  SmbiosRecord->ContainedElementCount = 0;
  SmbiosRecord->ContainedElementRecordLength = 0;

  //
  // SKU Number will be the 5th optional string following the formatted structure.
  //
  // Since SKU Number is not in SMBIOS_TABLE_TYPE3 structure, must locate it after ContainedElementRecordLength.
  //
  Buffer[sizeof (SMBIOS_TABLE_TYPE3) - sizeof (CONTAINED_ELEMENT)] = 5;

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  //
  // Since we set ContainedElementCount = 0 and ContainedElementRecordLength = 0,
  // remove sizeof (CONTAINED_ELEMENT) for ContainedElements[1].
  //
  OptionalStrStart -= sizeof (CONTAINED_ELEMENT);
  //
  // Add sizeof (SMBIOS_TABLE_STRING) for SKU Number, since not contained in SMBIOS_TABLE_TYPE3.
  //
  OptionalStrStart += sizeof (SMBIOS_TABLE_STRING);
  UnicodeStrToAsciiStr (Manufacturer, OptionalStrStart);
  UnicodeStrToAsciiStr (Version, OptionalStrStart + ManuStrLen + 1);
  UnicodeStrToAsciiStr (SerialNumber, OptionalStrStart + ManuStrLen + 1 + VerStrLen + 1);
  UnicodeStrToAsciiStr (AssetTag, OptionalStrStart + ManuStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1);
  UnicodeStrToAsciiStr (SkuNumber, OptionalStrStart + ManuStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + AssetTagStrLen + 1);

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}
