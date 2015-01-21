/*++

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscSystemManufacturerFunction.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data.

--*/


#include "CommonHeader.h"
#include "MiscSubclassDriver.h"
#include <Protocol/DxeSmmReadyToLock.h>
#include <Library/NetLib.h>
#include "Library/DebugLib.h"
#include <Uefi/UefiBaseType.h>

/**

  Publish the smbios type 1.

  @param Event      Event whose notification function is being invoked (gEfiDxeSmmReadyToLockProtocolGuid).
  @param Context    Pointer to the notification functions context, which is implementation dependent.

  @retval None

**/
EFI_STATUS
EFIAPI
AddSmbiosManuCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{

  CHAR8                             *OptionalStrStart;
  UINTN                             ManuStrLen;
  UINTN                             VerStrLen;
  UINTN                             PdNameStrLen;
  UINTN                             SerialNumStrLen;
  UINTN                             SkuNumberStrLen;
  UINTN				                FamilyNameStrLen;
  EFI_STATUS                        Status;
  EFI_STRING                        Manufacturer;
  EFI_STRING                        ProductName;
  EFI_STRING                        Version;
  EFI_STRING                        SerialNumber;
  EFI_STRING                        SkuNumber;
  EFI_STRING			            FamilyName;
  STRING_REF                        TokenToGet;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  SMBIOS_TABLE_TYPE1                *SmbiosRecord;
  EFI_MISC_SYSTEM_MANUFACTURER      *ForType1InputData;
  EFI_SMBIOS_PROTOCOL               *Smbios;
  CHAR16                            Buffer[40];
  
  CHAR16                          *MacStr; 
  EFI_HANDLE                      *Handles;
  UINTN                           BufferSize;

  ForType1InputData = (EFI_MISC_SYSTEM_MANUFACTURER *)Context;

  //
  // First check for invalid parameters.
  //
  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID *) &Smbios);
  ASSERT_EFI_ERROR (Status);

  //
  // Silicon Steppings
  //
  switch (PchStepping()) {
    case PchA0:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX A0 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"A0");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "A0 Stepping Detected\n"));
      break;
    case PchA1:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX A1 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"A1");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "A1 Stepping Detected\n"));
      break;
    case PchB0:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX B0 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"B0");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "B0 Stepping Detected\n"));
      break;
    case PchB1:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX B1 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"B1");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "B1 Stepping Detected\n"));
      break;
    case PchB2:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX B2 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"B2");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "B2 Stepping Detected\n"));
      break;
    case PchB3:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX B3 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"B3");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "B3 Stepping Detected\n"));
      break;
    case PchC0:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX C0 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"C0");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "C0 Stepping Detected\n"));
      break;
   case PchD0:
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"MinnowBoard MAX D0 PLATFORM");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_PRODUCT_NAME), Buffer, NULL);
      UnicodeSPrint (Buffer, sizeof (Buffer),L"%s",L"D0");
      HiiSetString(mHiiHandle,STRING_TOKEN(STR_MISC_SYSTEM_VERSION), Buffer, NULL);
      DEBUG ((EFI_D_ERROR, "D0 Stepping Detected\n"));
      break;
    default:
      DEBUG ((EFI_D_ERROR, "Unknow Stepping Detected\n"));
      break;
    }


  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_MANUFACTURER);
  Manufacturer = SmbiosMiscGetString (TokenToGet);
  ManuStrLen = StrLen(Manufacturer);
  if (ManuStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_PRODUCT_NAME);
  ProductName = SmbiosMiscGetString (TokenToGet);
  PdNameStrLen = StrLen(ProductName);
  if (PdNameStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_VERSION);
  Version = SmbiosMiscGetString (TokenToGet);
  VerStrLen = StrLen(Version);
  if (VerStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  //Get handle infomation
  //
  BufferSize = 0;
  Handles = NULL;
  Status = gBS->LocateHandle (
                  ByProtocol, 
                  &gEfiSimpleNetworkProtocolGuid,
                  NULL,
                  &BufferSize,
                  Handles
                  );

  if (Status == EFI_BUFFER_TOO_SMALL) {
  	Handles = AllocateZeroPool(BufferSize);
  	if (Handles == NULL) {
  		return (EFI_OUT_OF_RESOURCES);
  	}
  	Status = gBS->LocateHandle(
  	                ByProtocol,
  	                &gEfiSimpleNetworkProtocolGuid,
  	                NULL,
  	                &BufferSize,
  	                Handles
  	                );
 }
 	                
  //
  //Get the MAC string
  //
  Status = NetLibGetMacString (
             *Handles,
             NULL,
             &MacStr
             );
  if (EFI_ERROR (Status)) {	
    return Status;
  }
  SerialNumber = MacStr; 
  SerialNumStrLen = StrLen(SerialNumber);
  if (SerialNumStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_SKU_NUMBER);
  SkuNumber = SmbiosMiscGetString (TokenToGet);
  SkuNumberStrLen = StrLen(SkuNumber);
  if (SkuNumberStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }
  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_FAMILY_NAME1);
  FamilyName = SmbiosMiscGetString (TokenToGet);
  FamilyNameStrLen = StrLen(FamilyName);
  if (FamilyNameStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE1) + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + SkuNumberStrLen + 1 + FamilyNameStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE1) + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + SkuNumberStrLen + 1 + FamilyNameStrLen + 1 + 1);

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
  // Version will be the 4th optional string following the formatted structure.
  //
  SmbiosRecord->SerialNumber = 4;

  SmbiosRecord->SKUNumber= 5;
  SmbiosRecord->Family= 6;

  //
  // Unique UUID
  //
  ForType1InputData->SystemUuid.Data1 = PcdGet32 (PcdProductSerialNumber);
  ForType1InputData->SystemUuid.Data4[0] = PcdGet8 (PcdEmmcManufacturerId);

  CopyMem ((UINT8 *) (&SmbiosRecord->Uuid),&ForType1InputData->SystemUuid,16);

  SmbiosRecord->WakeUpType = (UINT8)ForType1InputData->SystemWakeupType;

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(Manufacturer, OptionalStrStart);
  UnicodeStrToAsciiStr(ProductName, OptionalStrStart + ManuStrLen + 1);
  UnicodeStrToAsciiStr(Version, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1);
  UnicodeStrToAsciiStr(SerialNumber, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1);

  UnicodeStrToAsciiStr(SkuNumber, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1 +  VerStrLen + 1 + SerialNumStrLen + 1);
  UnicodeStrToAsciiStr(FamilyName, OptionalStrStart + ManuStrLen + 1 + PdNameStrLen + 1 + VerStrLen + 1 + SerialNumStrLen + 1 + SkuNumberStrLen +1);

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
  EFI_STATUS                    Status;
  static BOOLEAN                CallbackIsInstalledManu = FALSE;
  VOID                           *AddSmbiosManuCallbackNotifyReg;
  EFI_EVENT                      AddSmbiosManuCallbackEvent;


  if (CallbackIsInstalledManu == FALSE) {
    CallbackIsInstalledManu = TRUE;        	// Prevent more than 1 callback.
    DEBUG ((EFI_D_INFO, "Create Smbios Manu callback.\n"));

  //
  // gEfiDxeSmmReadyToLockProtocolGuid is ready
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  (EFI_EVENT_NOTIFY)AddSmbiosManuCallback,
                  RecordData,
                  &AddSmbiosManuCallbackEvent
                  );

  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;

  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  AddSmbiosManuCallbackEvent,
                  &AddSmbiosManuCallbackNotifyReg
                  );

  return Status;
  }

  return EFI_SUCCESS;

}
