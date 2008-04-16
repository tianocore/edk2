/**@file
  This file contains functions related to Config Access Protocols installed by
  by HII Thunk Modules which is used to thunk UEFI Config Access Callback to 
  Framework HII Callback.
  
Copyright (c) 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"

HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE ConfigAccessProtocolInstanceTempate = {
  HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_SIGNATURE,
  {
    ThunkExtractConfig,
    ThunkRouteConfig,
    ThunkCallback
  }, //ConfigAccessProtocol
  NULL, //FrameworkFormCallbackProtocol
  {NULL, NULL} //ConfigAccessStorageListHead
};

EFI_HII_PACKAGE_HEADER *
GetIfrFormSet (
  IN  CONST EFI_HII_PACKAGES  *Packages
  )
{
  TIANO_AUTOGEN_PACKAGES_HEADER **TianoAutogenPackageHdrArray;
  EFI_HII_PACKAGE_HEADER        *IfrPackage;
  UINTN                         Index;

  ASSERT (Packages != NULL);

  IfrPackage = NULL;

  TianoAutogenPackageHdrArray = (TIANO_AUTOGEN_PACKAGES_HEADER **) (((UINT8 *) &Packages->GuidId) + sizeof (Packages->GuidId));
  for (Index = 0; Index < Packages->NumberOfPackages; Index++) {
    //
    // BugBug: The current UEFI HII build tool generate a binary in the format defined in: 
    // TIANO_AUTOGEN_PACKAGES_HEADER. We assume that all packages generated in
    // this binary is with same package type. So the returned IfrPackNum and StringPackNum
    // may not be the exact number of valid package number in the binary generated 
    // by HII Build tool.
    //
    switch (TianoAutogenPackageHdrArray[Index]->PackageHeader.Type) {
      case EFI_HII_PACKAGE_FORM:
        return &TianoAutogenPackageHdrArray[Index]->PackageHeader;
        break;

      case EFI_HII_PACKAGE_STRINGS:
      case EFI_HII_PACKAGE_SIMPLE_FONTS:
        break;

      //
      // The following fonts are invalid for a module that using Framework to UEFI thunk layer.
      //
      case EFI_HII_PACKAGE_KEYBOARD_LAYOUT:
      case EFI_HII_PACKAGE_FONTS:
      case EFI_HII_PACKAGE_IMAGES:
      default:
        ASSERT (FALSE);
        break;
    }
  }

  return (EFI_HII_PACKAGE_HEADER *) NULL;
}

EFI_STATUS
GetBufferStorage  (
  IN  CONST EFI_HII_PACKAGE_HEADER *FormSetPackage,
  OUT       LIST_ENTRY             *BufferStorageListHead
  )
{
  UINTN                   OpCodeOffset;
  UINTN                   OpCodeLength;
  UINT8                   *OpCodeData;
  UINT8                   Operand;
  EFI_IFR_VARSTORE        *VarStoreOpCode;
  HII_TRHUNK_BUFFER_STORAGE_KEY *BufferStorageKey;

  OpCodeOffset = sizeof (EFI_HII_PACKAGE_HEADER);
  while (OpCodeOffset < FormSetPackage->Length) {
    OpCodeData = (UINT8 *) FormSetPackage + OpCodeOffset;

    OpCodeLength = ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
    OpCodeOffset += OpCodeLength;
    Operand = ((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode;

    if (Operand == EFI_IFR_VARSTORE_OP) {
      VarStoreOpCode = (EFI_IFR_VARSTORE *)OpCodeData;
      BufferStorageKey = AllocateZeroPool (sizeof (*BufferStorageKey));
      if (BufferStorageKey == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      CopyMem (&BufferStorageKey->Guid, &VarStoreOpCode->Guid, sizeof (EFI_GUID));
      
      BufferStorageKey->Name = AllocateZeroPool (AsciiStrSize (VarStoreOpCode->Name) * 2);
      AsciiStrToUnicodeStr (VarStoreOpCode->Name, BufferStorageKey->Name);

      BufferStorageKey->VarStoreId = VarStoreOpCode->VarStoreId;

      BufferStorageKey->Size = VarStoreOpCode->Size;
      BufferStorageKey->Signature = HII_TRHUNK_BUFFER_STORAGE_KEY_SIGNATURE;

      InsertTailList (BufferStorageListHead, &BufferStorageKey->List);
    }
  }
  return EFI_SUCCESS;
}
  

EFI_STATUS
InstallDefaultUefiConfigAccessProtocol (
  IN  CONST EFI_HII_PACKAGES                         *Packages,
  OUT       EFI_HANDLE                               *Handle,
  IN  OUT   HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *MapEntry
  )
{
  EFI_HII_PACKAGE_HEADER                      *FormSetPackage;
  EFI_STATUS                                  Status;
  HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE  *ConfigAccessInstance;

  Status = HiiLibCreateHiiDriverHandle (Handle);
  ConfigAccessInstance = AllocateCopyPool (
                           sizeof (HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE), 
                           &ConfigAccessProtocolInstanceTempate
                           );
  InitializeListHead (&ConfigAccessInstance->ConfigAccessBufferStorageListHead);

  //
  // We assume there is only one formset package in each Forms Package
  //
  FormSetPackage = GetIfrFormSet (Packages);
  Status = GetBufferStorage (FormSetPackage, &ConfigAccessInstance->ConfigAccessBufferStorageListHead);
  if (EFI_ERROR (Status)) {
    FreePool (ConfigAccessInstance);
    ASSERT (FALSE);
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
          Handle,
          &gEfiHiiConfigAccessProtocolGuid,
          &ConfigAccessInstance->ConfigAccessProtocol,
          NULL
          );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    FreePool  (ConfigAccessInstance);
    return Status;
  }
  
  return EFI_SUCCESS;
}

EFI_STATUS
RouteConfigToFrameworkFormCallBack (
  IN       HII_TRHUNK_BUFFER_STORAGE_KEY              *BufferStorageKey,
  IN       EFI_FORM_CALLBACK_PROTOCOL                 *FrameworkFormCallBack,
  IN       VOID                                       *Data,
  IN       UINTN                                      DataSize
  )
{
  EFI_STATUS          Status;
  BOOLEAN             ResetRequired;

  Status = FrameworkFormCallBack->NvWrite (
              FrameworkFormCallBack,  
              BufferStorageKey->Name,
              &BufferStorageKey->Guid,
              EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
              DataSize,
              Data,
              &ResetRequired
              );
  return Status;
}

EFI_STATUS
ExtractConfigFromFrameworkFormCallBack (
  IN       HII_TRHUNK_BUFFER_STORAGE_KEY              *BufferStorageKey,
  IN       EFI_FORM_CALLBACK_PROTOCOL                 *FrameworkFormCallBack,
  OUT      VOID                                       **Data,
  OUT      UINTN                                      *DataSize
  )
{
  EFI_STATUS          Status;

  *DataSize = 0;
  *Data     = NULL;
  
  Status = FrameworkFormCallBack->NvRead (
              FrameworkFormCallBack,  
              BufferStorageKey->Name,
              &BufferStorageKey->Guid,
              NULL,
              DataSize,
              *Data
              );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    if (BufferStorageKey->Size != *DataSize) {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    *Data = AllocateZeroPool (*DataSize);
    if (Data == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    FrameworkFormCallBack->NvRead (
                  FrameworkFormCallBack,  
                  BufferStorageKey->Name,
                  &BufferStorageKey->Guid,
                  NULL,
                  DataSize,
                  *Data
                  );
  }

  return Status;
}

EFI_STATUS
RouteConfigToUefiVariable (
  IN       HII_TRHUNK_BUFFER_STORAGE_KEY              *BufferStorageKey,
  IN       VOID                                       *Data,
  IN       UINTN                                      DataSize
  )
{
  return gRT->SetVariable (
                  BufferStorageKey->Name,
                  &BufferStorageKey->Guid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  DataSize,
                  Data
                  );
  
}
EFI_STATUS
ExtractConfigFromUefiVariable (
  IN       HII_TRHUNK_BUFFER_STORAGE_KEY              *BufferStorageKey,
  OUT      VOID                                       **Data,
  OUT      UINTN                                      *DataSize
  )
{
  EFI_STATUS          Status;

  *DataSize = 0;
  *Data = NULL;
  Status = gRT->GetVariable (
              BufferStorageKey->Name,
              &BufferStorageKey->Guid,
              NULL,
              DataSize,
              *Data
              );
  if (Status == EFI_BUFFER_TOO_SMALL) {

    if (BufferStorageKey->Size != *DataSize) {
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
    }

    *Data = AllocateZeroPool (*DataSize);
    if (Data == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gRT->GetVariable (
                BufferStorageKey->Name,
                &BufferStorageKey->Guid,
                NULL,
                DataSize,
                *Data
                );
  }

  return Status;
}


EFI_STATUS
EFIAPI
ThunkExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                                  Status;
  HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE  *ConfigaAccessInstance;
  LIST_ENTRY                                  *ListEntry;
  HII_TRHUNK_BUFFER_STORAGE_KEY               *BufferStorageKey;
  VOID                                        *Data;
  UINTN                                       DataSize;

  Data = NULL;
  ConfigaAccessInstance = HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_FROM_PROTOCOL (This);

  ListEntry = GetFirstNode (&ConfigaAccessInstance->ConfigAccessBufferStorageListHead);
  if (ListEntry == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }
  
  BufferStorageKey = HII_TRHUNK_BUFFER_STORAGE_KEY_FROM_LIST_ENTRY (ListEntry);

  if (ConfigaAccessInstance->FrameworkFormCallbackProtocol == NULL ||
      ConfigaAccessInstance->FrameworkFormCallbackProtocol->NvRead == NULL) {
    Status = ExtractConfigFromUefiVariable (
               BufferStorageKey,
               &Data,
               &DataSize
               );
  } else {
    Status = ExtractConfigFromFrameworkFormCallBack (
               BufferStorageKey,
               ConfigaAccessInstance->FrameworkFormCallbackProtocol,
                &Data,
                &DataSize
               );
  }
  
  if (!EFI_ERROR (Status)) {
    Status = mUefiConfigRoutingProtocol->BlockToConfig (
                                            mUefiConfigRoutingProtocol,
                                            Request,
                                            Data,
                                            DataSize,
                                            Results,
                                            Progress
                                            );
  }

  SafeFreePool (Data);
  return Status;
}


EFI_STATUS
EFIAPI
ThunkRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                                  Status;
  HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE  *ConfigaAccessInstance;
  LIST_ENTRY                                  *ListEntry;
  HII_TRHUNK_BUFFER_STORAGE_KEY               *BufferStorageKey;
  VOID                                        *Data;
  UINTN                                       DataSize;
  UINTN                                       LastModifiedByteIndex;

  Data = NULL;
  ConfigaAccessInstance = HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_FROM_PROTOCOL (This);

  ListEntry = GetFirstNode (&ConfigaAccessInstance->ConfigAccessBufferStorageListHead);
  if (ListEntry == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  BufferStorageKey = HII_TRHUNK_BUFFER_STORAGE_KEY_FROM_LIST_ENTRY (ListEntry);

  Data = AllocateZeroPool (BufferStorageKey->Size);
  if (Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = mUefiConfigRoutingProtocol->ConfigToBlock (
                                          mUefiConfigRoutingProtocol,
                                          Configuration,
                                          Data,
                                          &LastModifiedByteIndex,
                                          Progress
                                          );

  if (EFI_ERROR (Status)) {
    goto Done;
  }

  DataSize = BufferStorageKey->Size;
  if (ConfigaAccessInstance->FrameworkFormCallbackProtocol == NULL ||
      ConfigaAccessInstance->FrameworkFormCallbackProtocol->NvRead == NULL) {
    Status = RouteConfigToUefiVariable (
               BufferStorageKey,
               Data,
               DataSize
               );
  } else {
    Status = RouteConfigToFrameworkFormCallBack (
               BufferStorageKey,
               ConfigaAccessInstance->FrameworkFormCallbackProtocol,
               Data,
               DataSize
               );
  }

Done:  
  SafeFreePool (Data);
  return Status;
}

EFI_STATUS
EFIAPI
ThunkCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  EFI_STATUS                                  Status;
  HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE  *ConfigaAccessInstance;
  EFI_FORM_CALLBACK_PROTOCOL                  *FrameworkFormCallbackProtocol;
  EFI_HII_CALLBACK_PACKET                     *Packet;
  FRAMEWORK_EFI_IFR_DATA_ARRAY                Data;
  FRAMEWORK_EFI_IFR_DATA_ENTRY                *DataEntry;
  EFI_FORM_CALLBACK                           Callback; 

  ASSERT (This != NULL);
  ASSERT (Value != NULL);
  ASSERT (ActionRequest != NULL);

  *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

  ConfigaAccessInstance = HII_TRHUNK_CONFIG_ACCESS_PROTOCOL_INSTANCE_FROM_PROTOCOL (This);

  FrameworkFormCallbackProtocol = ConfigaAccessInstance->FrameworkFormCallbackProtocol;
  if (FrameworkFormCallbackProtocol == NULL) {
    return EFI_UNSUPPORTED;
  }
  Callback = FrameworkFormCallbackProtocol->Callback;

  Status = Callback (
              FrameworkFormCallbackProtocol,
              QuestionId,
              &Data,
              &Packet
              );

  //
  // Callback require browser to perform action
  //
  if (Packet != NULL) {
    if (Packet->DataArray.EntryCount  == 1 && Packet->DataArray.NvRamMap == NULL) {
      DataEntry = (FRAMEWORK_EFI_IFR_DATA_ENTRY *) ((UINT8 *) Packet + sizeof (FRAMEWORK_EFI_IFR_DATA_ARRAY));
      switch (DataEntry->Flags) {
        case EXIT_REQUIRED:
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_EXIT;
          break;
        case SAVE_REQUIRED:
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_SUBMIT;
          break;
        case RESET_REQUIRED:
            *ActionRequest = EFI_BROWSER_ACTION_REQUEST_RESET;
            break;
        default:
            *ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
            break;  
      }
    }
  }
  
  return Status;
}

