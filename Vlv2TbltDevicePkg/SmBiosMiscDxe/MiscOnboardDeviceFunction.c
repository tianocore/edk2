/** @file

Copyright (c) 1999  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscOnboardDeviceFunction.c

Abstract:

  Create the device path for the Onboard device.
  The Onboard device information is Misc. subclass type 8 and SMBIOS type 10.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"



/**
  This is a macro defined function, in fact, the function is
  MiscOnboardDeviceFunction (RecordType, RecordLen, RecordData, LogRecordData)
  This function makes boot time changes to the contents of the
  MiscOnboardDevice structure.

  @param  MiscOnboardDevice      The string which is used to create the function
  The Arguments in fact:
  @param  RecordType             Type of record to be processed from the Data
                                 Table. mMiscSubclassDataTable[].RecordType
  @param  RecordLen              Size of static RecordData from the Data Table.
                                 mMiscSubclassDataTable[].RecordLen
  @param  RecordData             Pointer to RecordData, which will be written to
                                 the Data Hub
  @param  LogRecordData          TRUE to log RecordData to Data Hub. FALSE when
                                 there is no more data to log.

  @retval EFI_SUCCESS            *RecordData and *LogRecordData have been set.
  @retval EFI_UNSUPPORTED        Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER  One of the following parameter conditions was
                                 true: RecordLen was zero. RecordData was NULL.
                                 LogRecordData was NULL.

**/
MISC_SMBIOS_TABLE_FUNCTION (
  MiscOnboardDevice
  )
{
  CHAR8                         *OptionalStrStart;
  UINT8                         StatusAndType;
  UINTN                         DescriptionStrLen;
  EFI_STRING                    DeviceDescription;
  STRING_REF                    TokenToGet;
  EFI_STATUS                    Status;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  SMBIOS_TABLE_TYPE10           *SmbiosRecord;
  EFI_MISC_ONBOARD_DEVICE       *ForType10InputData;

  ForType10InputData = (EFI_MISC_ONBOARD_DEVICE *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenToGet = 0;
  switch (ForType10InputData->OnBoardDeviceDescription) {
    case STR_MISC_ONBOARD_DEVICE_VIDEO:
      TokenToGet = STRING_TOKEN (STR_MISC_ONBOARD_DEVICE_VIDEO);
      break;
    case STR_MISC_ONBOARD_DEVICE_AUDIO:
      TokenToGet = STRING_TOKEN (STR_MISC_ONBOARD_DEVICE_AUDIO);
      break;
	default:
	break;
  }

  DeviceDescription = SmbiosMiscGetString (TokenToGet);
  DescriptionStrLen = StrLen(DeviceDescription);
  if (DescriptionStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE10) + DescriptionStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE10) + DescriptionStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE10);

  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;

  //
  // Status & Type: Bit 7 Devicen Status, Bits 6:0 Type of Device
  //
  StatusAndType = (UINT8) ForType10InputData->OnBoardDeviceStatus.DeviceType;
  if (ForType10InputData->OnBoardDeviceStatus.DeviceEnabled != 0) {
    StatusAndType |= 0x80;
  } else {
    StatusAndType &= 0x7F;
  }

  SmbiosRecord->Device[0].DeviceType = StatusAndType;
  SmbiosRecord->Device[0].DescriptionString = 1;
  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(DeviceDescription, OptionalStrStart);

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
