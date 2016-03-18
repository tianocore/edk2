/** @file
Port internal connector designator information boot time changes.
SMBIOS type 8.

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

//STATIC PS2_CONN_DEVICE_PATH       mPs2KeyboardDevicePath   = { DP_ACPI, DP_PCI( 0x1F,0x00 ),DP_LPC( 0x0303,0 ), DP_END };
//STATIC PS2_CONN_DEVICE_PATH       mPs2MouseDevicePath      = { DP_ACPI, DP_PCI( 0x1F,0x00 ),DP_LPC( 0x0303,1 ), DP_END };
//STATIC SERIAL_CONN_DEVICE_PATH    mCom1DevicePath          = { DP_ACPI, DP_PCI( 0x1F,0x00 ),DP_LPC( 0x0501,0 ), DP_END };
//STATIC SERIAL_CONN_DEVICE_PATH    mCom2DevicePath          = { DP_ACPI, DP_PCI( 0x1F,0x00 ),DP_LPC( 0x0501,1 ), DP_END };
//STATIC PARALLEL_CONN_DEVICE_PATH  mLpt1DevicePath          = { DP_ACPI, DP_PCI( 0x1F,0x00 ),DP_LPC( 0x0401,0 ), DP_END };
//STATIC FLOOPY_CONN_DEVICE_PATH    mFloopyADevicePath       = { DP_ACPI, DP_PCI( 0x1F,0x00 ),DP_LPC( 0x0604,0 ), DP_END };
//STATIC FLOOPY_CONN_DEVICE_PATH    mFloopyBDevicePath       = { DP_ACPI, DP_PCI( 0x1F,0x00 ),DP_LPC( 0x0604,1 ), DP_END };
//STATIC USB_PORT_DEVICE_PATH       mUsb0DevicePath          = { DP_ACPI, DP_PCI( 0x1d,0x00 ), DP_END };
//STATIC USB_PORT_DEVICE_PATH       mUsb1DevicePath          = { DP_ACPI, DP_PCI( 0x1d,0x01 ), DP_END };
//STATIC USB_PORT_DEVICE_PATH       mUsb2DevicePath          = { DP_ACPI, DP_PCI( 0x1d,0x02 ), DP_END };
//STATIC USB_PORT_DEVICE_PATH       mUsb3DevicePath          = { DP_ACPI, DP_PCI( 0x1d,0x03 ), DP_END };
//STATIC IDE_DEVICE_PATH            mIdeDevicePath           = { DP_ACPI, DP_PCI( 0x1F,0x01 ), DP_END };
//STATIC IDE_DEVICE_PATH            mSata1DevicePath         = { DP_ACPI, DP_PCI( 0x1F,0x02 ), DP_END };
//STATIC GB_NIC_DEVICE_PATH         mGbNicDevicePath         = { DP_ACPI, DP_PCI( 0x03,0x00 ),DP_PCI( 0x1F,0x00 ),DP_PCI( 0x07,0x00 ), DP_END };
EFI_DEVICE_PATH_PROTOCOL          mEndDevicePath           = DP_END;

MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector1);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector2);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector3);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector4);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector5);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector6);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector7);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector8);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector9);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector10);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector11);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector12);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector13);
MISC_SMBIOS_DATA_TABLE_EXTERNS (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR, MiscPortConnector14);


EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR  *mMiscConnectorArray[SMBIOS_PORT_CONNECTOR_MAX_NUM] =
{
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector1),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector2),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector3),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector4),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector5),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector6),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector7),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector8),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector9),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector10),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector11),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector12),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector13),
  MISC_SMBIOS_DATA_TABLE_POINTER(MiscPortConnector14),
};

BOOLEAN  PcdMiscPortIsInit = FALSE;
SMBIOS_PORT_CONNECTOR_DESIGNATOR_COFNIG SMBIOSPortConnector = {0};


/**
  Get Misc Port Configuration information from PCD
  @param  SMBIOSPortConnector                 Pointer to SMBIOSPortConnector table.

**/

VOID
GetMiscPortConfigFromPcd ()
{
  //
  // Type 8
  //
  SMBIOSPortConnector.SMBIOSConnectorNumber = PcdGet8 (PcdSMBIOSConnectorNumber);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort1InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[0].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort1ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[0].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[0].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort1InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[0].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort1ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[0].PortType = PcdGet8 (PcdSMBIOSPort1Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort2InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[1].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort2ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[1].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[1].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort2InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[1].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort2ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[1].PortType = PcdGet8 (PcdSMBIOSPort2Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort3InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[2].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort3ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[2].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[2].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort3InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[2].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort3ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[2].PortType = PcdGet8 (PcdSMBIOSPort3Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort4InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[3].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort4ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[3].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[3].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort4InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[3].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort4ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[3].PortType = PcdGet8 (PcdSMBIOSPort4Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort5InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[4].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort5ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[4].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[4].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort5InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[4].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort5ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[4].PortType = PcdGet8 (PcdSMBIOSPort5Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort6InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[5].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort6ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[5].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[5].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort6InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[5].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort6ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[5].PortType = PcdGet8 (PcdSMBIOSPort6Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort7InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[6].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort7ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[6].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[6].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort7InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[6].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort7ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[6].PortType = PcdGet8 (PcdSMBIOSPort7Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort8InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[7].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort8ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[7].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[7].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort8InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[7].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort8ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[7].PortType = PcdGet8 (PcdSMBIOSPort8Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort9InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[8].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort9ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[8].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[8].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort9InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[8].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort9ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[8].PortType = PcdGet8 (PcdSMBIOSPort9Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort10InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[9].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort10ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[9].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[9].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort10InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[9].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort10ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[9].PortType = PcdGet8 (PcdSMBIOSPort10Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort11InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[10].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort11ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[10].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[10].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort11InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[10].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort11ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[10].PortType = PcdGet8 (PcdSMBIOSPort11Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort12InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[11].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort12ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[11].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[11].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort12InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[11].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort12ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[11].PortType = PcdGet8 (PcdSMBIOSPort12Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort13InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[12].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort13ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[12].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[12].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort13InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[12].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort13ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[12].PortType = PcdGet8 (PcdSMBIOSPort13Type);

  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort14InternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[13].PortInternalConnectorDesignator);
  AsciiStrToUnicodeStr ((CHAR8 *) PcdGetPtr(PcdSMBIOSPort14ExternalConnectorDesignator), SMBIOSPortConnector.SMBIOSPortConnector[13].PortExternalConnectorDesignator);
  SMBIOSPortConnector.SMBIOSPortConnector[13].PortInternalConnectorType = PcdGet8 (PcdSMBIOSPort14InternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[13].PortExternalConnectorType = PcdGet8 (PcdSMBIOSPort14ExternalConnectorType);
  SMBIOSPortConnector.SMBIOSPortConnector[13].PortType = PcdGet8 (PcdSMBIOSPort14Type);
}
/**
  This function makes boot time changes to the contents of the
  MiscPortConnectorInformation (Type 8).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscPortInternalConnectorDesignator)
{
  CHAR8                                        *OptionalStrStart;
  UINTN                                        InternalRefStrLen;
  UINTN                                        ExternalRefStrLen;
  EFI_STRING                                   InternalRef;
  EFI_STRING                                   ExternalRef;
  STRING_REF                                   TokenForInternal;
  STRING_REF                                   TokenForExternal;
  STRING_REF                                   TokenToUpdate;
  UINT8                                        InternalType;
  UINT8                                        ExternalType;
  UINT8                                        PortType;
  EFI_STATUS                                   Status;
  SMBIOS_TABLE_TYPE8                           *SmbiosRecord;
  EFI_SMBIOS_HANDLE                            SmbiosHandle;
  EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR  *ForType8InputData;
  UINT8                                        Index;

  ForType8InputData = (EFI_MISC_PORT_INTERNAL_CONNECTOR_DESIGNATOR *)RecordData;
  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenForInternal = 0;
  TokenForExternal = 0;
  InternalType     = 0;
  ExternalType     = 0;
  PortType         = 0;

  if (!PcdMiscPortIsInit) {
    GetMiscPortConfigFromPcd ();
    PcdMiscPortIsInit = TRUE;
  }

  for (Index = 0; Index < SMBIOS_PORT_CONNECTOR_MAX_NUM; Index++) {
    if (ForType8InputData->PortInternalConnectorDesignator == (mMiscConnectorArray[Index])->PortInternalConnectorDesignator) {
      //DEBUG ((EFI_D_ERROR, "Found Port Connector Data %d : ", Index));
      break;
    }
  }
  if (Index >= SMBIOSPortConnector.SMBIOSConnectorNumber) {
    return EFI_SUCCESS;
  }

  if (Index >= SMBIOS_PORT_CONNECTOR_MAX_NUM) {
    return EFI_INVALID_PARAMETER;
  }

  InternalRef = SMBIOSPortConnector.SMBIOSPortConnector[Index].PortInternalConnectorDesignator;
  if (StrLen (InternalRef) > 0) {
    TokenToUpdate = STRING_TOKEN ((mMiscConnectorArray[Index])->PortInternalConnectorDesignator);
    HiiSetString (mHiiHandle, TokenToUpdate, InternalRef, NULL);
  }
  ExternalRef = SMBIOSPortConnector.SMBIOSPortConnector[Index].PortExternalConnectorDesignator;
  if (StrLen (ExternalRef) > 0) {
    TokenToUpdate = STRING_TOKEN ((mMiscConnectorArray[Index])->PortExternalConnectorDesignator);
    HiiSetString (mHiiHandle, TokenToUpdate, ExternalRef, NULL);
  }
  TokenForInternal = STRING_TOKEN ((mMiscConnectorArray[Index])->PortInternalConnectorDesignator);
  TokenForExternal = STRING_TOKEN ((mMiscConnectorArray[Index])->PortExternalConnectorDesignator);
  InternalType = SMBIOSPortConnector.SMBIOSPortConnector[Index].PortInternalConnectorType;
  ExternalType = SMBIOSPortConnector.SMBIOSPortConnector[Index].PortExternalConnectorType;
  PortType = SMBIOSPortConnector.SMBIOSPortConnector[Index].PortType;

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
  SmbiosRecord->InternalConnectorType = InternalType;
  SmbiosRecord->ExternalReferenceDesignator = 2;
  SmbiosRecord->ExternalConnectorType = ExternalType;
  SmbiosRecord->PortType = PortType;

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
