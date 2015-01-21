/*++

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscChassisManufacturerFunction.c

Abstract:

  Chassis manufacturer information boot time changes.
  SMBIOS type 3.

--*/


#include "CommonHeader.h"

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
  UINTN                           AssertTagStrLen;
  UINTN                           SerialNumStrLen;
  EFI_STATUS                      Status;
  EFI_STRING                      Manufacturer;
  EFI_STRING                      Version;
  EFI_STRING                      SerialNumber;
  EFI_STRING                      AssertTag;
  STRING_REF                      TokenToGet;
  EFI_SMBIOS_HANDLE               SmbiosHandle;
  SMBIOS_TABLE_TYPE3              *SmbiosRecord;
  EFI_MISC_CHASSIS_MANUFACTURER   *ForType3InputData;

  ForType3InputData = (EFI_MISC_CHASSIS_MANUFACTURER *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_MANUFACTURER);
  Manufacturer = SmbiosMiscGetString (TokenToGet);
  ManuStrLen = StrLen(Manufacturer);
  if (ManuStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_VERSION);
  Version = SmbiosMiscGetString (TokenToGet);
  VerStrLen = StrLen(Version);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_SERIAL_NUMBER);
  SerialNumber = SmbiosMiscGetString (TokenToGet);
  SerialNumStrLen = StrLen(SerialNumber);
  if (SerialNumStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_CHASSIS_ASSET_TAG);
  AssertTag = SmbiosMiscGetString (TokenToGet);
  AssertTagStrLen = StrLen(AssertTag);
  if (AssertTagStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE3) + ManuStrLen + 1  + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE3) + ManuStrLen + 1  + VerStrLen + 1 + SerialNumStrLen + 1 + AssertTagStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_ENCLOSURE;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE3);

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
  // AssertTag will be the 4th optional string following the formatted structure.
  //
  SmbiosRecord->AssetTag = 4;
  SmbiosRecord->BootupState = (UINT8)ForType3InputData->ChassisBootupState;
  SmbiosRecord->PowerSupplyState = (UINT8)ForType3InputData->ChassisPowerSupplyState;
  SmbiosRecord->ThermalState = (UINT8)ForType3InputData->ChassisThermalState;
  SmbiosRecord->SecurityStatus = (UINT8)ForType3InputData->ChassisSecurityState;
  CopyMem (SmbiosRecord->OemDefined,(UINT8*)&ForType3InputData->ChassisOemDefined, 4);

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(Manufacturer, OptionalStrStart);
  UnicodeStrToAsciiStr(Version, OptionalStrStart + ManuStrLen + 1);
  UnicodeStrToAsciiStr(SerialNumber, OptionalStrStart + ManuStrLen + 1 + VerStrLen + 1);
  UnicodeStrToAsciiStr(AssertTag, OptionalStrStart + ManuStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1);

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
