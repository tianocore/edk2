/**@file
  This file implements the protocol functions related to string package.
  
Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

typedef struct {
  CHAR8 *Iso639;
  CHAR8 *Rfc3066;
} ISO639TORFC3066MAP;

ISO639TORFC3066MAP Iso639ToRfc3066Map [] = {
    {"eng", "en-US"},
    {"fra", "fr-FR"},
};

CHAR8 *
ConvertIso639ToRfc3066 (
  CHAR8 *Iso638Lang
  )
{
  UINTN Index;

  for (Index = 0; Index < sizeof (Iso639ToRfc3066Map) / sizeof (Iso639ToRfc3066Map[0]); Index++) {
    if (AsciiStrnCmp (Iso638Lang, Iso639ToRfc3066Map[Index].Iso639, AsciiStrSize (Iso638Lang)) == 0) {
      return Iso639ToRfc3066Map[Index].Rfc3066;
    }
  }

  return (CHAR8 *) NULL;
}


EFI_STATUS
EFIAPI
HiiTestString (
  IN     EFI_HII_PROTOCOL   *This,
  IN     CHAR16             *StringToTest,
  IN OUT UINT32             *FirstMissing,
  OUT    UINT32             *GlyphBufferSize
  )
/*++

Routine Description:
  Test if all of the characters in a string have corresponding font characters.

Arguments:

Returns:

--*/
{
  ASSERT (FALSE);
  return EFI_SUCCESS;
}



EFI_STATUS
GetTagGuidByFwHiiHandle (
  IN  CONST HII_THUNK_PRIVATE_DATA      *Private,
  IN        FRAMEWORK_EFI_HII_HANDLE    FwHiiHandle,
  OUT       EFI_GUID                    *TagGuid
  )
{
  LIST_ENTRY                                *Link;
  HII_THUNK_CONTEXT                          *ThunkContext;

  ASSERT (TagGuid != NULL);

  Link = GetFirstNode (&Private->ThunkContextListHead);
  while (!IsNull (&Private->ThunkContextListHead, Link)) {

    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (FwHiiHandle == ThunkContext->FwHiiHandle) {
      CopyGuid (TagGuid, &ThunkContext->TagGuid);
      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  return EFI_NOT_FOUND;
}



EFI_STATUS
EFIAPI
HiiNewString (
  IN     EFI_HII_PROTOCOL           *This,
  IN     CHAR16                     *Language,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN OUT STRING_REF                 *Reference,
  IN     CHAR16                     *NewString
  )
/*++

Routine Description:
  This function allows a new String to be added to an already existing String Package.
  We will make a buffer the size of the package + StrSize of the new string.  We will
  copy the string package that first gets changed and the following language packages until
  we encounter the NULL string package.  All this time we will ensure that the offsets have
  been adjusted.

Arguments:

Returns:

--*/
{
  EFI_STATUS                                Status;
  HII_THUNK_PRIVATE_DATA                    *Private;
  EFI_GUID                                  TagGuid;
  LIST_ENTRY                                *Link;
  HII_THUNK_CONTEXT                          *ThunkContext;
  EFI_STRING_ID                             StringId;
  EFI_STRING_ID                             LastStringId;
  CHAR8                                     AsciiLanguage[ISO_639_2_ENTRY_SIZE + 1];
  CHAR16                                    LanguageCopy[ISO_639_2_ENTRY_SIZE + 1];
  BOOLEAN                                   Found;
  CHAR8                                     *Rfc3066AsciiLanguage;

  LastStringId      = (EFI_STRING_ID) 0;
  StringId          = (EFI_STRING_ID) 0;
  Found             = FALSE;
  Rfc3066AsciiLanguage = NULL;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  Status = GetTagGuidByFwHiiHandle (Private, Handle, &TagGuid);
  ASSERT_EFI_ERROR (Status);

  if (Language != NULL) {
    ZeroMem (AsciiLanguage, sizeof (AsciiLanguage));;
    ZeroMem (LanguageCopy, sizeof (LanguageCopy));
    CopyMem (LanguageCopy, Language, ISO_639_2_ENTRY_SIZE * sizeof (CHAR16));
    UnicodeStrToAsciiStr (LanguageCopy, AsciiLanguage);
    Rfc3066AsciiLanguage = ConvertIso639ToRfc3066 (AsciiLanguage);
    ASSERT (Rfc3066AsciiLanguage != NULL);
  }

  Link = GetFirstNode (&Private->ThunkContextListHead);
  while (!IsNull (&Private->ThunkContextListHead, Link)) {
    ThunkContext = HII_THUNK_CONTEXT_FROM_LINK (Link);

    if (CompareGuid (&TagGuid, &ThunkContext->TagGuid)) {
      Found = TRUE;
      if (*Reference == 0) {
        //
        // Create a new string token.
        //
        if (Rfc3066AsciiLanguage == NULL) {
          //
          // For all languages in the package list.
          //
          Status = HiiLibNewString (ThunkContext->UefiHiiHandle, &StringId, NewString);
        } else {
          //
          // For specified language.
          //
          Status = mHiiStringProtocol->NewString (
                                         mHiiStringProtocol,
                                         ThunkContext->UefiHiiHandle,
                                         &StringId,
                                         Rfc3066AsciiLanguage,
                                         NULL,
                                         NewString,
                                         NULL
                                         );
        }
      } else {
        //
        // Update the existing string token.
        //
        if (Rfc3066AsciiLanguage == NULL) {
          //
          // For all languages in the package list.
          //
          Status = HiiLibSetString (ThunkContext->UefiHiiHandle, *Reference, NewString);
        } else {
          //
          // For specified language.
          //
          Status = mHiiStringProtocol->SetString (
                                       mHiiStringProtocol,
                                       ThunkContext->UefiHiiHandle,
                                       *Reference,
                                       Rfc3066AsciiLanguage,
                                       NewString,
                                       NULL
                                       );
        }
      }
      if (EFI_ERROR (Status)) {
        //
        // Only EFI_INVALID_PARAMETER is defined in HII 0.92 specification.
        //
        return EFI_INVALID_PARAMETER;
      }

      if (*Reference == 0) {
        //
        // When creating new string token, make sure all created token is the same
        // for all string packages registered using FW HII interface.
        //
        if (LastStringId == (EFI_STRING_ID) 0) {
          LastStringId = StringId;
        } else {
          if (LastStringId != StringId) {
            ASSERT(FALSE);
            return EFI_INVALID_PARAMETER;
          }
        }
      }
    }

    Link = GetNextNode (&Private->ThunkContextListHead, Link);
  }

  if (Found) {
    if (*Reference == 0) {
      *Reference = StringId;
    }
    Status = EFI_SUCCESS;
  } else {
    DEBUG((EFI_D_ERROR, "Thunk HiiNewString fails to find the String Packages to update\n"));
    //
    // BUGBUG: Remove ths ASSERT when development is done.
    //
    ASSERT (FALSE);
    Status = EFI_NOT_FOUND;
  }
  
  //
  // For UNI file, some String may not be defined for a language. This has been true for a lot of platform code.
  // For this case, EFI_NOT_FOUND will be returned. To allow the old code to be run without porting, we don't assert 
  // on EFI_NOT_FOUND. The missing Strings will be shown if user select a differnt languages other than the default
  // English language for the platform.
  //
  ASSERT_EFI_ERROR (EFI_ERROR (Status) && Status != EFI_NOT_FOUND);  

  return Status;
}

EFI_STATUS
EFIAPI
HiiResetStrings (
  IN     EFI_HII_PROTOCOL   *This,
  IN     FRAMEWORK_EFI_HII_HANDLE      Handle
  )
/*++

Routine Description:

    This function removes any new strings that were added after the initial string export for this handle.

Arguments:

Returns:

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiGetString (
  IN     EFI_HII_PROTOCOL           *This,
  IN     FRAMEWORK_EFI_HII_HANDLE   Handle,
  IN     STRING_REF                 Token,
  IN     BOOLEAN                    Raw,
  IN     CHAR16                     *LanguageString,
  IN OUT UINTN                      *BufferLengthTemp,
  OUT    EFI_STRING                 StringBuffer
  )
/*++

Routine Description:

  This function extracts a string from a package already registered with the EFI HII database.

Arguments:
  This            - A pointer to the EFI_HII_PROTOCOL instance.
  Handle          - The HII handle on which the string resides.
  Token           - The string token assigned to the string.
  Raw             - If TRUE, the string is returned unedited in the internal storage format described
                    above. If false, the string returned is edited by replacing <cr> with <space>
                    and by removing special characters such as the <wide> prefix.
  LanguageString  - Pointer to a NULL-terminated string containing a single ISO 639-2 language
                    identifier, indicating the language to print. If the LanguageString is empty (starts
                    with a NULL), the default system language will be used to determine the language.
  BufferLength    - Length of the StringBuffer. If the status reports that the buffer width is too
                    small, this parameter is filled with the length of the buffer needed.
  StringBuffer    - The buffer designed to receive the characters in the string. Type EFI_STRING is
                    defined in String.

Returns:
  EFI_INVALID_PARAMETER - If input parameter is invalid.
  EFI_BUFFER_TOO_SMALL  - If the *BufferLength is too small.
  EFI_SUCCESS           - Operation is successful.

--*/
{
  CHAR8                                 *Iso639AsciiLanguage;
  HII_THUNK_PRIVATE_DATA                *Private;
  CHAR8                                 *Rfc3066AsciiLanguage;
  EFI_HII_HANDLE                        UefiHiiHandle;
  EFI_STATUS                            Status;

  Private = HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  Iso639AsciiLanguage = NULL;
  Rfc3066AsciiLanguage = NULL;

  if (LanguageString != NULL) {
    Iso639AsciiLanguage = AllocateZeroPool (StrLen (LanguageString) + 1);
    if (Iso639AsciiLanguage == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    UnicodeStrToAsciiStr  (LanguageString, Iso639AsciiLanguage);

    //
    // Caller of Framework HII Interface uses the Language Identification String defined 
    // in Iso639. So map it to the Language Identifier defined in RFC3066.
    //
    Rfc3066AsciiLanguage = ConvertIso639ToRfc3066 (Iso639AsciiLanguage);

    //
    // If Rfc3066AsciiLanguage is NULL, more language mapping must be added to 
    // Iso639ToRfc3066Map.
    //
    ASSERT (Rfc3066AsciiLanguage != NULL);
    
  }

  UefiHiiHandle = FwHiiHandleToUefiHiiHandle (Private, Handle);
  if (UefiHiiHandle == NULL) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  if (Rfc3066AsciiLanguage == NULL) {
    Status =  HiiLibGetString (UefiHiiHandle, Token, StringBuffer, BufferLengthTemp);
  } else {
    Status = mHiiStringProtocol->GetString (
                                 mHiiStringProtocol,
                                 Rfc3066AsciiLanguage,
                                 UefiHiiHandle,
                                 Token,
                                 StringBuffer,
                                 BufferLengthTemp,
                                 NULL
                                 );
  }

Done:
  SafeFreePool (Iso639AsciiLanguage);
  
  return Status;
}

EFI_STATUS
EFIAPI
HiiGetLine (
  IN     EFI_HII_PROTOCOL   *This,
  IN     FRAMEWORK_EFI_HII_HANDLE      Handle,
  IN     STRING_REF         Token,
  IN OUT UINT16             *Index,
  IN     UINT16             LineWidth,
  IN     CHAR16             *LanguageString,
  IN OUT UINT16             *BufferLength,
  OUT    EFI_STRING         StringBuffer
  )
/*++

Routine Description:

  This function allows a program to extract a part of a string of not more than a given width.
  With repeated calls, this allows a calling program to extract "lines" of text that fit inside
  columns.  The effort of measuring the fit of strings inside columns is localized to this call.

Arguments:

Returns:

--*/
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}


