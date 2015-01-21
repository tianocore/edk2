/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscPortInternalConnectorDesignatorFunction.c

Abstract:

  Create the device path for the Port internal connector designator.
  Port internal connector designator information is Misc. subclass type 6
  and SMBIOS type 8.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"


/**
  This is a macro defined function, in fact, the function is
  MiscPortInternalConnectorDesignatorFunction (RecordType, RecordLen, RecordData, LogRecordData)
  This function makes boot time changes to the contents of the
  MiscPortConnectorInformation.

  @param  MiscPortInternalConnectorDesignator  The string which is used to create
                                               the function
  The Arguments in fact:
  @param  RecordType                           Type of record to be processed from
                                               the Data Table.
                                               mMiscSubclassDataTable[].RecordType
  @param  RecordLen                            Size of static RecordData from the
                                               Data Table.
                                               mMiscSubclassDataTable[].RecordLen
  @param  RecordData                           Pointer to RecordData, which will be
                                               written to the Data Hub
  @param  LogRecordData                        TRUE to log RecordData to Data Hub.
                                               FALSE when there is no more data to
                                               log.

  @retval EFI_SUCCESS                          *RecordData and *LogRecordData have
                                               been set.
  @retval EFI_UNSUPPORTED                      Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER                One of the following parameter
                                               conditions was true: RecordLen was
                                               zero. RecordData was NULL.
                                               LogRecordData was NULL.

**/
MISC_SMBIOS_TABLE_FUNCTION (
  MiscPortInternalConnectorDesignator
  )
{
  CHAR8                                        *OptionalStrStart;
  UINTN                                        InternalRefStrLen;
  UINTN                                        ExternalRefStrLen;
  EFI_STRING                                   InternalRef;
  EFI_STRING                                   ExternalRef;
  STRING_REF                                   TokenForInternal;
  STRING_REF                                   TokenForExternal;
  EFI_STATUS                                   Status;
  SMBIOS_TABLE_TYPE8                           *SmbiosRecord;
  EFI_SMBIOS_HANDLE                            SmbiosHandle;
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR  *ForType8InputData;

  ForType8InputData = (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenForInternal = 0;
  TokenForExternal = 0;

  switch (ForType8InputData->PortInternalConnectorDesignator) {

    case STR_MISC_PORT_INTERNAL_IDE1:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_IDE1);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_IDE1);
      break;
    case STR_MISC_PORT_INTERNAL_IDE2:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_IDE2);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_IDE2);
      break;
    case STR_MISC_PORT_INTERNAL_ATX_POWER:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_ATX_POWER);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_ATX_POWER);
      break;
    default:
      break;
  }

  InternalRef = SmbiosMiscGetString (TokenForInternal);
  InternalRefStrLen = StrLen(InternalRef);
  if (InternalRefStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  ExternalRef = SmbiosMiscGetString (TokenForExternal);
  ExternalRefStrLen = StrLen(ExternalRef);
  if (ExternalRefStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE8) + InternalRefStrLen + 1 + ExternalRefStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE8) + InternalRefStrLen + 1 + ExternalRefStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE8);

  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;
  SmbiosRecord->InternalReferenceDesignator = 1;
  SmbiosRecord->InternalConnectorType = (UINT8)ForType8InputData->PortInternalConnectorType;
  SmbiosRecord->ExternalReferenceDesignator = 2;
  SmbiosRecord->ExternalConnectorType = (UINT8)ForType8InputData->PortExternalConnectorType;
  SmbiosRecord->PortType = (UINT8)ForType8InputData->PortType;

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(InternalRef, OptionalStrStart);
  UnicodeStrToAsciiStr(ExternalRef, OptionalStrStart + InternalRefStrLen + 1);

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
