/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  HiiDatabase.c

Abstract:

  This file contains the entry code to the HII database.

--*/

#include "HiiDatabase.h"

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
  EFI_STATUS          Status;
  EFI_HII_DATA        *HiiData;
  EFI_HII_GLOBAL_DATA *GlobalData;
  EFI_HANDLE          *HandleBuffer;
  EFI_HANDLE          Handle;
  UINTN               HandleCount;
  UINTN               Index;

  //
  // There will be only one HII Database in the system
  // If there is another out there, someone is trying to install us
  // again.  Fail that scenario.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiHiiProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );

  //
  // If there was no error, assume there is an installation and fail to load
  //
  if (!EFI_ERROR (Status)) {
    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }

    return EFI_DEVICE_ERROR;
  }

  HiiData = AllocatePool (sizeof (EFI_HII_DATA));

  ASSERT (HiiData);

  GlobalData = AllocateZeroPool (sizeof (EFI_HII_GLOBAL_DATA));

  ASSERT (GlobalData);

  //
  // Seed the Font Database with a known non-character glyph
  //
  for (Index = 0; Index <= MAX_GLYPH_COUNT; Index++) {
    //
    // Seeding the UnicodeWeight with 0 signifies that it is uninitialized
    //
    GlobalData->NarrowGlyphs[Index].UnicodeWeight = 0;
    GlobalData->WideGlyphs[Index].UnicodeWeight   = 0;
    GlobalData->NarrowGlyphs[Index].Attributes    = 0;
    GlobalData->WideGlyphs[Index].Attributes      = 0;
    CopyMem (GlobalData->NarrowGlyphs[Index].GlyphCol1, &mUnknownGlyph, NARROW_GLYPH_ARRAY_SIZE);
    CopyMem (GlobalData->WideGlyphs[Index].GlyphCol1, &mUnknownGlyph, WIDE_GLYPH_ARRAY_SIZE);
  }
  //
  // Fill in HII data
  //
  HiiData->Signature                        = EFI_HII_DATA_SIGNATURE;
  HiiData->GlobalData                       = GlobalData;
  HiiData->GlobalData->SystemKeyboardUpdate = FALSE;
  HiiData->DatabaseHead                     = NULL;
  HiiData->Hii.NewPack                      = HiiNewPack;
  HiiData->Hii.RemovePack                   = HiiRemovePack;
  HiiData->Hii.FindHandles                  = HiiFindHandles;
  HiiData->Hii.ExportDatabase               = HiiExportDatabase;
  HiiData->Hii.GetGlyph                     = HiiGetGlyph;
  HiiData->Hii.GetPrimaryLanguages          = HiiGetPrimaryLanguages;
  HiiData->Hii.GetSecondaryLanguages        = HiiGetSecondaryLanguages;
  HiiData->Hii.NewString                    = HiiNewString;
  HiiData->Hii.GetString                    = HiiGetString;
  HiiData->Hii.ResetStrings                 = HiiResetStrings;
  HiiData->Hii.TestString                   = HiiTestString;
  HiiData->Hii.GetLine                      = HiiGetLine;
  HiiData->Hii.GetForms                     = HiiGetForms;
  HiiData->Hii.GetDefaultImage              = HiiGetDefaultImage;
  HiiData->Hii.UpdateForm                   = HiiUpdateForm;
  HiiData->Hii.GetKeyboardLayout            = HiiGetKeyboardLayout;
  HiiData->Hii.GlyphToBlt                   = HiiGlyphToBlt;

  //
  // Install protocol interface
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiHiiProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &HiiData->Hii
                  );

  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
EFIAPI
HiiFindHandles (
  IN     EFI_HII_PROTOCOL *This,
  IN OUT UINT16           *HandleBufferLength,
  OUT    EFI_HII_HANDLE   Handle[1]
  )
/*++

Routine Description:
  Determines the handles that are currently active in the database.
  
Arguments:

Returns: 

--*/
{
  EFI_HII_HANDLE_DATABASE *Database;
  EFI_HII_DATA            *HiiData;
  UINTN                   HandleCount;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData     = EFI_HII_DATA_FROM_THIS (This);

  Database    = HiiData->DatabaseHead;

  if (Database == NULL) {
    *HandleBufferLength = 0;
    return EFI_NOT_FOUND;
  }

  for (HandleCount = 0; Database != NULL; HandleCount++) {
    Database = Database->NextHandleDatabase;
  }
  //
  // Is there a sufficient buffer for the data being passed back?
  //
  if (*HandleBufferLength >= (sizeof (EFI_HII_HANDLE) * HandleCount)) {
    Database = HiiData->DatabaseHead;

    //
    // Copy the Head information
    //
    if (Database->Handle != 0) {
      CopyMem (&Handle[0], &Database->Handle, sizeof (EFI_HII_HANDLE));
      Database = Database->NextHandleDatabase;
    }
    //
    // Copy more data if appropriate
    //
    for (HandleCount = 1; Database != NULL; HandleCount++) {
      CopyMem (&Handle[HandleCount], &Database->Handle, sizeof (EFI_HII_HANDLE));
      Database = Database->NextHandleDatabase;
    }

    *HandleBufferLength = (UINT16) (sizeof (EFI_HII_HANDLE) * HandleCount);
    return EFI_SUCCESS;
  } else {
    //
    // Insufficient buffer length
    //
    *HandleBufferLength = (UINT16) (sizeof (EFI_HII_HANDLE) * HandleCount);
    return EFI_BUFFER_TOO_SMALL;
  }
}

EFI_STATUS
EFIAPI
HiiGetPrimaryLanguages (
  IN  EFI_HII_PROTOCOL      *This,
  IN  EFI_HII_HANDLE        Handle,
  OUT EFI_STRING            *LanguageString
  )
/*++

Routine Description:
  
  This function allows a program to determine what the primary languages that are supported on a given handle.

Arguments:

Returns: 

--*/
{
  UINTN                     Count;
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_PACKAGE_INSTANCE  *StringPackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_STRING_PACK       *StringPack;
  EFI_HII_STRING_PACK       *Location;
  UINT32                    Length;
  RELOFST                   Token;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  PackageInstance = NULL;
  //
  // Find matching handle in the handle database. Then get the package instance.
  //
  for (HandleDatabase = HiiData->DatabaseHead;
       HandleDatabase != NULL;
       HandleDatabase = HandleDatabase->NextHandleDatabase
      ) {
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ValidatePack (This, PackageInstance, &StringPackageInstance, NULL);

  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (StringPackageInstance->IfrSize > 0) {
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (&StringPackageInstance->IfrData) + StringPackageInstance->IfrSize);
  } else {
    StringPack = (EFI_HII_STRING_PACK *) (&StringPackageInstance->IfrData);
  }

  Location = StringPack;
  //
  // Remember that the string packages are formed into contiguous blocks of language data.
  //
  CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  for (Count = 0; Length != 0; Count = Count + 3) {
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + Length);
    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  }

  *LanguageString = AllocateZeroPool (2 * (Count + 1));

  ASSERT (*LanguageString);

  StringPack = (EFI_HII_STRING_PACK *) Location;

  //
  // Copy the 6 bytes to LanguageString - keep concatenating it.  Shouldn't we just store uint8's since the ISO
  // standard defines the lettering as all US English characters anyway?  Save a few bytes.
  //
  CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  for (Count = 0; Length != 0; Count = Count + 3) {
    CopyMem (&Token, &StringPack->LanguageNameString, sizeof (RELOFST));
    CopyMem (*LanguageString + Count, (VOID *) ((CHAR8 *) (StringPack) + Token), 6);
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + Length);
    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiGetSecondaryLanguages (
  IN  EFI_HII_PROTOCOL      *This,
  IN  EFI_HII_HANDLE        Handle,
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
  UINTN                     Count;
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_PACKAGE_INSTANCE  *StringPackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_STRING_PACK       *StringPack;
  RELOFST                   Token;
  UINT32                    Length;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData = EFI_HII_DATA_FROM_THIS (This);
  //
  // Check numeric value against the head of the database
  //
  PackageInstance = NULL;
  for (HandleDatabase = HiiData->DatabaseHead;
       HandleDatabase != NULL;
       HandleDatabase = HandleDatabase->NextHandleDatabase
      ) {
    //
    // Match the numeric value with the database entry - if matched, extract PackageInstance
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ValidatePack (This, PackageInstance, &StringPackageInstance, NULL);

  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (StringPackageInstance->IfrSize > 0) {
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (&StringPackageInstance->IfrData) + StringPackageInstance->IfrSize);
  } else {
    StringPack = (EFI_HII_STRING_PACK *) (&StringPackageInstance->IfrData);
  }

  //
  // Remember that the string packages are formed into contiguous blocks of language data.
  //
  for (; StringPack->Header.Length != 0;) {
    //
    // Find the PrimaryLanguage being requested
    //
    Token = StringPack->LanguageNameString;
    if (CompareMem ((VOID *) ((CHAR8 *) (StringPack) + Token), PrimaryLanguage, 3) == 0) {
      //
      // Now that we found the primary, the secondary languages will follow immediately
      // or the next character is a NULL if there are no secondary languages.  We determine
      // the number by getting the stringsize based on the StringPack origination + the LanguageNameString
      // offset + 6 (which is the size of the first 3 letter ISO primary language name).  If we get 2, there
      // are no secondary languages (2 = null-terminator).
      //
      Count           = StrSize ((VOID *) ((CHAR8 *) (StringPack) + Token + 6));

      *LanguageString = AllocateZeroPool (2 * (Count + 1));

      ASSERT (*LanguageString);

      CopyMem (*LanguageString, (VOID *) ((CHAR8 *) (StringPack) + Token + 6), Count);
      break;
    }

    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + Length);
  }

  return EFI_SUCCESS;
}

