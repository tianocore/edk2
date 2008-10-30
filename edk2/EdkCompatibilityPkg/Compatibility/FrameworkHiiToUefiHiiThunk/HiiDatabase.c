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
#include "HiiHandle.h"

HII_THUNK_PRIVATE_DATA *mHiiThunkPrivateData;

HII_THUNK_PRIVATE_DATA mHiiThunkPrivateDataTempate = {
  HII_THUNK_PRIVATE_DATA_SIGNATURE,
  (EFI_HANDLE) NULL,
  {
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

  {
    ///
    /// HiiHandleLinkList
    ///
    NULL, NULL                  
  },
};

EFI_FORMBROWSER_THUNK_PRIVATE_DATA mBrowserThunkPrivateDataTemplate = {
  EFI_FORMBROWSER_THUNK_PRIVATE_DATA_SIGNATURE,
  (EFI_HANDLE) NULL,
  (HII_THUNK_PRIVATE_DATA *) NULL,
  {
    ThunkSendForm,
    ThunkCreatePopUp
  }
};


CONST EFI_HII_DATABASE_PROTOCOL            *mHiiDatabase;
CONST EFI_HII_IMAGE_PROTOCOL               *mHiiImageProtocol;
CONST EFI_HII_STRING_PROTOCOL              *mHiiStringProtocol;
CONST EFI_HII_FONT_PROTOCOL                *mHiiFontProtocol;
CONST EFI_HII_CONFIG_ROUTING_PROTOCOL      *mHiiConfigRoutingProtocol;
CONST EFI_FORM_BROWSER2_PROTOCOL           *mFormBrowser2Protocol;






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
  HII_THUNK_PRIVATE_DATA *Private;
  EFI_HANDLE              Handle;
  EFI_STATUS              Status;
  UINTN                   BufferLength;
  EFI_HII_HANDLE          *Buffer;
  UINTN                   Index;
  HII_THUNK_CONTEXT       *ThunkContext;
  

  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiHiiCompatibilityProtocolGuid);
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiFormBrowserCompatibilityProtocolGuid);

  Private = AllocateCopyPool (sizeof (HII_THUNK_PRIVATE_DATA), &mHiiThunkPrivateDataTempate);
  ASSERT (Private != NULL);
  InitializeListHead (&Private->ThunkContextListHead);

  InitHiiHandleDatabase ();

  mHiiThunkPrivateData = Private;

  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &mHiiDatabase
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **) &mHiiStringProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiFontProtocolGuid,
                  NULL,
                  (VOID **) &mHiiFontProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **) &mHiiConfigRoutingProtocol
                  );
  ASSERT_EFI_ERROR (Status);


  Status = gBS->LocateProtocol (
                  &gEfiFormBrowser2ProtocolGuid,
                  NULL,
                  (VOID **) &mFormBrowser2Protocol
                  );
  ASSERT_EFI_ERROR (Status);


  

  //
  // Install protocol interface
  //
  Status = gBS->InstallProtocolInterface (
                  &Private->Handle,
                  &gEfiHiiCompatibilityProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *) &Private->Hii
                  );
  ASSERT_EFI_ERROR (Status);

  Status = HiiLibListPackageLists (EFI_HII_PACKAGE_STRINGS, NULL, &BufferLength, &Buffer);
  if (Status == EFI_SUCCESS) {
    for (Index = 0; Index < BufferLength / sizeof (EFI_HII_HANDLE); Index++) {
      ThunkContext = CreateThunkContextForUefiHiiHandle (Buffer[Index]);
      ASSERT (ThunkContext!= NULL);
      
      InsertTailList (&Private->ThunkContextListHead, &ThunkContext->Link);
    }

    FreePool (Buffer);
  }

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_STRINGS,
                           NULL,
                           NewOrAddPackNotify,
                           EFI_HII_DATABASE_NOTIFY_NEW_PACK,
                           &Handle
                           );
  ASSERT_EFI_ERROR (Status);

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_STRINGS,
                           NULL,
                           NewOrAddPackNotify,
                           EFI_HII_DATABASE_NOTIFY_ADD_PACK,
                           &Handle
                           );
  ASSERT_EFI_ERROR (Status);

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_FORM,
                           NULL,
                           NewOrAddPackNotify,
                           EFI_HII_DATABASE_NOTIFY_NEW_PACK,
                           &Handle
                           );
  ASSERT_EFI_ERROR (Status);

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_FORM,
                           NULL,
                           NewOrAddPackNotify,
                           EFI_HII_DATABASE_NOTIFY_ADD_PACK,
                           &Handle
                           );
  ASSERT_EFI_ERROR (Status);

  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_STRINGS,
                           NULL,
                           RemovePackNotify,
                           EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                           &Handle
                           );
  ASSERT_EFI_ERROR (Status);

  mBrowserThunkPrivateDataTemplate.ThunkPrivate = Private;
  Status = gBS->InstallProtocolInterface (
                  &mBrowserThunkPrivateDataTemplate.Handle,
                  &gEfiFormBrowserCompatibilityProtocolGuid,
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
  LIST_ENTRY                                *Link;
  HII_THUNK_CONTEXT *ThunkContext;
  HII_THUNK_PRIVATE_DATA               *Private;

  if (HandleBufferLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);  

  //
  // Count the number of handles.
  //
  Count = 0;
  Link = GetFirstNode (&Private->ThunkContextListHead);
  while (!IsNull (&Private->ThunkContextListHead, Link)) {
    Count++;
    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  if (Count > *HandleBufferLength) {
    *HandleBufferLength = (Count * sizeof (FRAMEWORK_EFI_HII_HANDLE));
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Output the handles.
  //
  Count = 0;
  Link = GetFirstNode (&Private->ThunkContextListHead);
  while (!IsNull (&Private->ThunkContextListHead, Link)) {

    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);
    Handle[Count] = ThunkContext->FwHiiHandle;

    Count++;
    Link = GetNextNode (&Private->ThunkContextListHead, Link);

  }

  *HandleBufferLength = (Count * sizeof (FRAMEWORK_EFI_HII_HANDLE));
  return EFI_SUCCESS;
}

EFI_STATUS
LangCodes3066To639 (
  IN CHAR8 *LangCodes3066,
  IN CHAR8 **LangCodes639
  )
{
  CHAR8                      *AsciiLangCodes;
  CHAR8                      Lang[RFC_3066_ENTRY_SIZE];
  UINTN                      Index;
  UINTN                      Count;
  EFI_STATUS                 Status;

  ASSERT (LangCodes3066 != NULL);
  ASSERT (LangCodes639 != NULL);
  
  //
  // Count the number of RFC 3066 language codes.
  //
  Index = 0;
  AsciiLangCodes = LangCodes3066;
  while (AsciiStrLen (AsciiLangCodes) != 0) {
    HiiLibGetNextLanguage (&AsciiLangCodes, Lang);
    Index++;
  }

  Count = Index;

  //
  // 
  //
  *LangCodes639 = AllocateZeroPool (ISO_639_2_ENTRY_SIZE * Count + 1);
  if (*LangCodes639 == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  AsciiLangCodes = LangCodes3066;

  for (Index = 0; Index < Count; Index++) {
    HiiLibGetNextLanguage (&AsciiLangCodes, Lang);
    Status = ConvertRfc3066LanguageToIso639Language (Lang, *LangCodes639 + Index * ISO_639_2_ENTRY_SIZE);
    ASSERT_EFI_ERROR (Status);
  }

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
  HII_THUNK_PRIVATE_DATA     *Private;
  EFI_HII_HANDLE             UefiHiiHandle;
  CHAR8                      *LangCodes3066;
  CHAR16                     *UnicodeLangCodes639;
  CHAR8                      *LangCodes639;
  EFI_STATUS                 Status;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  UefiHiiHandle = FwHiiHandleToUefiHiiHandle (Private, Handle);
  if (UefiHiiHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  LangCodes3066 = HiiLibGetSupportedLanguages (UefiHiiHandle);

  if (LangCodes3066 == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  LangCodes639 = NULL;
  Status = LangCodes3066To639 (LangCodes3066, &LangCodes639);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  UnicodeLangCodes639 = AllocateZeroPool (AsciiStrSize (LangCodes639) * sizeof (CHAR16));
  if (UnicodeLangCodes639 == NULL) {
    Status =  EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // The language returned is in RFC 639-2 format.
  //
  AsciiStrToUnicodeStr (LangCodes639, UnicodeLangCodes639);
  *LanguageString = UnicodeLangCodes639;

Done:
  FreePool (LangCodes3066);
  if (LangCodes639 != NULL) {
    FreePool (LangCodes639);
  }

  return Status;
}



EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL              *This,
  IN  FRAMEWORK_EFI_HII_HANDLE      Handle,
  IN  CHAR16                        *PrimaryLanguage,
  OUT EFI_STRING                    *LanguageString
  )
/*++

Routine Description:

  This function allows a program to determine which secondary languages are supported
  on a given handle for a given primary language.

  Arguments:

Returns:

--*/
{
  HII_THUNK_PRIVATE_DATA *Private;
  EFI_HII_HANDLE             UefiHiiHandle;
  CHAR8                      PrimaryLang3066[RFC_3066_ENTRY_SIZE];
  CHAR8                      *PrimaryLang639;
  CHAR8                      *SecLangCodes3066;
  CHAR8                      *SecLangCodes639;
  CHAR16                     *UnicodeSecLangCodes639;
  EFI_STATUS                 Status;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  SecLangCodes639         = NULL;
  SecLangCodes3066        = NULL;
  UnicodeSecLangCodes639 = NULL;

  UefiHiiHandle = FwHiiHandleToUefiHiiHandle (Private, Handle);
  if (UefiHiiHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrimaryLang639 = AllocateZeroPool (StrLen (PrimaryLanguage) + 1);
  if (PrimaryLang639 == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  UnicodeStrToAsciiStr (PrimaryLanguage, PrimaryLang639);

  Status = ConvertIso639LanguageToRfc3066Language (PrimaryLang639, PrimaryLang3066);
  ASSERT_EFI_ERROR (Status);

  SecLangCodes3066 = HiiLibGetSupportedSecondaryLanguages (UefiHiiHandle, PrimaryLang3066);

  if (SecLangCodes3066 == NULL) {
    Status =  EFI_INVALID_PARAMETER;
    goto Done;
  }

  Status = LangCodes3066To639 (SecLangCodes3066, &SecLangCodes639);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  UnicodeSecLangCodes639 = AllocateZeroPool (AsciiStrSize (SecLangCodes639) * sizeof (CHAR16));
  if (UnicodeSecLangCodes639 == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // The language returned is in RFC 3066 format.
  //
  *LanguageString = AsciiStrToUnicodeStr (SecLangCodes639, UnicodeSecLangCodes639);

Done:
  if (PrimaryLang639 != NULL) {
    FreePool (PrimaryLang639);
  }
  if (SecLangCodes639 != NULL) {
    FreePool (SecLangCodes639);
  }
  if (SecLangCodes3066 != NULL) {
    FreePool (SecLangCodes3066);
  }
  if (UnicodeSecLangCodes639 != NULL) {
    FreePool (UnicodeSecLangCodes639);
  }
  
  return Status;
}

 
