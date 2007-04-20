/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Strings.c

Abstract:

  This file contains the string processing code to the HII database.

--*/


#include "HiiDatabase.h"

STATIC
VOID
AsciiToUnicode (
  IN    UINT8     *Lang,
  IN    UINT16    *Language
  )
{
  UINT8 Count;

  //
  // Convert the ASCII Lang variable to a Unicode Language variable
  //
  for (Count = 0; Count < 3; Count++) {
    Language[Count] = (CHAR16) Lang[Count];
  }
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
  EFI_HII_GLOBAL_DATA *GlobalData;
  EFI_HII_DATA        *HiiData;
  BOOLEAN             WideChar;
  INT32               Location;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData     = EFI_HII_DATA_FROM_THIS (This);
  GlobalData  = HiiData->GlobalData;

  
  //
  // Rewind through the string looking for a glyph width identifier
  // If no width identifier exists, we assume string has narrow width identifier
  //
  for (WideChar = FALSE, Location = (INT32) *FirstMissing; Location >= 0; Location--) {
    if ((StringToTest[Location] == NARROW_CHAR) || (StringToTest[Location] == WIDE_CHAR)) {
      //
      // We found something that identifies what glyph database to look in
      //
      WideChar = (BOOLEAN) (StringToTest[Location] == WIDE_CHAR);
      break;
    }
  }

  //
  // Walk through the string until you hit the null terminator
  //
  for (*GlyphBufferSize = 0; StringToTest[*FirstMissing] != CHAR_NULL; (*FirstMissing)++) {
    //
    // We found something that identifies what glyph database to look in
    //
    if ((StringToTest[*FirstMissing] == NARROW_CHAR) || (StringToTest[*FirstMissing] == WIDE_CHAR)) {
      WideChar = (BOOLEAN) (StringToTest[*FirstMissing] == WIDE_CHAR);
      continue;
    }

    if (!WideChar) {
      if (CompareMem (
          GlobalData->NarrowGlyphs[StringToTest[*FirstMissing]].GlyphCol1,
          &mUnknownGlyph,
          NARROW_GLYPH_ARRAY_SIZE
          ) == 0
          ) {
        //
        // Break since this glyph isn't defined
        //
        return EFI_NOT_FOUND;
      }
    } else {
      //
      // Can compare wide glyph against only GlyphCol1 since GlyphCol1 and GlyphCol2 are contiguous - just give correct size
      //
      if (CompareMem (
          GlobalData->WideGlyphs[StringToTest[*FirstMissing]].GlyphCol1,
          &mUnknownGlyph,
          WIDE_GLYPH_ARRAY_SIZE
          ) == 0
          ) {
        //
        // Break since this glyph isn't defined
        //
        return EFI_NOT_FOUND;
      }
    }

    *GlyphBufferSize += (WideChar ? sizeof (EFI_WIDE_GLYPH) : sizeof (EFI_NARROW_GLYPH));
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
HiiNewString2 (
  IN     EFI_HII_PROTOCOL       *This,
  IN     CHAR16                 *Language,
  IN     EFI_HII_HANDLE         Handle,
  IN OUT STRING_REF             *Reference,
  IN     CHAR16                 *NewString,
  IN     BOOLEAN                ResetStrings
  )
/*++

Routine Description:

  This function allows a new String to be added to an already existing String Package.
  We will make a buffer the size of the package + EfiStrSize of the new string.  We will
  copy the string package that first gets changed and the following language packages until
  we encounter the NULL string package.  All this time we will ensure that the offsets have
  been adjusted.  

Arguments:
  
  This         -  Pointer to the HII protocol.
  Language     -  Pointer to buffer which contains the language code of this NewString.
  Handle       -  Handle of the package instance to be processed.
  Reference    -  The token number for the string. If 0, new string token to be returned through this parameter.
  NewString    -  Buffer pointer for the new string. 
  ResetStrings -  Indicate if we are resetting a string.
  
Returns: 

  EFI_SUCCESS            - The string has been added or reset to Hii database.
  EFI_INVALID_PARAMETER  - Some parameter passed in is invalid.

--*/
{
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_PACKAGE_INSTANCE  *StringPackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_STRING_PACK       *StringPack;
  EFI_HII_STRING_PACK       *NewStringPack;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_PACKAGE_INSTANCE  *NewBuffer;
  UINT8                     *Location;
  UINT8                     *StringLocation;
  RELOFST                   *StringPointer;
  UINTN                     Count;
  UINTN                     Size;
  UINTN                     Index;
  UINTN                     SecondIndex;
  BOOLEAN                   AddString;
  EFI_STATUS                Status;
  UINTN                     Increment;
  UINTN                     StringCount;
  UINT32                    TotalStringCount;
  UINT32                    OriginalStringCount;
  RELOFST                   StringSize;
  UINT32                    Length;
  RELOFST                   Offset;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData             = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase      = HiiData->DatabaseHead;
  PackageInstance     = NULL;
  AddString           = FALSE;
  Increment           = 0;
  StringCount         = 0;
  TotalStringCount    = 0;
  OriginalStringCount = 0;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
    //
    // Match the numeric value with the database entry - if matched, extract PackageInstance
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      if (ResetStrings) {
        TotalStringCount = HandleDatabase->NumberOfTokens;
      }
      break;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidatePack (This, PackageInstance, &StringPackageInstance, &TotalStringCount);

  //
  // This sets Count to 0 or the size of the IfrData.  We intend to use Count as an offset value
  //
  Count = StringPackageInstance->IfrSize;

  //
  // This is the size of the complete series of string packs
  //
  Size = StringPackageInstance->StringSize;

  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (StringPackageInstance->IfrSize > 0) {
    Location = (UINT8 *) (&StringPackageInstance->IfrData) + StringPackageInstance->IfrSize;
  } else {
    Location = (UINT8 *) (&StringPackageInstance->IfrData);
  }
  //
  // We allocate a buffer which is big enough for both adding and resetting string.
  // The size is slightly larger than the real size of the packages when we are resetting a string.
  //
  NewBuffer = AllocateZeroPool (
                sizeof (EFI_HII_PACKAGE_INSTANCE) -
                2 * sizeof (VOID *) +
                StringPackageInstance->IfrSize +
                StringPackageInstance->StringSize +
                sizeof (RELOFST) +
                StrSize (NewString)
                );
  ASSERT (NewBuffer);

  //
  // Copy data to new buffer
  //
  NewBuffer->Handle   = StringPackageInstance->Handle;
  NewBuffer->IfrSize  = StringPackageInstance->IfrSize;

  //
  // The worst case scenario for sizing is that we are adding a new string (not replacing one) and there was not a string
  // package to begin with.
  //
  NewBuffer->StringSize = StringPackageInstance->StringSize + StrSize (NewString) + sizeof (EFI_HII_STRING_PACK);

  if (StringPackageInstance->IfrSize > 0) {
    CopyMem (&NewBuffer->IfrData, &StringPackageInstance->IfrData, StringPackageInstance->IfrSize);
  }

  StringPack = (EFI_HII_STRING_PACK *) Location;

  //
  // There may be multiple instances packed together of strings
  // so we must walk the self describing structures until we encounter
  // what we are looking for.  In the meantime, copy everything we encounter
  // to the new buffer.
  //
  CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  for (; Length != 0;) {
    //
    // If passed in Language ISO value is in this string pack's language string
    // then we are dealing with the strings we want.
    //
    CopyMem (&Offset, &StringPack->LanguageNameString, sizeof (RELOFST));
    Status = HiiCompareLanguage ((CHAR16 *) ((CHAR8 *) (StringPack) + Offset), Language);

    if (!EFI_ERROR (Status)) {
      break;
    }

    CopyMem (((CHAR8 *) (&NewBuffer->IfrData) + Count), StringPack, Length);

    Count       = Count + Length;
    StringPack  = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + Length);
    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  }
  //
  // Found the language pack to update on a particular handle
  // We need to Copy the Contents of this pack and adjust the offset values associated
  // with adding/changing a string.  This is a particular piece of code that screams for
  // it being prone to programming error.
  //
  //
  // Copy the string package up to the string data
  //
  StringPointer = (RELOFST *) (StringPack + 1);
  CopyMem (
    ((CHAR8 *) (&NewBuffer->IfrData) + Count),
    StringPack,
    (UINTN) ((UINTN) (StringPointer) - (UINTN) (StringPack))
    );

  //
  // Determine the number of StringPointers
  //
  if (!ResetStrings) {
    CopyMem (&TotalStringCount, &StringPack->NumStringPointers, sizeof (RELOFST));
  } else {
    //
    // If we are resetting the strings, use the original value when exported
    //
    CopyMem (&OriginalStringCount, &StringPack->NumStringPointers, sizeof (RELOFST));
    ((EFI_HII_STRING_PACK *) ((CHAR8 *) (&NewBuffer->IfrData) + Count))->LanguageNameString -=
      (
        (RELOFST) (OriginalStringCount - TotalStringCount) *
        sizeof (RELOFST)
      );
    ((EFI_HII_STRING_PACK *) ((CHAR8 *) (&NewBuffer->IfrData) + Count))->PrintableLanguageName -=
      (
        (RELOFST) (OriginalStringCount - TotalStringCount) *
        sizeof (RELOFST)
      );
    ((EFI_HII_STRING_PACK *) ((CHAR8 *) (&NewBuffer->IfrData) + Count))->NumStringPointers  = TotalStringCount;
    *Reference = (STRING_REF) (TotalStringCount);
  }
  //
  // If the token value is not valid, error out
  //
  if ((*Reference >= TotalStringCount) && !ResetStrings) {
    FreePool (NewBuffer);
    return EFI_INVALID_PARAMETER;
  }
  //
  // If Reference is 0, update it with what the new token reference will be and turn the AddString flag on
  //
  if (*Reference == 0) {
    *Reference  = (STRING_REF) (TotalStringCount);
    AddString   = TRUE;
  }

  if (AddString) {
    ((EFI_HII_STRING_PACK *) ((CHAR8 *) (&NewBuffer->IfrData) + Count))->LanguageNameString += sizeof (RELOFST);
    ((EFI_HII_STRING_PACK *) ((CHAR8 *) (&NewBuffer->IfrData) + Count))->PrintableLanguageName += sizeof (RELOFST);
    ((EFI_HII_STRING_PACK *) ((CHAR8 *) (&NewBuffer->IfrData) + Count))->NumStringPointers++;
  }
  //
  // Increment offset by amount of copied data
  //
  Count = Count + ((UINTN) (StringPointer) - (UINTN) StringPack);

  for (Index = 0; Index < TotalStringCount; Index++) {
    //
    // If we are pointing to the size of the changing string value
    // then cache the old string value so you know what the difference is
    //
    if (Index == *Reference) {
      CopyMem (&Offset, &StringPointer[Index], sizeof (RELOFST));

      StringLocation = ((UINT8 *) (StringPack) + Offset);
      for (SecondIndex = 0;
           (StringLocation[SecondIndex] != 0) || (StringLocation[SecondIndex + 1] != 0);
           SecondIndex = SecondIndex + 2
          )
        ;
      SecondIndex = SecondIndex + 2;

      Size        = SecondIndex;

      //
      // NewString is a passed in local string which is assumed to be aligned
      //
      Size = StrSize (NewString) - Size;
    }
    //
    // If we are about to copy the offset of the string that follows the changed string make
    // sure that the offsets are adjusted accordingly
    //
    if ((Index > *Reference) && !ResetStrings) {
      CopyMem (&Offset, &StringPointer[Index], sizeof (RELOFST));
      Offset = (RELOFST) (Offset + Size);
      CopyMem (&StringPointer[Index], &Offset, sizeof (RELOFST));
    }
    //
    // If we are adding a string that means we will have an extra string pointer that will affect all string offsets
    //
    if (AddString) {
      CopyMem (&Offset, &StringPointer[Index], sizeof (RELOFST));
      Offset = (UINT32) (Offset + sizeof (RELOFST));
      CopyMem (&StringPointer[Index], &Offset, sizeof (RELOFST));
    }
    //
    // If resetting the strings, we need to reduce the offset by the difference in the strings
    //
    if (ResetStrings) {
      CopyMem (&Length, &StringPointer[Index], sizeof (RELOFST));
      Length = Length - ((RELOFST) (OriginalStringCount - TotalStringCount) * sizeof (RELOFST));
      CopyMem (&StringPointer[Index], &Length, sizeof (RELOFST));
    }
    //
    // Notice that if the string was being added as a new token, we don't have to worry about the
    // offsets changing in the other indexes
    //
    CopyMem (((CHAR8 *) (&NewBuffer->IfrData) + Count), &StringPointer[Index], sizeof (RELOFST));
    Count = Count + sizeof (RELOFST);
    StringCount++;
  }
  //
  // If we are adding a new string the above for loop did not copy the offset for us
  //
  if (AddString) {
    //
    // Since the Index is pointing to the beginning of the first string, we need to gather the size of the previous
    // offset's string and create an offset to our new string.
    //
    CopyMem (&Offset, &StringPointer[Index - 1], sizeof (RELOFST));
    StringLocation  = (UINT8 *) StringPack;
    StringLocation  = StringLocation + Offset - sizeof (RELOFST);

    //
    // Since StringPack is a packed structure, we need to size it carefully (byte-wise) to avoid alignment issues
    //
    for (Length = 0;
         (StringLocation[Length] != 0) || (StringLocation[Length + 1] != 0);
         Length = (RELOFST) (Length + 2)
        )
      ;
    Length      = (RELOFST) (Length + 2);

    StringSize  = (RELOFST) (Offset + Length);

    //
    // Copy the new string offset
    //
    CopyMem (((CHAR8 *) (&NewBuffer->IfrData) + Count), &StringSize, sizeof (RELOFST));
    Count = Count + sizeof (RELOFST);

    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
    Length = Length + sizeof (RELOFST);
    CopyMem (&StringPack->Header.Length, &Length, sizeof (UINT32));
  }
  //
  // Set Location to the First String
  //
  if (ResetStrings) {
    Index = OriginalStringCount;
  }
  //
  // Set Location to the First String
  //
  Location  = (UINT8 *) &StringPointer[Index];
  Index     = 0;

  //
  // Keep copying strings until you run into two CHAR16's in a row that are NULL
  //
  do {
    if ((*Reference == Increment) && !AddString) {
      StringLocation = ((UINT8 *) (&NewBuffer->IfrData) + Count);
      CopyMem (StringLocation, NewString, StrSize (NewString));

      //
      // Advance the destination location by Count number of bytes
      //
      Count = Count + StrSize (NewString);

      //
      // Add the difference between the new string and the old string to the length
      //
      CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));

      //
      // Since StringPack is a packed structure, we need to size it carefully (byte-wise) to avoid alignment issues
      //
      StringLocation = (UINT8 *) &Location[Index];
      for (Offset = 0;
           (StringLocation[Offset] != 0) || (StringLocation[Offset + 1] != 0);
           Offset = (RELOFST) (Offset + 2)
          )
        ;
      Offset  = (RELOFST) (Offset + 2);

      Length  = Length + (UINT32) StrSize (NewString) - Offset;

      CopyMem (&StringPack->Header.Length, &Length, sizeof (UINT32));
    } else {
      StringLocation = (UINT8 *) &Location[Index];
      for (Offset = 0;
           (StringLocation[Offset] != 0) || (StringLocation[Offset + 1] != 0);
           Offset = (RELOFST) (Offset + 2)
          )
        ;
      Offset = (RELOFST) (Offset + 2);

      CopyMem (((CHAR8 *) (&NewBuffer->IfrData) + Count), StringLocation, Offset);

      //
      // Advance the destination location by Count number of bytes
      //
      Count = Count + Offset;
    }
    //
    // Retrieve the number of characters to advance the index - should land at beginning of next string
    //
    Index = Index + Offset;
    Increment++;
    StringCount--;
    Offset = 0;
  } while (StringCount > 0);

  //
  // If we are adding a new string, then the above do/while will not suffice
  //
  if (AddString) {
    Offset = (RELOFST) StrSize (NewString);
    CopyMem (((CHAR8 *) (&NewBuffer->IfrData) + Count), NewString, Offset);

    Count = Count + StrSize (NewString);
    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
    Length = Length + (UINT32) StrSize (NewString);
    CopyMem (&StringPack->Header.Length, &Length, sizeof (UINT32));
  }

  if (ResetStrings) {
    //
    // Skip the remainder of strings in the string package
    //
    StringCount = OriginalStringCount - TotalStringCount;

    while (StringCount > 0) {
      StringLocation = (UINT8 *) &Location[Index];
      for (Offset = 0;
           (StringLocation[Offset] != 0) || (StringLocation[Offset + 1] != 0);
           Offset = (RELOFST) (Offset + 2)
          )
        ;
      Offset  = (RELOFST) (Offset + 2);
      Index   = Index + Offset;
      StringCount--;

      //
      // Adjust the size of the string pack by the string size we just skipped.
      // Also reduce the length by the size of a RelativeOffset value since we
      // obviously would have skipped that as well.
      //
      CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
      Length = Length - Offset - sizeof (RELOFST);
      CopyMem (&StringPack->Header.Length, &Length, sizeof (UINT32));
    }
  }

  StringPack = (EFI_HII_STRING_PACK *) &Location[Index];

  CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  for (; Length != 0;) {

    CopyMem (((CHAR8 *) (&NewBuffer->IfrData) + Count), StringPack, Length);

    Count       = Count + Length;
    StringPack  = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + Length);
    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  }
  //
  // Copy the null terminator to the new buffer
  //
  CopyMem (((CHAR8 *) (&NewBuffer->IfrData) + Count), StringPack, sizeof (EFI_HII_STRING_PACK));

  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (StringPackageInstance->IfrSize > 0) {
    Location      = (UINT8 *) (&StringPackageInstance->IfrData) + StringPackageInstance->IfrSize;
    StringPack    = (EFI_HII_STRING_PACK *) Location;
    Location      = (UINT8 *) (&NewBuffer->IfrData) + NewBuffer->IfrSize;
    NewStringPack = (EFI_HII_STRING_PACK *) Location;
  } else {
    StringPack    = (EFI_HII_STRING_PACK *) (&StringPackageInstance->IfrData);
    NewStringPack = (EFI_HII_STRING_PACK *) (&NewBuffer->IfrData);
  }

  CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  for (; Length != 0;) {
    //
    // Since we updated the old version of the string data as we moved things over
    // And we had a chicken-egg problem with the data we copied, let's post-fix the new
    // buffer with accurate length data.
    //
    CopyMem (&Count, &NewStringPack->Header.Length, sizeof (UINT32));
    CopyMem (&NewStringPack->Header.Length, &StringPack->Header.Length, sizeof (UINT32));
    CopyMem (&StringPack->Header.Length, &Count, sizeof (UINT32));

    CopyMem (&Count, &NewStringPack->Header.Length, sizeof (UINT32));
    NewStringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (NewStringPack) + Count);
    CopyMem (&Count, &StringPack->Header.Length, sizeof (UINT32));
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + Count);
    CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  }

  GetPackSize ((VOID *) ((CHAR8 *) (&NewBuffer->IfrData) + NewBuffer->IfrSize), &NewBuffer->StringSize, NULL);

  //
  // Search through the handles until the requested handle is found.
  //
  for (HandleDatabase = HiiData->DatabaseHead;
       HandleDatabase->Handle != 0;
       HandleDatabase = HandleDatabase->NextHandleDatabase
      ) {
    if (HandleDatabase->Handle == StringPackageInstance->Handle) {
      //
      // Free the previous buffer associated with this handle, and assign the new buffer to the handle
      //
      FreePool (HandleDatabase->Buffer);
      HandleDatabase->Buffer = NewBuffer;
      break;
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiNewString (
  IN     EFI_HII_PROTOCOL       *This,
  IN     CHAR16                 *Language,
  IN     EFI_HII_HANDLE         Handle,
  IN OUT STRING_REF             *Reference,
  IN     CHAR16                 *NewString
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
  UINTN       Index;
  CHAR16      *LangCodes;
  CHAR16      Lang[4];
  STRING_REF  OriginalValue;
  EFI_STATUS  Status;

  //
  // To avoid a warning 4 uninitialized variable warning
  //
  Status = EFI_SUCCESS;

  Status = HiiGetPrimaryLanguages (
            This,
            Handle,
            &LangCodes
            );

  if (!EFI_ERROR (Status)) {
    OriginalValue = *Reference;

    if (Language == NULL) {
      for (Index = 0; LangCodes[Index] != 0; Index += 3) {
        *Reference = OriginalValue;
        CopyMem (Lang, &LangCodes[Index], 6);
        Lang[3] = 0;
        Status = HiiNewString2 (
                  This,
                  Lang,
                  Handle,
                  Reference,
                  NewString,
                  FALSE
                  );

      }
    } else {
      Status = HiiNewString2 (
                This,
                Language,
                Handle,
                Reference,
                NewString,
                FALSE
                );
    }

    FreePool (LangCodes);
  }

  return Status;
}

EFI_STATUS
EFIAPI
HiiResetStrings (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle
  )
/*++

Routine Description:
  
    This function removes any new strings that were added after the initial string export for this handle.

Arguments:

Returns: 

--*/
{
  UINTN       Index;
  CHAR16      *LangCodes;
  CHAR16      Lang[4];
  STRING_REF  Reference;
  CHAR16      NewString;
  EFI_STATUS  Status;

  Reference = 1;
  NewString = 0;

  HiiGetPrimaryLanguages (
    This,
    Handle,
    &LangCodes
    );

  for (Index = 0; LangCodes[Index] != 0; Index += 3) {
    CopyMem (Lang, &LangCodes[Index], 6);
    Lang[3] = 0;
    Status = HiiNewString2 (
              This,
              Lang,
              Handle,
              &Reference,
              &NewString,
              TRUE
              );
    ASSERT_EFI_ERROR (Status);
  }

  FreePool (LangCodes);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HiiGetString (
  IN     EFI_HII_PROTOCOL    *This,
  IN     EFI_HII_HANDLE      Handle,
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
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_PACKAGE_INSTANCE  *StringPackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_STRING_PACK       *StringPack;
  RELOFST                   *StringPointer;
  EFI_STATUS                Status;
  UINTN                     DataSize;
  CHAR8                     Lang[3];
  CHAR16                    Language[3];
  UINT32                    Length;
  UINTN                     Count;
  RELOFST                   Offset;
  UINT16                    *Local;
  UINT16                    Zero;
  UINT16                    Narrow;
  UINT16                    Wide;
  UINT16                    NoBreak;
  BOOLEAN                   LangFound;
  UINT16                    *BufferLength = (UINT16 *) BufferLengthTemp;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  LangFound       = TRUE;

  DataSize        = sizeof (Lang);

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  PackageInstance = NULL;
  Zero            = 0;
  Narrow          = NARROW_CHAR;
  Wide            = WIDE_CHAR;
  NoBreak         = NON_BREAKING_CHAR;

  //
  // Check numeric value against the head of the database
  //
  for (HandleDatabase = HiiData->DatabaseHead;
       HandleDatabase != NULL;
       HandleDatabase = HandleDatabase->NextHandleDatabase
      ) {
    //
    // Match the numeric value with the database entry - if matched, extract PackageInstance
    //
    if (Handle == HandleDatabase->Handle) {
      PackageInstance = HandleDatabase->Buffer;
      break;
    }
  }
  //
  // No handle was found - error condition
  //
  if (PackageInstance == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidatePack (This, PackageInstance, &StringPackageInstance, NULL);

  //
  // If there is no specified language, assume the system default language
  //
  if (LanguageString == NULL) {
    //
    // Get system default language
    //
    Status = gRT->GetVariable (
                    (CHAR16 *) L"Lang",
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &DataSize,
                    Lang
                    );

    if (EFI_ERROR (Status)) {
      //
      // If Lang doesn't exist, just use the first language you find
      //
      LangFound = FALSE;
      goto LangNotFound;
    }
    //
    // Convert the ASCII Lang variable to a Unicode Language variable
    //
    AsciiToUnicode ((UINT8 *)Lang, Language);
  } else {
    //
    // Copy input ISO value to Language variable
    //
    CopyMem (Language, LanguageString, 6);
  }
  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
LangNotFound:
  if (StringPackageInstance->IfrSize > 0) {
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (&StringPackageInstance->IfrData) + StringPackageInstance->IfrSize);
  } else {
    StringPack = (EFI_HII_STRING_PACK *) (&StringPackageInstance->IfrData);
  }
  //
  // If Token is 0, extract entire string package
  //
  if (Token == 0) {
    //
    // Compute the entire string pack length, including all languages' and the terminating pack's.
    //
    Length = 0;
    while (0 != StringPack->Header.Length) {
      Length += StringPack->Header.Length;
      StringPack = (VOID*)(((UINT8*)StringPack) + StringPack->Header.Length);
    }
    //
    // Back to the start of package.
    //
    StringPack = (VOID*)(((UINT8*)StringPack) - Length); 
    //
    // Terminating zero sub-pack.
    //
    Length += sizeof (EFI_HII_STRING_PACK); 

    //
    // If trying to get the entire string package and have insufficient space.  Return error.
    //
    if (Length > *BufferLength || StringBuffer == NULL) {
      *BufferLength = (UINT16)Length;
      return EFI_BUFFER_TOO_SMALL;
    }
    //
    // Copy the Pack to the caller's buffer.
    //
    *BufferLength = (UINT16)Length;
    CopyMem (StringBuffer, StringPack, Length);

    return EFI_SUCCESS;
  }
  //
  // There may be multiple instances packed together of strings
  // so we must walk the self describing structures until we encounter
  // what we are looking for, and then extract the string we are looking for
  //
  CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
  for (; Length != 0;) {
    //
    // If passed in Language ISO value is in this string pack's language string
    // then we are dealing with the strings we want.
    //
    CopyMem (&Offset, &StringPack->LanguageNameString, sizeof (RELOFST));
    Status = HiiCompareLanguage ((CHAR16 *) ((CHAR8 *) (StringPack) + Offset), Language);

    //
    // If we cannot find the lang variable, we skip this check and use the first language available
    //
    if (LangFound) {
      if (EFI_ERROR (Status)) {
        StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + Length);
        CopyMem (&Length, &StringPack->Header.Length, sizeof (UINT32));
        continue;
      }
    }

    StringPointer = (RELOFST *) (StringPack + 1);

    //
    // We have the right string package - size it, and copy it to the StringBuffer
    //
    if (Token >= StringPack->NumStringPointers) {
      return EFI_INVALID_PARAMETER;
    } else {
      CopyMem (&Offset, &StringPointer[Token], sizeof (RELOFST));
    }
    //
    // Since StringPack is a packed structure, we need to determine the string's
    // size safely, thus byte-wise.  Post-increment the size to include the null-terminator
    //
    Local = (UINT16 *) ((CHAR8 *) (StringPack) + Offset);
    for (Count = 0; CompareMem (&Local[Count], &Zero, 2); Count++)
      ;
    Count++;

    Count = Count * sizeof (CHAR16);;

    if (*BufferLength >= Count && StringBuffer != NULL) {
      //
      // Copy the string to the user's buffer
      //
      if (Raw) {
        CopyMem (StringBuffer, Local, Count);
      } else {
        for (Count = 0; CompareMem (Local, &Zero, 2); Local++) {
          //
          // Skip "Narraw, Wide, NoBreak"
          //
          if (CompareMem (Local, &Narrow,  2) &&
              CompareMem (Local, &Wide,    2) && 
              CompareMem (Local, &NoBreak, 2)) {          
            CopyMem (&StringBuffer[Count++], Local, 2);          
          }        
        } 
        //
        // Add "NULL" at the end.
        //
        CopyMem (&StringBuffer[Count], &Zero, 2);
        Count++;
        Count *= sizeof (CHAR16);
      }

      *BufferLength = (UINT16) Count;
      return EFI_SUCCESS;
    } else {
      *BufferLength = (UINT16) Count;
      return EFI_BUFFER_TOO_SMALL;
    }

  }

  LangFound = FALSE;
  goto LangNotFound;
}

EFI_STATUS
EFIAPI
HiiGetLine (
  IN     EFI_HII_PROTOCOL   *This,
  IN     EFI_HII_HANDLE     Handle,
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
  UINTN                     Count;
  EFI_HII_PACKAGE_INSTANCE  *PackageInstance;
  EFI_HII_PACKAGE_INSTANCE  *StringPackageInstance;
  EFI_HII_DATA              *HiiData;
  EFI_HII_HANDLE_DATABASE   *HandleDatabase;
  EFI_HII_STRING_PACK       *StringPack;
  RELOFST                   *StringPointer;
  CHAR16                    *Location;
  EFI_STATUS                Status;
  UINTN                     DataSize;
  CHAR8                     Lang[3];
  CHAR16                    Language[3];

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  HandleDatabase  = HiiData->DatabaseHead;

  PackageInstance = NULL;
  DataSize        = 4;

  //
  // Check numeric value against the head of the database
  //
  for (; HandleDatabase != NULL; HandleDatabase = HandleDatabase->NextHandleDatabase) {
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

  Status = ValidatePack (This, PackageInstance, &StringPackageInstance, NULL);

  //
  // If there is no specified language, assume the system default language
  //
  if (LanguageString == NULL) {
    //
    // Get system default language
    //
    Status = gRT->GetVariable (
                    (CHAR16 *) L"Lang",
                    &gEfiGlobalVariableGuid,
                    NULL,
                    &DataSize,
                    Lang
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Convert the ASCII Lang variable to a Unicode Language variable
    //
    AsciiToUnicode ((UINT8 *)Lang, Language);
  } else {
    //
    // Copy input ISO value to Language variable
    //
    CopyMem (Language, LanguageString, 6);
  }
  //
  // Based on if there is IFR data in this package instance, determine
  // what the location is of the beginning of the string data.
  //
  if (StringPackageInstance->IfrSize > 0) {
    StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (&StringPackageInstance->IfrData) + StringPackageInstance->IfrSize);
  } else {
    StringPack = (EFI_HII_STRING_PACK *) (&StringPackageInstance->IfrData);
  }

  StringPointer = (RELOFST *) (StringPack + 1);

  //
  // There may be multiple instances packed together of strings
  // so we must walk the self describing structures until we encounter
  // what we are looking for, and then extract the string we are looking for
  //
  for (; StringPack->Header.Length != 0;) {
    //
    // If passed in Language ISO value is in this string pack's language string
    // then we are dealing with the strings we want.
    //
    Status = HiiCompareLanguage ((CHAR16 *) ((CHAR8 *) (StringPack) + StringPack->LanguageNameString), Language);

    if (EFI_ERROR (Status)) {
      StringPack = (EFI_HII_STRING_PACK *) ((CHAR8 *) (StringPack) + StringPack->Header.Length);
      continue;
    }

    Location = (CHAR16 *) ((CHAR8 *) (StringPack) + StringPointer[Token] +*Index * 2);

    //
    // If the size of the remaining string is less than the LineWidth
    // then copy the entire thing
    //
    if (StrSize (Location) <= LineWidth) {
      if (*BufferLength >= StrSize (Location)) {
        StrCpy (StringBuffer, Location);
        return EFI_SUCCESS;
      } else {
        *BufferLength = (UINT16) StrSize (Location);
        return EFI_BUFFER_TOO_SMALL;
      }
    } else {
      //
      // Rewind the string from the maximum size until we see a space the break the line
      //
      for (Count = LineWidth; Location[Count] != 0x0020; Count--)
        ;

      //
      // Put the index at the next character
      //
      *Index = (UINT16) (Count + 1);

      if (*BufferLength >= Count) {
        StrnCpy (StringBuffer, Location, Count);
        return EFI_SUCCESS;
      } else {
        *BufferLength = (UINT16) Count;
        return EFI_BUFFER_TOO_SMALL;
      }
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
HiiCompareLanguage (
  IN  CHAR16                *LanguageStringLocation,
  IN  CHAR16                *Language
  )
{
  UINT8   *Local;
  UINTN   Index;
  CHAR16  *InputString;
  CHAR16  *OriginalInputString;

  //
  // Allocate a temporary buffer for InputString
  //
  InputString = AllocateZeroPool (0x100);

  ASSERT (InputString);

  OriginalInputString = InputString;

  Local               = (UINT8 *) LanguageStringLocation;

  //
  // Determine the size of this packed string safely (e.g. access by byte), post-increment
  // to include the null-terminator
  //
  for (Index = 0; Local[Index] != 0; Index = Index + 2)
    ;
  //
  // MARMAR  Index = Index + 2;
  //
  // This is a packed structure that this location comes from, so let's make sure
  // the value is aligned by copying it to a local variable and working on it.
  //
  CopyMem (InputString, LanguageStringLocation, Index);

  for (Index = 0; Index < 3; Index++) {
    InputString[Index]  = (CHAR16) (InputString[Index] | 0x20);
    Language[Index]     = (CHAR16) (Language[Index] | 0x20);
  }
  //
  // If the Language is the same return success
  //
  if (CompareMem (LanguageStringLocation, Language, 6) == 0) {
    FreePool (InputString);
    return EFI_SUCCESS;
  }
  //
  // Skip the first three letters that comprised the primary language,
  // see if what is being compared against is a secondary language
  //
  InputString = InputString + 3;

  //
  // If the Language is not the same as the Primary language, see if there are any
  // secondary languages, and if there are see if we have a match.  If not, return an error.
  //
  for (Index = 0; InputString[Index] != 0; Index = Index + 3) {
    //
    // Getting in here means we have a secondary language
    //
    if (CompareMem (&InputString[Index], Language, 6) == 0) {
      FreePool (InputString);
      return EFI_SUCCESS;
    }
  }
  //
  // If nothing was found, return the error
  //
  FreePool (OriginalInputString);
  return EFI_NOT_FOUND;

}
