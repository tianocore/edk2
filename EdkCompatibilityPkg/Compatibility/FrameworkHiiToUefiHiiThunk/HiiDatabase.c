/**@file

Framework to UEFI 2.1 HII Thunk

Copyright (c) 2003, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"


EFI_HII_THUNK_PRIVATE_DATA HiiThunkPrivateDataTempate = {
  {//Signature
    EFI_HII_THUNK_DRIVER_DATA_SIGNATURE 
  },
  {//Handle
    (EFI_HANDLE) NULL
  },
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
  { //StaticHiiHandle
    //The FRAMEWORK_EFI_HII_HANDLE starts from 1 
    // and increase upwords untill reach 2^(sizeof (FRAMEWORK_EFI_HII_HANDLE)) - 1. 
    // The code will assert to prevent overflow.
    (FRAMEWORK_EFI_HII_HANDLE) 1 
  },
  {
    NULL, NULL                  //HiiHandleLinkList
  },
};

EFI_HII_DATABASE_PROTOCOL *mUefiHiiDatabaseProtocol;
EFI_HII_FONT_PROTOCOL     *mUefiHiiFontProtocol;
EFI_HII_IMAGE_PROTOCOL    *mUefiHiiImageProtocol;
EFI_HII_STRING_PROTOCOL   *mUefiStringProtocol;

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

  HiiData = AllocateCopyPool (sizeof (EFI_HII_THUNK_PRIVATE_DATA), &HiiThunkPrivateDataTempate);
  ASSERT (HiiData != NULL);
  InitializeListHead (&HiiData->HiiThunkHandleMappingDBListHead);

  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **) &mUefiHiiDatabaseProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiFontProtocolGuid,
                  NULL,
                  (VOID **) &mUefiHiiFontProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiImageProtocolGuid,
                  NULL,
                  (VOID **) &mUefiHiiImageProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **) &mUefiStringProtocol
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
  ASSERT (FALSE);
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
  ASSERT (FALSE);
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
  ASSERT (FALSE);
  return EFI_SUCCESS;
}


