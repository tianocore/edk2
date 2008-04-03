/**@file

  This file contains the keyboard processing code to the HII database.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

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
GetTagGuidByFrameworkHiiHandle (
  IN  CONST EFI_HII_THUNK_PRIVATE_DATA  *Private,
  IN        FRAMEWORK_EFI_HII_HANDLE    FrameworkHiiHandle,
  OUT       EFI_GUID                    *TagGuid
  )
{
  LIST_ENTRY                                *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *HandleMapEntry;

  ASSERT (TagGuid != NULL);

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {

    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    if (FrameworkHiiHandle == HandleMapEntry->FrameworkHiiHandle) {
      CopyGuid (TagGuid, &HandleMapEntry->TagGuid);
      return EFI_SUCCESS;
    }
  }
  
  return EFI_NOT_FOUND;
}

EFI_STATUS
HiiThunkNewStringForAllStringPackages (
  IN  CONST EFI_HII_THUNK_PRIVATE_DATA  *Private,
  OUT CONST EFI_GUID                    *TagGuid,
  IN        CHAR16                     *Language,
  IN OUT    STRING_REF                 *Reference,
  IN        CHAR16                     *NewString
  )
{
  EFI_STATUS                                Status;
  LIST_ENTRY                                *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *HandleMapEntry;
  EFI_STRING_ID                             StringId1;
  EFI_STRING_ID                             StringId2;
  CHAR8                                     *UefiStringProtocolLanguage;
  BOOLEAN                                   Found;

  ASSERT (TagGuid != NULL);

  StringId1 = (EFI_STRING_ID) 0;
  StringId2 = (EFI_STRING_ID) 0;
  Found = FALSE;

  //
  // BugBug: We will handle the case that Language is not NULL later.
  //
  ASSERT (Language == NULL);
  
  //if (Language == NULL) {
    UefiStringProtocolLanguage = NULL;
  //}

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {

    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    if (CompareGuid (TagGuid, &HandleMapEntry->TagGuid)) {
      Found = TRUE;
      if (*Reference == 0) {
        Status = HiiLibNewString (HandleMapEntry->UefiHiiHandle, &StringId2, NewString);
      } else {
        Status = HiiLibSetString (HandleMapEntry->UefiHiiHandle, *Reference, NewString);
      }
      if (EFI_ERROR (Status)) {
        return Status;
      }
      if (*Reference == 0) {
        if (StringId1 == (EFI_STRING_ID) 0) {
          StringId1 = StringId2;
        } else {
          if (StringId1 != StringId2) {
            ASSERT(FALSE);
            return EFI_INVALID_PARAMETER;
          }
        }
      }
    }
  }

  if (Found) {
    *Reference = StringId1;
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_NOT_FOUND;
  }
  
  return Status;
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
  EFI_STATUS                 Status;
  EFI_HII_THUNK_PRIVATE_DATA *Private;
  EFI_GUID                   TagGuid;

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  Status = GetTagGuidByFrameworkHiiHandle (Private, Handle, &TagGuid);
  ASSERT_EFI_ERROR (Status);

  Status = HiiThunkNewStringForAllStringPackages (Private, &TagGuid, Language, Reference, NewString);
  ASSERT_EFI_ERROR (Status);  

  return EFI_SUCCESS;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HiiGetString (
  IN     EFI_HII_PROTOCOL    *This,
  IN     FRAMEWORK_EFI_HII_HANDLE       Handle,
  IN     STRING_REF          Token,
  IN     BOOLEAN             Raw,
  IN     CHAR16              *LanguageString,
  IN OUT UINTN               *BufferLengthTemp,
  OUT    EFI_STRING          StringBuffer
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
  LIST_ENTRY                                *ListEntry;
  HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY  *HandleMapEntry;
  CHAR8                                     *AsciiLanguage;
  EFI_HII_THUNK_PRIVATE_DATA                *Private;

  Private = EFI_HII_THUNK_PRIVATE_DATA_FROM_THIS(This);

  if (LanguageString == NULL) {
    AsciiLanguage = NULL;
  } else {
    AsciiLanguage = AllocateZeroPool (StrLen (LanguageString) + 1);
    if (AsciiLanguage == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    UnicodeStrToAsciiStr  (LanguageString, AsciiLanguage);
  }

  for (ListEntry = Private->HiiThunkHandleMappingDBListHead.ForwardLink;
       ListEntry != &Private->HiiThunkHandleMappingDBListHead;
       ListEntry = ListEntry->ForwardLink
       ) {

    HandleMapEntry = HII_TRHUNK_HANDLE_MAPPING_DATABASE_ENTRY_FROM_LISTENTRY (ListEntry);

    if (Handle == HandleMapEntry->FrameworkHiiHandle) {
      if (AsciiLanguage == NULL) {
        return HiiLibGetString (HandleMapEntry->UefiHiiHandle, Token, StringBuffer, BufferLengthTemp);
      } else {
        return mUefiStringProtocol->GetString (
                                     mUefiStringProtocol,
                                     AsciiLanguage,
                                     HandleMapEntry->UefiHiiHandle,
                                     Token,
                                     StringBuffer,
                                     BufferLengthTemp,
                                     NULL
                                     );
      }
    }
  }

  return EFI_NOT_FOUND;
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

