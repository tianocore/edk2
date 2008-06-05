/**@file
Framework to UEFI 2.1 HII Thunk. The driver consume UEFI HII protocols
to produce a Framework HII protocol.

Copyright (c) 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"

EFI_HII_THUNK_PRIVATE_DATA *mHiiThunkPrivateData;

EFI_HII_THUNK_PRIVATE_DATA mHiiThunkPrivateDataTempate = {
  //Signature
  EFI_HII_THUNK_DRIVER_DATA_SIGNATURE 
  ,
  //Handle
  (EFI_HANDLE) NULL
  ,
  { //Hii
    HiiNewPack,
    HiiRemovePack,
    HiiFindHandles,
    HiiExportDatabase,
    
    HiiTestString,
    HiiGetGlyph,
    HiiGlyphToBlt,
    
    HiiNewString,
    HiiGetPrimaryLanguages,
    HiiGetSecondaryLanguages,
    HiiGetString,
    HiiResetStrings,
    HiiGetLine,
    HiiGetForms,
    HiiGetDefaultImage,
    HiiUpdateForm,
    
    HiiGetKeyboardLayout
  },
  //StaticHiiHandle
  //The FRAMEWORK_EFI_HII_HANDLE starts from 1 
  // and increase upwords untill reach the value of StaticPureUefiHiiHandle. 
  // The code will assert to prevent overflow.
  (FRAMEWORK_EFI_HII_HANDLE) 1 
  ,
  //StaticPureUefiHiiHandle
  //The Static FRAMEWORK_EFI_HII_HANDLE starts from 0xFFFF 
  // and decrease downwords untill reach the value of StaticHiiHandle. 
  // The code will assert to prevent overflow.
  (FRAMEWORK_EFI_HII_HANDLE) 0xFFFF 
  ,
  {
    NULL, NULL                  //HiiHandleLinkList
  },
};

EFI_FORMBROWSER_THUNK_PRIVATE_DATA mBrowserThunkPrivateDataTemplate = {
  EFI_FORMBROWSER_THUNK_PRIVATE_DATA_SIGNATURE,
  (EFI_HANDLE) NULL,
  {
    ThunkSendForm,
    ThunkCreatePopUp
  }
};


CONST EFI_HII_DATABASE_PROTOCOL            *mHiiDatabase;
CONST EFI_HII_FONT_PROTOCOL                *mHiiFontProtocol;
CONST EFI_HII_IMAGE_PROTOCOL               *mHiiImageProtocol;
CONST EFI_HII_STRING_PROTOCOL              *mHiiStringProtocol;
CONST EFI_HII_CONFIG_ROUTING_PROTOCOL      *mHiiConfigRoutingProtocol;

EFI_STATUS
RegisterUefiHiiHandle (
  EFI_HII_THUNK_PRIVATE_DATA *Private,
  EFI_HII_HANDLE             UefiHiiHandle
 )
{
  EFI_STATUS     Status;
  EFI_GUID       PackageGuid;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMappingEntry;

  HandleMappingEntry = AllocateZeroPool (sizeof (*HandleMappingEntry));
  HandleMappingEntry->Signature = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_SIGNATURE;

  Status = AssignPureUefiHiiHandle (Private, &HandleMappingEntry->FrameworkHiiHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  HandleMappingEntry->UefiHiiHandle = UefiHiiHandle;
  Status = HiiLibExtractGuidFromHiiHandle (UefiHiiHandle, &PackageGuid);
  ASSERT_EFI_ERROR (Status);
  
  CopyGuid(&HandleMappingEntry->TagGuid, &PackageGuid);
  
  InsertTailList (&Private->HiiThunkHandleMappingDBListHead, &HandleMappingEntry->List);

  return EFI_SUCCESS;
}


EFI_STATUS
UnRegisterUefiHiiHandle (
  EFI_HII_THUNK_PRIVATE_DATA *Private,
  EFI_HII_HANDLE UefiHiiHandle
 )
{
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY     *MapEntry;

  MapEntry = UefiHiiHandleToMapDatabaseEntry (Private, UefiHiiHandle);
  ASSERT (MapEntry != NULL);
  
  RemoveEntryList (&MapEntry->List);

  FreePool (MapEntry);
    
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AddPackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS   Status;
  EFI_HII_THUNK_PRIVATE_DATA *Private;

  ASSERT (PackageType == EFI_HII_PACKAGE_STRINGS);
  ASSERT (NotifyType == EFI_HII_DATABASE_NOTIFY_ADD_PACK);

  Status  = EFI_SUCCESS;
  Private = mHiiThunkPrivateData;

  if (mInFrameworkHiiNewPack) {
    return EFI_SUCCESS;
  }

  //
  // We only create a MapEntry if the Uefi Hii Handle is only already registered
  // by the HII Thunk Layer.
  //
  if (UefiHiiHandleToMapDatabaseEntry (Private, Handle) == NULL) {
    Status = RegisterUefiHiiHandle (Private, Handle);
  } 

  return Status;  
}
EFI_STATUS
EFIAPI
NewPackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS   Status;
  EFI_HII_THUNK_PRIVATE_DATA *Private;

  ASSERT (PackageType == EFI_HII_PACKAGE_STRINGS);
  ASSERT (NotifyType == EFI_HII_DATABASE_NOTIFY_NEW_PACK);

  if (mInFrameworkHiiNewPack) {
    return EFI_SUCCESS;
  }

  Status  = EFI_SUCCESS;
  Private = mHiiThunkPrivateData;

  //
  // We only
  //
  if (UefiHiiHandleToMapDatabaseEntry (Private, Handle) == NULL) {
    Status = RegisterUefiHiiHandle (Private, Handle);
  } 

  return Status;
}

BOOLEAN
IsLastStringPack (
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle
  )
{
  EFI_HII_PACKAGE_LIST_HEADER *HiiPackageList;
  UINTN                        BufferSize;
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_HEADER       *PackageHdrPtr;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  BOOLEAN                      Match;

  Match = FALSE;
  HiiPackageList = NULL;
  BufferSize = 0;
  Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
  ASSERT (Status != EFI_NOT_FOUND);
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocateZeroPool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, Handle, &BufferSize, HiiPackageList);
  }

  
  PackageHdrPtr = (EFI_HII_PACKAGE_HEADER *) ((UINT8 *) HiiPackageList + sizeof (EFI_HII_PACKAGE_LIST_HEADER));
  CopyMem (&PackageHeader, PackageHdrPtr, sizeof (EFI_HII_PACKAGE_HEADER));

  Status = EFI_SUCCESS;

  while (PackageHeader.Type != EFI_HII_PACKAGE_END) {
    switch (PackageHeader.Type) {
      case EFI_HII_PACKAGE_STRINGS:
        if (CompareMem (Package, PackageHdrPtr, Package->Length) != 0) {
          FreePool (HiiPackageList);
          return FALSE;
        }
      break;      
      default:
        break;
    }
    //
    // goto header of next package
    //
    PackageHdrPtr = (EFI_HII_PACKAGE_HEADER *) ((UINT8 *) PackageHdrPtr + PackageHeader.Length);
    CopyMem (&PackageHeader, PackageHdrPtr, sizeof (EFI_HII_PACKAGE_HEADER));
  }

  FreePool (HiiPackageList);
  return TRUE;
}

EFI_STATUS
EFIAPI
RemovePackNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  EFI_STATUS   Status;
  EFI_HII_THUNK_PRIVATE_DATA *Private;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY * MapEntry;

  Status = EFI_SUCCESS;

  ASSERT (PackageType == EFI_HII_PACKAGE_STRINGS);
  ASSERT (NotifyType == EFI_HII_DATABASE_NOTIFY_REMOVE_PACK);

  if (mInFrameworkHiiRemovePack) {
    return EFI_SUCCESS;
  }

  Private = mHiiThunkPrivateData;

  MapEntry = UefiHiiHandleToMapDatabaseEntry (Private, Handle);

  if (MapEntry->FrameworkHiiHandle > Private->StaticHiiHandle) {
    //
    // This is a PackageList registered using UEFI HII Protocol Instance.
    // The MapEntry->TagGuid for this type of PackageList is a auto generated GUID
    // to link StringPack with IfrPack.
    // RemovePackNotify is only used to remove PackageList when it is removed by
    // calling mHiiDatabase->RemovePackageList interface.
    if (IsLastStringPack (Package, Handle)) {
      Status = UnRegisterUefiHiiHandle (Private, Handle);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
MapUefiHiiHandles (
  EFI_HII_THUNK_PRIVATE_DATA *Private
  )
{
  UINTN          HandleBufferLength;
  EFI_HII_HANDLE *HandleBuffer;
  EFI_STATUS     Status;
  UINTN          Index;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY * MapEntry;

  HandleBufferLength = 0;
  HandleBuffer       = NULL;
  Status = mHiiDatabase->ListPackageLists (
                            mHiiDatabase,
                            EFI_HII_PACKAGE_TYPE_ALL,
                            NULL,
                            &HandleBufferLength,
                            HandleBuffer
                            );
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    //
    // As no package is registered to UEFI HII Database, EFI_SUCCESS is returned.
    // 
    //
    if (Status == EFI_NOT_FOUND) {
      return EFI_SUCCESS;
    } else {
      return Status;
    }
  }

  HandleBuffer = AllocateZeroPool (HandleBufferLength);
  Status = mHiiDatabase->ListPackageLists (
                            mHiiDatabase,
                            EFI_HII_PACKAGE_TYPE_ALL,
                            NULL,
                            &HandleBufferLength,
                            HandleBuffer
                            );

  for (Index = 0; Index < HandleBufferLength / sizeof (EFI_HII_HANDLE); Index++) {
    MapEntry = UefiHiiHandleToMapDatabaseEntry (Private, HandleBuffer[Index]);
    //
    // Only register those UEFI HII Handles that are registered using the UEFI HII database interface.
    //
    if (MapEntry == NULL) {
      Status = RegisterUefiHiiHandle (Private, HandleBuffer[Index]);
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;                  
}

EFI_STATUS
EFIAPI
InitializeHiiDatabase (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:
  Initialize HII Database

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_SUCCESS - Setup loaded.
  other       - Setup Error

--*/
{
  EFI_HII_THUNK_PRIVATE_DATA *HiiData;
  EFI_HANDLE                 Handle;
  EFI_STATUS                 Status;

  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiHiiProtocolGuid);

  HiiData = AllocateCopyPool (sizeof (EFI_HII_THUNK_PRIVATE_DATA), &mHiiThunkPrivateDataTempate);
  ASSERT (HiiData != NULL);
  InitializeListHead (&HiiData->HiiThunkHandleMappingDBListHead);

  mHiiThunkPrivateData = HiiData;

  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &mHiiDatabase
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiFontProtocolGuid,
                  NULL,
                  (VOID **) &mHiiFontProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiImageProtocolGuid,
                  NULL,
                  (VOID **) &mHiiImageProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **) &mHiiStringProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &mHiiConfigRoutingProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install protocol interface
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &HiiData->Handle,
                  &gEfiHiiProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *) &HiiData->Hii
                  );
  ASSERT_EFI_ERROR (Status);

  Status = MapUefiHiiHandles (HiiData);
  ASSERT_EFI_ERROR (Status);

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_STRINGS,
                           NULL,
                           NewPackNotify,
                           EFI_HII_DATABASE_NOTIFY_NEW_PACK,
                           &HiiData->NewPackNotifyHandle
                           );
  ASSERT_EFI_ERROR (Status);

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_STRINGS,
                           NULL,
                           AddPackNotify,
                           EFI_HII_DATABASE_NOTIFY_ADD_PACK,
                           &HiiData->AddPackNotifyHandle
                           );
  ASSERT_EFI_ERROR (Status);

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_STRINGS,
                           NULL,
                           RemovePackNotify,
                           EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                           &HiiData->RemovePackNotifyHandle
                           );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->InstallProtocolInterface (
                  &mBrowserThunkPrivateDataTemplate.Handle,
                  &gEfiFormBrowserProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *) &mBrowserThunkPrivateDataTemplate.FormBrowser
                  );
  ASSERT_EFI_ERROR (Status);
  
  return Status;
}

EFI_STATUS
EFIAPI
HiiFindHandles (
  IN     EFI_HII_PROTOCOL *This,
  IN OUT UINT16           *HandleBufferLength,
  OUT    FRAMEWORK_EFI_HII_HANDLE    Handle[1]
  )
/*++

Routine Description:
  Determines the handles that are currently active in the database.

Arguments:

Returns:

--*/
{
  UINT16                                     Count;
  LIST_ENTRY                                *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY *HandleMapEntry;
  EFI_HII_THUNK_PRIVATE_DATA               *Private;

  if (HandleBufferLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);  

  Count = 0;
  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {
    Count++;
  }

  if (Count > *HandleBufferLength) {
    *HandleBufferLength = (Count * sizeof (FRAMEWORK_EFI_HII_HANDLE));
    return EFI_BUFFER_TOO_SMALL;
  }

  Count = 0;
  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {
    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    Handle[Count] = HandleMapEntry->FrameworkHiiHandle;
  
    Count++;
  }  

  *HandleBufferLength = (Count * sizeof (FRAMEWORK_EFI_HII_HANDLE));
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiGetPrimaryLanguages (
  IN  EFI_HII_PROTOCOL      *This,
  IN  FRAMEWORK_EFI_HII_HANDLE         Handle,
  OUT EFI_STRING            *LanguageString
  )
/*++

Routine Description:

  This function allows a program to determine what the primary languages that are supported on a given handle.

Arguments:

Returns:

--*/
{
  EFI_HII_THUNK_PRIVATE_DATA *Private;
  EFI_HII_HANDLE             UefiHiiHandle;
  CHAR8                      *AsciiLanguageCodes;
  CHAR16                     *UnicodeLanguageCodes;

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  
  
  UefiHiiHandle = FrameworkHiiHandleToUefiHiiHandle (Private, Handle);
  if (UefiHiiHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsciiLanguageCodes = HiiLibGetSupportedLanguages (UefiHiiHandle);

  if (AsciiLanguageCodes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UnicodeLanguageCodes = AllocateZeroPool (AsciiStrSize (AsciiLanguageCodes) * sizeof (CHAR16));
  if (UnicodeLanguageCodes == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // The language returned is in RFC 3066 format.
  //
  *LanguageString = AsciiStrToUnicodeStr (AsciiLanguageCodes, UnicodeLanguageCodes);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL      *This,
  IN  FRAMEWORK_EFI_HII_HANDLE         Handle,
  IN  CHAR16                *PrimaryLanguage,
  OUT EFI_STRING            *LanguageString
  )
/*++

Routine Description:

  This function allows a program to determine which secondary languages are supported
  on a given handle for a given primary language.

  Arguments:

Returns:

--*/
{
  EFI_HII_THUNK_PRIVATE_DATA *Private;
  EFI_HII_HANDLE             UefiHiiHandle;
  CHAR8                      *AsciiPrimaryLanguage;
  CHAR8                      *AsciiLanguageCodes;
  CHAR16                     *UnicodeLanguageCodes;

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  
  
  UefiHiiHandle = FrameworkHiiHandleToUefiHiiHandle (Private, Handle);
  if (UefiHiiHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AsciiPrimaryLanguage = AllocateZeroPool (StrLen (PrimaryLanguage) + 1);

  UnicodeStrToAsciiStr (PrimaryLanguage, AsciiPrimaryLanguage);

  AsciiLanguageCodes = HiiLibGetSupportedSecondaryLanguages (UefiHiiHandle, AsciiPrimaryLanguage);

  if (AsciiLanguageCodes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UnicodeLanguageCodes = AllocateZeroPool (AsciiStrSize (AsciiLanguageCodes) * sizeof (CHAR16));
  if (UnicodeLanguageCodes == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // The language returned is in RFC 3066 format.
  //
  *LanguageString = AsciiStrToUnicodeStr (AsciiLanguageCodes, UnicodeLanguageCodes);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI 
ThunkSendForm (
  IN  EFI_FORM_BROWSER_PROTOCOL       *This,
  IN  BOOLEAN                         UseDatabase,
  IN  FRAMEWORK_EFI_HII_HANDLE        *Handle,
  IN  UINTN                           HandleCount,
  IN  FRAMEWORK_EFI_IFR_PACKET                  *Packet, OPTIONAL
  IN  EFI_HANDLE                      CallbackHandle, OPTIONAL
  IN  UINT8                           *NvMapOverride, OPTIONAL
  IN  FRAMEWORK_EFI_SCREEN_DESCRIPTOR            *ScreenDimensions, OPTIONAL
  OUT BOOLEAN                         *ResetRequired OPTIONAL
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI 
ThunkCreatePopUp (
  IN  UINTN                           NumberOfLines,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   *KeyValue,
  IN  CHAR16                          *String,
  ...
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

