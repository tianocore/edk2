/*++

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  MiscPortInternalConnectorDesignatorFunction.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"


MISC_SMBIOS_TABLE_FUNCTION (
  MiscPortInternalConnectorDesignator
  )
/*++
Description:

  This function makes boot time changes to the contents of the
  MiscPortConnectorInformation (Type 8).

Parameters:

  RecordType
    Type of record to be processed from the Data Table.
    mMiscSubclassDataTable[].RecordType

  RecordLen
    Size of static RecordData from the Data Table.
    mMiscSubclassDataTable[].RecordLen

  RecordData
    Pointer to copy of RecordData from the Data Table.  Changes made
    to this copy will be written to the Data Hub but will not alter
    the contents of the static Data Table.

  LogRecordData
    Set *LogRecordData to TRUE to log RecordData to Data Hub.
    Set *LogRecordData to FALSE when there is no more data to log.

Returns:

  EFI_SUCCESS
    All parameters were valid and *RecordData and *LogRecordData have
    been set.

  EFI_UNSUPPORTED
    Unexpected RecordType value.

  EFI_INVALID_PARAMETER
    One of the following parameter conditions was true:
      RecordLen was zero.
      RecordData was NULL.
      LogRecordData was NULL.
**/
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

    case STR_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_CONNECTOR_DESIGNATOR);
      break;
    case STR_MISC_PORT_INTERNAL_KEYBOARD:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_KEYBOARD);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_KEYBOARD);
      break;
    case STR_MISC_PORT_INTERNAL_MOUSE:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_MOUSE);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_MOUSE);
      break;
    case STR_MISC_PORT_INTERNAL_COM1:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_COM1);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_COM1);
      break;
    case STR_MISC_PORT_INTERNAL_COM2:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_COM2);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_COM2);
      break;
    case STR_MISC_PORT_INTERNAL_EXTENSION_POWER:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_EXTENSION_POWER);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_EXTENSION_POWER);
      break;
    case STR_MISC_PORT_INTERNAL_FLOPPY:
      TokenForInternal = STRING_TOKEN (STR_MISC_PORT_INTERNAL_FLOPPY);
      TokenForExternal = STRING_TOKEN(STR_MISC_PORT_EXTERNAL_FLOPPY);
      break;
    default:
      break;
  }

  InternalRef = HiiGetPackageString(&gEfiCallerIdGuid, TokenForInternal, NULL);
  InternalRefStrLen = StrLen(InternalRef);
  if (InternalRefStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  ExternalRef = HiiGetPackageString(&gEfiCallerIdGuid, TokenForExternal, NULL);
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
  Status = AddSmbiosRecord (Smbios, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord);

  FreePool(SmbiosRecord);
  return Status;
}



/* eof - MiscSystemManufacturerFunction.c */
