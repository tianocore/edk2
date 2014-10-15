/*++

Caution: This file is used for Duet platform only, do not use them in real platform.
All variable code, variable metadata, and variable data used by Duet platform are on 
disk. They can be changed by user. BIOS is not able to protoect those.
Duet trusts all meta data from disk. If variable code, variable metadata and variable
data is modified in inproper way, the behavior is undefined.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FSVariable.c

Abstract:

  Provide support functions for variable services.

--*/

#include "FSVariable.h"

VARIABLE_STORE_HEADER mStoreHeaderTemplate = {
  VARIABLE_STORE_SIGNATURE,
  VOLATILE_VARIABLE_STORE_SIZE,
  VARIABLE_STORE_FORMATTED,
  VARIABLE_STORE_HEALTHY,
  0,
  0
};

//
// Don't use module globals after the SetVirtualAddress map is signaled
//
VARIABLE_GLOBAL  *mGlobal;

/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable

  @param[in] VendorGuid         Guid of variable

  @param[in] Data               Variable data

  @param[in] DataSize           Size of data. 0 means delete

  @param[in] Attributes         Attribues of the variable

  @param[in] Variable           The variable information which is used to keep track of variable usage.

  @retval EFI_SUCCESS           The update operation is success.

  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
EFIAPI
UpdateVariable (
  IN      CHAR16                 *VariableName,
  IN      EFI_GUID               *VendorGuid,
  IN      VOID                   *Data,
  IN      UINTN                  DataSize,
  IN      UINT32                 Attributes OPTIONAL,
  IN      VARIABLE_POINTER_TRACK *Variable
  );

VOID
EFIAPI
OnVirtualAddressChangeFsv (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

VOID
EFIAPI
OnSimpleFileSystemInstall (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

BOOLEAN
IsValidVariableHeader (
  IN  VARIABLE_HEADER   *Variable
  )
/*++

Routine Description:

  This code checks if variable header is valid or not.

Arguments:
  Variable        Pointer to the Variable Header.

Returns:
  TRUE            Variable header is valid.
  FALSE           Variable header is not valid.

--*/
{
  if (Variable == NULL || Variable->StartId != VARIABLE_DATA) {
    return FALSE;
  }

  return TRUE;
}

VARIABLE_STORE_STATUS
GetVariableStoreStatus (
  IN VARIABLE_STORE_HEADER *VarStoreHeader
  )
/*++

Routine Description:

  This code gets the current status of Variable Store.

Arguments:

  VarStoreHeader  Pointer to the Variable Store Header.

Returns:

  EfiRaw        Variable store status is raw
  EfiValid      Variable store status is valid
  EfiInvalid    Variable store status is invalid

--*/
{
  if (CompareGuid (&VarStoreHeader->Signature, &mStoreHeaderTemplate.Signature) &&
      (VarStoreHeader->Format == mStoreHeaderTemplate.Format) &&
      (VarStoreHeader->State == mStoreHeaderTemplate.State)
     ) {
    return EfiValid;
  } else if (((UINT32 *)(&VarStoreHeader->Signature))[0] == VAR_DEFAULT_VALUE_32 &&
             ((UINT32 *)(&VarStoreHeader->Signature))[1] == VAR_DEFAULT_VALUE_32 &&
             ((UINT32 *)(&VarStoreHeader->Signature))[2] == VAR_DEFAULT_VALUE_32 &&
             ((UINT32 *)(&VarStoreHeader->Signature))[3] == VAR_DEFAULT_VALUE_32 &&
             VarStoreHeader->Size == VAR_DEFAULT_VALUE_32 &&
             VarStoreHeader->Format == VAR_DEFAULT_VALUE &&
             VarStoreHeader->State == VAR_DEFAULT_VALUE
          ) {

    return EfiRaw;
  } else {
    return EfiInvalid;
  }
}

UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
/*++

Routine Description:

  This code gets the pointer to the variable data.

Arguments:

  Variable            Pointer to the Variable Header.

Returns:

  UINT8*              Pointer to Variable Data

--*/
{
  //
  // Be careful about pad size for alignment
  //
  return (UINT8 *) ((UINTN) GET_VARIABLE_NAME_PTR (Variable) + Variable->NameSize + GET_PAD_SIZE (Variable->NameSize));
}

VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
/*++

Routine Description:

  This code gets the pointer to the next variable header.

Arguments:

  Variable              Pointer to the Variable Header.

Returns:

  VARIABLE_HEADER*      Pointer to next variable header.

--*/
{
  if (!IsValidVariableHeader (Variable)) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  return (VARIABLE_HEADER *) ((UINTN) GetVariableDataPtr (Variable) + Variable->DataSize + GET_PAD_SIZE (Variable->DataSize));
}

VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VarStoreHeader
  )
/*++

Routine Description:

  This code gets the pointer to the last variable memory pointer byte

Arguments:

  VarStoreHeader        Pointer to the Variable Store Header.

Returns:

  VARIABLE_HEADER*      Pointer to last unavailable Variable Header

--*/
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) ((UINTN) VarStoreHeader + VarStoreHeader->Size);
}

BOOLEAN
ExistNewerVariable (
  IN  VARIABLE_HEADER         *Variable
  )
/*++

Routine Description:

  Check if exist newer variable when doing reclaim

Arguments:

  Variable                    Pointer to start position

Returns:

  TRUE - Exists another variable, which is newer than the current one
  FALSE  - Doesn't exist another vairable which is newer than the current one

--*/
{
  VARIABLE_HEADER       *NextVariable;
  CHAR16                *VariableName;
  EFI_GUID              *VendorGuid;
  
  VendorGuid   = &Variable->VendorGuid;
  VariableName = GET_VARIABLE_NAME_PTR(Variable);
  
  NextVariable = GetNextVariablePtr (Variable);
  while (IsValidVariableHeader (NextVariable)) {
    if ((NextVariable->State == VAR_ADDED) || (NextVariable->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))) {
      //
      // If match Guid and Name
      //
      if (CompareGuid (VendorGuid, &NextVariable->VendorGuid)) {
         if (CompareMem (VariableName, GET_VARIABLE_NAME_PTR (NextVariable), StrSize (VariableName)) == 0) {
           return TRUE;
         }
       }
    }
    NextVariable = GetNextVariablePtr (NextVariable);
  }
  return FALSE;
}

EFI_STATUS
Reclaim (
  IN  VARIABLE_STORAGE_TYPE StorageType,
  IN  VARIABLE_HEADER       *CurrentVariable OPTIONAL
  )
/*++

Routine Description:

  Variable store garbage collection and reclaim operation

Arguments:

  IsVolatile                  The variable store is volatile or not,
                              if it is non-volatile, need FTW
  CurrentVairable             If it is not NULL, it means not to process
                              current variable for Reclaim.

Returns:

  EFI STATUS

--*/
{
  VARIABLE_HEADER       *Variable;
  VARIABLE_HEADER       *NextVariable;
  VARIABLE_STORE_HEADER *VariableStoreHeader;
  UINT8                 *ValidBuffer;
  UINTN                 ValidBufferSize;
  UINTN                 VariableSize;
  UINT8                 *CurrPtr;
  EFI_STATUS            Status;

  VariableStoreHeader = (VARIABLE_STORE_HEADER *) mGlobal->VariableBase[StorageType];

  //
  // Start Pointers for the variable.
  //
  Variable        = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

  //
  // recaluate the total size of Common/HwErr type variables in non-volatile area.
  //
  if (!StorageType) {
    mGlobal->CommonVariableTotalSize = 0;
    mGlobal->HwErrVariableTotalSize  = 0;
  }
  //
  // To make the reclaim, here we just allocate a memory that equal to the original memory
  //
  ValidBufferSize = sizeof (VARIABLE_STORE_HEADER) + VariableStoreHeader->Size;

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  ValidBufferSize,
                  (VOID**) &ValidBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CurrPtr = ValidBuffer;

  //
  // Copy variable store header
  //
  CopyMem (CurrPtr, VariableStoreHeader, sizeof (VARIABLE_STORE_HEADER));
  CurrPtr += sizeof (VARIABLE_STORE_HEADER);

  //
  // Start Pointers for the variable.
  //
  Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

  
  ValidBufferSize = sizeof (VARIABLE_STORE_HEADER);
  while (IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    //
    // State VAR_ADDED or VAR_IN_DELETED_TRANSITION are to kept,
    // The CurrentVariable, is also saved, as SetVariable may fail due to lack of space
    //
    if (Variable->State == VAR_ADDED) {
      VariableSize = (UINTN) NextVariable - (UINTN) Variable;
      CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
      ValidBufferSize += VariableSize;
      CurrPtr += VariableSize;
      if ((!StorageType) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        mGlobal->HwErrVariableTotalSize += VariableSize;
      } else if ((!StorageType) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
        mGlobal->CommonVariableTotalSize += VariableSize;
      }
    } else if (Variable->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)) {
      //
      // As variables that with the same guid and name may exist in NV due to power failure during SetVariable,
      // we will only save the latest valid one
      //
      if (!ExistNewerVariable(Variable)) {
        VariableSize = (UINTN) NextVariable - (UINTN) Variable;
        CopyMem (CurrPtr, (UINT8 *) Variable, VariableSize);
        //
        // If CurrentVariable == Variable, mark as VAR_IN_DELETED_TRANSITION
        //
        if (Variable != CurrentVariable){
          ((VARIABLE_HEADER *)CurrPtr)->State = VAR_ADDED;
        }
        CurrPtr += VariableSize;
        ValidBufferSize += VariableSize;
        if ((!StorageType) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          mGlobal->HwErrVariableTotalSize += VariableSize;
        } else if ((!StorageType) && ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
          mGlobal->CommonVariableTotalSize += VariableSize;
        }
      }
    }
    Variable = NextVariable;
  }

  mGlobal->LastVariableOffset[StorageType] = ValidBufferSize;

  //
  // TODO: cannot restore to original state, basic FTW needed
  //
  Status = mGlobal->VariableStore[StorageType]->Erase (
                                                  mGlobal->VariableStore[StorageType]
                                                  );
  Status = mGlobal->VariableStore[StorageType]->Write (
                                                    mGlobal->VariableStore[StorageType],
                                                    0,
                                                    ValidBufferSize,
                                                    ValidBuffer
                                                    );

  if (EFI_ERROR (Status)) {
    //
    // If error, then reset the last variable offset to zero.
    //
    mGlobal->LastVariableOffset[StorageType] = 0;
  };

  gBS->FreePool (ValidBuffer);

  return Status;
}

EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack
  )
/*++

Routine Description:

  This code finds variable in storage blocks (Volatile or Non-Volatile)

Arguments:

  VariableName                Name of the variable to be found
  VendorGuid                  Vendor GUID to be found.
  PtrTrack                    Variable Track Pointer structure that contains
                              Variable Information.
                              Contains the pointer of Variable header.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter
  EFI_SUCCESS                 - Find the specified variable
  EFI_NOT_FOUND               - Not found

--*/
{
  VARIABLE_HEADER         *Variable;
  VARIABLE_STORE_HEADER   *VariableStoreHeader;
  UINTN                   Index;
  VARIABLE_HEADER         *InDeleteVariable;
  UINTN                   InDeleteIndex;
  VARIABLE_HEADER         *InDeleteStartPtr;
  VARIABLE_HEADER         *InDeleteEndPtr;

  if (VariableName[0] != 0 && VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InDeleteVariable = NULL;
  InDeleteIndex    = (UINTN)-1;
  InDeleteStartPtr = NULL;
  InDeleteEndPtr   = NULL;

  for (Index = 0; Index < MaxType; Index ++) {
    //
    // 0: Non-Volatile, 1: Volatile
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) mGlobal->VariableBase[Index];

    //
    // Start Pointers for the variable.
    // Actual Data Pointer where data can be written.
    //
    Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

    //
    // Find the variable by walk through non-volatile and volatile variable store
    //
    PtrTrack->StartPtr = Variable;
    PtrTrack->EndPtr   = GetEndPointer (VariableStoreHeader);

    while ((Variable < PtrTrack->EndPtr) && IsValidVariableHeader (Variable)) {
      if (Variable->State == VAR_ADDED) {
        if (!EfiAtRuntime () || (Variable->Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
          if (VariableName[0] == 0) {
            PtrTrack->CurrPtr = Variable;
            PtrTrack->Type    = (VARIABLE_STORAGE_TYPE) Index;
            return EFI_SUCCESS;
          } else {
            if (CompareGuid (VendorGuid, &Variable->VendorGuid)) {
              if (!CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable), StrSize (VariableName))) {
                PtrTrack->CurrPtr = Variable;
                PtrTrack->Type    = (VARIABLE_STORAGE_TYPE) Index;
                return EFI_SUCCESS;
              }
            }
          }
        }
      } else if (Variable->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)) {
        //
        // VAR_IN_DELETED_TRANSITION should also be checked.
        //
        if (!EfiAtRuntime () || (Variable->Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
          if (VariableName[0] == 0) {
            InDeleteVariable = Variable;
            InDeleteIndex    = Index;
            InDeleteStartPtr = PtrTrack->StartPtr;
            InDeleteEndPtr   = PtrTrack->EndPtr;
          } else {
            if (CompareGuid (VendorGuid, &Variable->VendorGuid)) {
              if (!CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable), StrSize (VariableName))) {
                InDeleteVariable = Variable;
                InDeleteIndex    = Index;
                InDeleteStartPtr = PtrTrack->StartPtr;
                InDeleteEndPtr   = PtrTrack->EndPtr;
              }
            }
          }
        }
      }

      Variable = GetNextVariablePtr (Variable);
    }
    //
    // While (...)
    //
  }
  //
  // for (...)
  //

  //
  // if VAR_IN_DELETED_TRANSITION found, and VAR_ADDED not found,
  // we return it.
  //
  if (InDeleteVariable != NULL) {
    PtrTrack->CurrPtr  = InDeleteVariable;
    PtrTrack->Type     = (VARIABLE_STORAGE_TYPE) InDeleteIndex;
    PtrTrack->StartPtr = InDeleteStartPtr;
    PtrTrack->EndPtr   = InDeleteEndPtr;
    return EFI_SUCCESS;
  }

  PtrTrack->CurrPtr = NULL;
  return EFI_NOT_FOUND;
}

/**
  Get index from supported language codes according to language string.

  This code is used to get corresponding index in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation to find matched string and calculate the index.
  In RFC4646 language tags, take semicolon as a delimitation to find matched string and calculate the index.

  For example:
    SupportedLang  = "engfraengfra"
    Lang           = "eng"
    Iso639Language = TRUE
  The return value is "0".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Lang           = "fr-FR"
    Iso639Language = FALSE
  The return value is "3".

  @param  SupportedLang               Platform supported language codes.
  @param  Lang                        Configured language.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval the index of language in the language codes.

**/
UINTN
GetIndexFromSupportedLangCodes(
  IN  CHAR8            *SupportedLang,
  IN  CHAR8            *Lang,
  IN  BOOLEAN          Iso639Language
  ) 
{
  UINTN    Index;
  UINTN    CompareLength;
  UINTN    LanguageLength;

  if (Iso639Language) {
    CompareLength = ISO_639_2_ENTRY_SIZE;
    for (Index = 0; Index < AsciiStrLen (SupportedLang); Index += CompareLength) {
      if (AsciiStrnCmp (Lang, SupportedLang + Index, CompareLength) == 0) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        Index = Index / CompareLength;
        return Index;
      }
    }
    ASSERT (FALSE);
    return 0;
  } else {
    //
    // Compare RFC4646 language code
    //
    Index = 0;
    for (LanguageLength = 0; Lang[LanguageLength] != '\0'; LanguageLength++);

    for (Index = 0; *SupportedLang != '\0'; Index++, SupportedLang += CompareLength) {
      //
      // Skip ';' characters in SupportedLang
      //
      for (; *SupportedLang != '\0' && *SupportedLang == ';'; SupportedLang++);
      //
      // Determine the length of the next language code in SupportedLang
      //
      for (CompareLength = 0; SupportedLang[CompareLength] != '\0' && SupportedLang[CompareLength] != ';'; CompareLength++);
      
      if ((CompareLength == LanguageLength) && 
          (AsciiStrnCmp (Lang, SupportedLang, CompareLength) == 0)) {
        //
        // Successfully find the index of Lang string in SupportedLang string.
        //
        return Index;
      }
    }
    ASSERT (FALSE);
    return 0;
  }
}

/**
  Get language string from supported language codes according to index.

  This code is used to get corresponding language string in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation. Find language string according to the index.
  In RFC4646 language tags, take semicolon as a delimitation. Find language string according to the index.

  For example:
    SupportedLang  = "engfraengfra"
    Index          = "1"
    Iso639Language = TRUE
  The return value is "fra".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Index          = "1"
    Iso639Language = FALSE
  The return value is "fr".

  @param  SupportedLang               Platform supported language codes.
  @param  Index                       the index in supported language codes.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval the language string in the language codes.

**/
CHAR8 *
GetLangFromSupportedLangCodes (
  IN  CHAR8            *SupportedLang,
  IN  UINTN            Index,
  IN  BOOLEAN          Iso639Language
)
{
  UINTN    SubIndex;
  UINTN    CompareLength;
  CHAR8    *Supported;

  SubIndex  = 0;
  Supported = SupportedLang;
  if (Iso639Language) {
    //
    // according to the index of Lang string in SupportedLang string to get the language.
    // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
    // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
    //
    CompareLength = ISO_639_2_ENTRY_SIZE;
    mGlobal->Lang[CompareLength] = '\0';
    return CopyMem (mGlobal->Lang, SupportedLang + Index * CompareLength, CompareLength);

  } else {
    while (TRUE) {
      //
      // take semicolon as delimitation, sequentially traverse supported language codes.
      //
      for (CompareLength = 0; *Supported != ';' && *Supported != '\0'; CompareLength++) {
        Supported++;
      }
      if ((*Supported == '\0') && (SubIndex != Index)) {
        //
        // Have completed the traverse, but not find corrsponding string.
        // This case is not allowed to happen.
        //
        ASSERT(FALSE);
        return NULL;
      }
      if (SubIndex == Index) {
        //
        // according to the index of Lang string in SupportedLang string to get the language.
        // As this code will be invoked in RUNTIME, therefore there is not memory allocate/free operation.
        // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
        //
        mGlobal->PlatformLang[CompareLength] = '\0';
        return CopyMem (mGlobal->PlatformLang, Supported - CompareLength, CompareLength);
      }
      SubIndex++;

      //
      // Skip ';' characters in Supported
      //
      for (; *Supported != '\0' && *Supported == ';'; Supported++);
    }
  }
}

/**
  Returns a pointer to an allocated buffer that contains the best matching language 
  from a set of supported languages.  
  
  This function supports both ISO 639-2 and RFC 4646 language codes, but language 
  code types may not be mixed in a single call to this function. This function
  supports a variable argument list that allows the caller to pass in a prioritized
  list of language codes to test against all the language codes in SupportedLanguages.

  If SupportedLanguages is NULL, then ASSERT().

  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string that
                                  contains a set of language codes in the format 
                                  specified by Iso639Language.
  @param[in]  Iso639Language      If TRUE, then all language codes are assumed to be
                                  in ISO 639-2 format.  If FALSE, then all language
                                  codes are assumed to be in RFC 4646 language format
  @param[in]  ...                 A variable argument list that contains pointers to 
                                  Null-terminated ASCII strings that contain one or more
                                  language codes in the format specified by Iso639Language.
                                  The first language code from each of these language
                                  code lists is used to determine if it is an exact or
                                  close match to any of the language codes in 
                                  SupportedLanguages.  Close matches only apply to RFC 4646
                                  language codes, and the matching algorithm from RFC 4647
                                  is used to determine if a close match is present.  If 
                                  an exact or close match is found, then the matching
                                  language code from SupportedLanguages is returned.  If
                                  no matches are found, then the next variable argument
                                  parameter is evaluated.  The variable argument list 
                                  is terminated by a NULL.

  @retval NULL   The best matching language could not be found in SupportedLanguages.
  @retval NULL   There are not enough resources available to return the best matching 
                 language.
  @retval Other  A pointer to a Null-terminated ASCII string that is the best matching 
                 language in SupportedLanguages.

**/
CHAR8 *
EFIAPI
VariableGetBestLanguage (
  IN CONST CHAR8  *SupportedLanguages, 
  IN BOOLEAN      Iso639Language,
  ...
  )
{
  VA_LIST      Args;
  CHAR8        *Language;
  UINTN        CompareLength;
  UINTN        LanguageLength;
  CONST CHAR8  *Supported;
  CHAR8        *Buffer;

  ASSERT (SupportedLanguages != NULL);

  VA_START (Args, Iso639Language);
  while ((Language = VA_ARG (Args, CHAR8 *)) != NULL) {
    //
    // Default to ISO 639-2 mode
    //
    CompareLength  = 3;
    LanguageLength = MIN (3, AsciiStrLen (Language));

    //
    // If in RFC 4646 mode, then determine the length of the first RFC 4646 language code in Language
    //
    if (!Iso639Language) {
      for (LanguageLength = 0; Language[LanguageLength] != 0 && Language[LanguageLength] != ';'; LanguageLength++);
    }

    //
    // Trim back the length of Language used until it is empty
    //
    while (LanguageLength > 0) {
      //
      // Loop through all language codes in SupportedLanguages
      //
      for (Supported = SupportedLanguages; *Supported != '\0'; Supported += CompareLength) {
        //
        // In RFC 4646 mode, then Loop through all language codes in SupportedLanguages
        //
        if (!Iso639Language) {
          //
          // Skip ';' characters in Supported
          //
          for (; *Supported != '\0' && *Supported == ';'; Supported++);
          //
          // Determine the length of the next language code in Supported
          //
          for (CompareLength = 0; Supported[CompareLength] != 0 && Supported[CompareLength] != ';'; CompareLength++);
          //
          // If Language is longer than the Supported, then skip to the next language
          //
          if (LanguageLength > CompareLength) {
            continue;
          }
        }
        //
        // See if the first LanguageLength characters in Supported match Language
        //
        if (AsciiStrnCmp (Supported, Language, LanguageLength) == 0) {
          VA_END (Args);

          Buffer = Iso639Language ? mGlobal->Lang : mGlobal->PlatformLang;
          Buffer[CompareLength] = '\0';
          return CopyMem (Buffer, Supported, CompareLength);
        }
      }

      if (Iso639Language) {
        //
        // If ISO 639 mode, then each language can only be tested once
        //
        LanguageLength = 0;
      } else {
        //
        // If RFC 4646 mode, then trim Language from the right to the next '-' character 
        //
        for (LanguageLength--; LanguageLength > 0 && Language[LanguageLength] != '-'; LanguageLength--);
      }
    }
  }
  VA_END (Args);

  //
  // No matches were found 
  //
  return NULL;
}

/**
  Hook the operations in PlatformLangCodes, LangCodes, PlatformLang and Lang.

  When setting Lang/LangCodes, simultaneously update PlatformLang/PlatformLangCodes.

  According to UEFI spec, PlatformLangCodes/LangCodes are only set once in firmware initialization,
  and are read-only. Therefore, in variable driver, only store the original value for other use.

  @param[in] VariableName       Name of variable

  @param[in] Data               Variable data

  @param[in] DataSize           Size of data. 0 means delete

**/
VOID
AutoUpdateLangVariable(
  IN  CHAR16             *VariableName,
  IN  VOID               *Data,
  IN  UINTN              DataSize
  )
{
  EFI_STATUS             Status;
  CHAR8                  *BestPlatformLang;
  CHAR8                  *BestLang;
  UINTN                  Index;
  UINT32                 Attributes;
  VARIABLE_POINTER_TRACK Variable;
  BOOLEAN                SetLanguageCodes;

  //
  // Don't do updates for delete operation
  //
  if (DataSize == 0) {
    return;
  }

  SetLanguageCodes = FALSE;

  if (StrCmp (VariableName, L"PlatformLangCodes") == 0) {
    //
    // PlatformLangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (EfiAtRuntime ()) {
      return;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, PlatformLangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (mGlobal->PlatformLangCodes != NULL) {
      FreePool (mGlobal->PlatformLangCodes);
    }
    mGlobal->PlatformLangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mGlobal->PlatformLangCodes != NULL);

    //
    // PlatformLang holds a single language from PlatformLangCodes, 
    // so the size of PlatformLangCodes is enough for the PlatformLang.
    //
    if (mGlobal->PlatformLang != NULL) {
      FreePool (mGlobal->PlatformLang);
    }
    mGlobal->PlatformLang = AllocateRuntimePool (DataSize);
    ASSERT (mGlobal->PlatformLang != NULL);

  } else if (StrCmp (VariableName, L"LangCodes") == 0) {
    //
    // LangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (EfiAtRuntime ()) {
      return;
    }

    SetLanguageCodes = TRUE;

    //
    // According to UEFI spec, LangCodes is only set once in firmware initialization, and is read-only
    // Therefore, in variable driver, only store the original value for other use.
    //
    if (mGlobal->LangCodes != NULL) {
      FreePool (mGlobal->LangCodes);
    }
    mGlobal->LangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mGlobal->LangCodes != NULL);
  }

  if (SetLanguageCodes 
      && (mGlobal->PlatformLangCodes != NULL)
      && (mGlobal->LangCodes != NULL)) {
    //
    // Update Lang if PlatformLang is already set
    // Update PlatformLang if Lang is already set
    //
    Status = FindVariable (L"PlatformLang", &gEfiGlobalVariableGuid, &Variable);
    if (!EFI_ERROR (Status)) {
      //
      // Update Lang
      //
      VariableName = L"PlatformLang";
      Data         = GetVariableDataPtr (Variable.CurrPtr);
      DataSize     = Variable.CurrPtr->DataSize;
    } else {
      Status = FindVariable (L"Lang", &gEfiGlobalVariableGuid, &Variable);
      if (!EFI_ERROR (Status)) {
        //
        // Update PlatformLang
        //
        VariableName = L"Lang";
        Data         = GetVariableDataPtr (Variable.CurrPtr);
        DataSize     = Variable.CurrPtr->DataSize;
      } else {
        //
        // Neither PlatformLang nor Lang is set, directly return
        //
        return;
      }
    }
  }
  
  //
  // According to UEFI spec, "Lang" and "PlatformLang" is NV|BS|RT attributions.
  //
  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  if (StrCmp (VariableName, L"PlatformLang") == 0) {
    //
    // Update Lang when PlatformLangCodes/LangCodes were set.
    //
    if ((mGlobal->PlatformLangCodes != NULL) && (mGlobal->LangCodes != NULL)) {
      //
      // When setting PlatformLang, firstly get most matched language string from supported language codes.
      //
      BestPlatformLang = VariableGetBestLanguage (mGlobal->PlatformLangCodes, FALSE, Data, NULL);
      if (BestPlatformLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mGlobal->PlatformLangCodes, BestPlatformLang, FALSE);

        //
        // Get the corresponding ISO639 language tag according to RFC4646 language tag.
        //
        BestLang = GetLangFromSupportedLangCodes (mGlobal->LangCodes, Index, TRUE);

        //
        // Successfully convert PlatformLang to Lang, and set the BestLang value into Lang variable simultaneously.
        //
        FindVariable(L"Lang", &gEfiGlobalVariableGuid, &Variable);

        Status = UpdateVariable (L"Lang", &gEfiGlobalVariableGuid, BestLang, ISO_639_2_ENTRY_SIZE + 1, Attributes, &Variable);

        DEBUG ((EFI_D_INFO, "Variable Driver Auto Update PlatformLang, PlatformLang:%a, Lang:%a\n", BestPlatformLang, BestLang));

        ASSERT_EFI_ERROR(Status);
      }
    }

  } else if (StrCmp (VariableName, L"Lang") == 0) {
    //
    // Update PlatformLang when PlatformLangCodes/LangCodes were set.
    //
    if ((mGlobal->PlatformLangCodes != NULL) && (mGlobal->LangCodes != NULL)) {
      //
      // When setting Lang, firstly get most matched language string from supported language codes.
      //
      BestLang = VariableGetBestLanguage (mGlobal->LangCodes, TRUE, Data, NULL);
      if (BestLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mGlobal->LangCodes, BestLang, TRUE);

        //
        // Get the corresponding RFC4646 language tag according to ISO639 language tag.
        //
        BestPlatformLang = GetLangFromSupportedLangCodes (mGlobal->PlatformLangCodes, Index, FALSE);

        //
        // Successfully convert Lang to PlatformLang, and set the BestPlatformLang value into PlatformLang variable simultaneously.
        //
        FindVariable(L"PlatformLang", &gEfiGlobalVariableGuid, &Variable);

        Status = UpdateVariable (L"PlatformLang", &gEfiGlobalVariableGuid, BestPlatformLang, 
                                 AsciiStrSize (BestPlatformLang), Attributes, &Variable);

        DEBUG ((EFI_D_INFO, "Variable Driver Auto Update Lang, Lang:%a, PlatformLang:%a\n", BestLang, BestPlatformLang));
        ASSERT_EFI_ERROR (Status);
      }
    }
  }
}

/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable

  @param[in] VendorGuid         Guid of variable

  @param[in] Data               Variable data

  @param[in] DataSize           Size of data. 0 means delete

  @param[in] Attributes         Attribues of the variable

  @param[in] Variable           The variable information which is used to keep track of variable usage.

  @retval EFI_SUCCESS           The update operation is success.

  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
EFIAPI
UpdateVariable (
  IN      CHAR16                 *VariableName,
  IN      EFI_GUID               *VendorGuid,
  IN      VOID                   *Data,
  IN      UINTN                  DataSize,
  IN      UINT32                 Attributes OPTIONAL,
  IN      VARIABLE_POINTER_TRACK *Variable
  )
{
  EFI_STATUS                          Status;
  VARIABLE_HEADER                     *NextVariable;
  UINTN                               VarNameOffset;
  UINTN                               VarDataOffset;
  UINTN                               VarNameSize;
  UINTN                               VarSize;
  UINT8                               State;
  BOOLEAN                             Reclaimed;
  VARIABLE_STORAGE_TYPE               StorageType;

  Reclaimed         = FALSE;

  if (Variable->CurrPtr != NULL) {  
    //
    // Update/Delete existing variable
    //
    
    if (EfiAtRuntime ()) {              
      //
      // If EfiAtRuntime and the variable is Volatile and Runtime Access,  
      // the volatile is ReadOnly, and SetVariable should be aborted and 
      // return EFI_WRITE_PROTECTED.
      //
      if (Variable->Type == Volatile) {
        return EFI_WRITE_PROTECTED;
      }
      //
      // Only variable have NV attribute can be updated/deleted in Runtime
      //
      if (!(Variable->CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE)) {
        return EFI_INVALID_PARAMETER;      
      }
    }
    
    //
    // Setting a data variable with no access, or zero DataSize attributes
    // specified causes it to be deleted.
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      //
      // Found this variable in storage
      //
      State = Variable->CurrPtr->State;
      State &= VAR_DELETED;

      Status = mGlobal->VariableStore[Variable->Type]->Write (
                                                        mGlobal->VariableStore[Variable->Type],
                                                        VARIABLE_MEMBER_OFFSET (State, (UINTN) Variable->CurrPtr - (UINTN) Variable->StartPtr),
                                                        sizeof (Variable->CurrPtr->State),
                                                        &State
                                                        );
      //
      // NOTE: Write operation at least can write data to memory cache
      //       Discard file writing failure here.
      //
      return EFI_SUCCESS;
    }
    
    //
    // Found this variable in storage
    // If the variable is marked valid and the same data has been passed in
    // then return to the caller immediately.
    //
    if ((Variable->CurrPtr->DataSize == DataSize) &&
        (CompareMem (Data, GetVariableDataPtr (Variable->CurrPtr), DataSize) == 0)
          ) {
      return EFI_SUCCESS;
    } else if ((Variable->CurrPtr->State == VAR_ADDED) ||
               (Variable->CurrPtr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))) {
      //
      // Mark the old variable as in delete transition
      //
      State = Variable->CurrPtr->State;
      State &= VAR_IN_DELETED_TRANSITION;

      Status = mGlobal->VariableStore[Variable->Type]->Write (
                                                        mGlobal->VariableStore[Variable->Type],
                                                        VARIABLE_MEMBER_OFFSET (State, (UINTN) Variable->CurrPtr - (UINTN) Variable->StartPtr),
                                                        sizeof (Variable->CurrPtr->State),
                                                        &State
                                                        );
      //
      // NOTE: Write operation at least can write data to memory cache
      //       Discard file writing failure here.
      //
    }
  } else {
    //
    // Create a new variable
    //  
    
    //
    // Make sure we are trying to create a new variable.
    // Setting a data variable with no access, or zero DataSize attributes means to delete it.    
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      return EFI_NOT_FOUND;
    }    
    //
    // Only variable have NV|RT attribute can be created in Runtime
    //
    if (EfiAtRuntime () &&
        (!(Attributes & EFI_VARIABLE_RUNTIME_ACCESS) || !(Attributes & EFI_VARIABLE_NON_VOLATILE))) {
      return EFI_INVALID_PARAMETER;
    }        
    
  } 

  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.  
  // We can firstly write all the data in memory, then write them to file
  // This can reduce the times of write operation
  //
  
  NextVariable = (VARIABLE_HEADER *) mGlobal->Scratch;

  NextVariable->StartId     = VARIABLE_DATA;
  NextVariable->Attributes  = Attributes;
  NextVariable->State       = VAR_ADDED;
  NextVariable->Reserved    = 0;
  VarNameOffset             = sizeof (VARIABLE_HEADER);
  VarNameSize               = StrSize (VariableName);
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarDataOffset),
    Data,
    DataSize
    );
  CopyMem (&NextVariable->VendorGuid, VendorGuid, sizeof (EFI_GUID));
  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->DataSize should not include pad size so that variable
  // service can get actual size in GetVariable
  //
  NextVariable->NameSize  = (UINT32)VarNameSize;
  NextVariable->DataSize  = (UINT32)DataSize;

  //
  // The actual size of the variable that stores in storage should
  // include pad size.
  // VarDataOffset: offset from begin of current variable header
  //
  VarSize = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);

  StorageType = (Attributes & EFI_VARIABLE_NON_VOLATILE) ? NonVolatile : Volatile;

  if ((UINT32) (VarSize + mGlobal->LastVariableOffset[StorageType]) >
      ((VARIABLE_STORE_HEADER *) mGlobal->VariableBase[StorageType])->Size
      ) {
    if ((StorageType == NonVolatile) && EfiAtRuntime ()) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Perform garbage collection & reclaim operation
    //
    Status = Reclaim (StorageType, Variable->CurrPtr);
    if (EFI_ERROR (Status)) {
      //
      // Reclaim error
      // we cannot restore to original state, fetal error, report to user
      //
      DEBUG ((EFI_D_ERROR, "FSVariable: Recalim error (fetal error) - %r\n", Status));
      return Status;
    }
    //
    // If still no enough space, return out of resources
    //
    if ((UINT32) (VarSize + mGlobal->LastVariableOffset[StorageType]) >
        ((VARIABLE_STORE_HEADER *) mGlobal->VariableBase[StorageType])->Size
       ) {
      return EFI_OUT_OF_RESOURCES;
    }

    Reclaimed = TRUE;
  }
  Status = mGlobal->VariableStore[StorageType]->Write (
                                                  mGlobal->VariableStore[StorageType],
                                                  mGlobal->LastVariableOffset[StorageType],
                                                  VarSize,
                                                  NextVariable
                                                  );
  //
  // NOTE: Write operation at least can write data to memory cache
  //       Discard file writing failure here.
  //
  mGlobal->LastVariableOffset[StorageType] += VarSize;

  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
    mGlobal->HwErrVariableTotalSize += VarSize;
  } else {
    mGlobal->CommonVariableTotalSize += VarSize;
  }

  //
  // Mark the old variable as deleted
  //
  if (!Reclaimed && !EFI_ERROR (Status) && Variable->CurrPtr != NULL) {
    State = Variable->CurrPtr->State;
    State &= VAR_DELETED;

    Status = mGlobal->VariableStore[StorageType]->Write (
                                                    mGlobal->VariableStore[StorageType],
                                                    VARIABLE_MEMBER_OFFSET (State, (UINTN) Variable->CurrPtr - (UINTN) Variable->StartPtr),
                                                    sizeof (Variable->CurrPtr->State),
                                                    &State
                                                    );
    //
    // NOTE: Write operation at least can write data to memory cache
    //       Discard file writing failure here.
    //
  }
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DuetGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data
  )
/*++

Routine Description:

  This code finds variable in storage blocks (Volatile or Non-Volatile)

Arguments:

  VariableName                    Name of Variable to be found
  VendorGuid                      Variable vendor GUID
  Attributes OPTIONAL             Attribute value of the variable found
  DataSize                        Size of Data found. If size is less than the
                                  data, this value contains the required size.
  Data                            Data pointer

Returns:

  EFI STATUS

--*/
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;
  EFI_STATUS              Status;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find existing variable
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Get data size
  //
  VarDataSize = Variable.CurrPtr->DataSize;
  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    CopyMem (Data, GetVariableDataPtr (Variable.CurrPtr), VarDataSize);

    if (Attributes != NULL) {
      *Attributes = Variable.CurrPtr->Attributes;
    }

    *DataSize = VarDataSize;

    return EFI_SUCCESS;
  } else {
    *DataSize = VarDataSize;
    return EFI_BUFFER_TOO_SMALL;
  }
}

EFI_STATUS
EFIAPI
GetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid
  )
/*++

Routine Description:

  This code Finds the Next available variable

Arguments:

  VariableNameSize            Size of the variable
  VariableName                Pointer to variable name
  VendorGuid                  Variable Vendor Guid

Returns:

  EFI STATUS

--*/
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = FindVariable (VariableName, VendorGuid, &Variable);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    return Status;
  }

  if (VariableName[0] != 0) {
    //
    // If variable name is not NULL, get next variable
    //
    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }

  while (TRUE) {
    //
    // The order we find variable is: 1). NonVolatile; 2). Volatile
    // If both volatile and non-volatile variable store are parsed,
    // return not found
    //
    if (Variable.CurrPtr >= Variable.EndPtr || Variable.CurrPtr == NULL) {
      if (Variable.Type == Volatile) {
        //
        // Since we met the end of Volatile storage, we have parsed all the stores.
        //
        return EFI_NOT_FOUND;
      }

      //
      // End of NonVolatile, continue to parse Volatile
      //
      Variable.Type = Volatile;
      Variable.StartPtr = (VARIABLE_HEADER *) ((VARIABLE_STORE_HEADER *) mGlobal->VariableBase[Volatile] + 1);
      Variable.EndPtr   = (VARIABLE_HEADER *) GetEndPointer ((VARIABLE_STORE_HEADER *) mGlobal->VariableBase[Volatile]);

      Variable.CurrPtr = Variable.StartPtr;
      if (!IsValidVariableHeader (Variable.CurrPtr)) {
        continue;
      }
    }
    //
    // Variable is found
    //
    if (IsValidVariableHeader (Variable.CurrPtr) &&
        ((Variable.CurrPtr->State == VAR_ADDED) ||
         (Variable.CurrPtr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION)))) {
      if (!EfiAtRuntime () || (Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
        VarNameSize = Variable.CurrPtr->NameSize;
        if (VarNameSize <= *VariableNameSize) {
          CopyMem (
            VariableName,
            GET_VARIABLE_NAME_PTR (Variable.CurrPtr),
            VarNameSize
            );
          CopyMem (
            VendorGuid,
            &Variable.CurrPtr->VendorGuid,
            sizeof (EFI_GUID)
            );
          Status = EFI_SUCCESS;
        } else {
          Status = EFI_BUFFER_TOO_SMALL;
        }

        *VariableNameSize = VarNameSize;
        return Status;
      }
    }

    Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
  }
}

EFI_STATUS
EFIAPI
SetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data
  )
/*++

Routine Description:

  This code sets variable in storage blocks (Volatile or Non-Volatile)

Arguments:

  VariableName                    Name of Variable to be found
  VendorGuid                      Variable vendor GUID
  Attributes                      Attribute value of the variable found
  DataSize                        Size of Data found. If size is less than the
                                  data, this value contains the required size.
  Data                            Data pointer

Returns:
  
  EFI_INVALID_PARAMETER           - Invalid parameter
  EFI_SUCCESS                     - Set successfully
  EFI_OUT_OF_RESOURCES            - Resource not enough to set variable
  EFI_NOT_FOUND                   - Not found
  EFI_DEVICE_ERROR                - Variable can not be saved due to hardware failure
  EFI_WRITE_PROTECTED             - Variable is read-only

--*/
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;

  //
  // Check input parameters
  // 
  if (VariableName == NULL || VariableName[0] == 0 || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (DataSize != 0 && Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Not support authenticated variable write yet.
  //
  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //  Make sure if runtime bit is set, boot service bit is set also
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of PcdGet32 (PcdMaxHardwareErrorVariableSize)
  //  bytes for HwErrRec, and PcdGet32 (PcdMaxVariableSize) bytes for the others.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if ((DataSize > PcdGet32(PcdMaxHardwareErrorVariableSize)) ||                                                       
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + DataSize > PcdGet32(PcdMaxHardwareErrorVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // According to UEFI spec, HARDWARE_ERROR_RECORD variable name convention should be L"HwErrRecXXXX"
    //
    if (StrnCmp(VariableName, L"HwErrRec", StrLen(L"HwErrRec")) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    if ((DataSize > PcdGet32(PcdMaxVariableSize)) ||
        (sizeof (VARIABLE_HEADER) + StrSize (VariableName) + DataSize > PcdGet32(PcdMaxVariableSize))) {
      return EFI_INVALID_PARAMETER;
    }  
  }  

  //
  // Check whether the input variable is already existed
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable);

  //
  // Hook the operation of setting PlatformLangCodes/PlatformLang and LangCodes/Lang
  //
  AutoUpdateLangVariable (VariableName, Data, DataSize);

  Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes, &Variable);

  return Status;
}

EFI_STATUS
EFIAPI
QueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize
  )
/*++

Routine Description:

  This code returns information about the EFI variables.

Arguments:

  Attributes                      Attributes bitmask to specify the type of variables
                                  on which to return information.
  MaximumVariableStorageSize      Pointer to the maximum size of the storage space available
                                  for the EFI variables associated with the attributes specified.
  RemainingVariableStorageSize    Pointer to the remaining size of the storage space available
                                  for the EFI variables associated with the attributes specified.
  MaximumVariableSize             Pointer to the maximum size of the individual EFI variables
                                  associated with the attributes specified.

Returns:

  EFI STATUS
  EFI_INVALID_PARAMETER           - An invalid combination of attribute bits was supplied.
  EFI_SUCCESS                     - Query successfully.
  EFI_UNSUPPORTED                 - The attribute is not supported on this platform.

--*/
{
  VARIABLE_HEADER        *Variable;
  VARIABLE_HEADER        *NextVariable;
  UINT64                 VariableSize;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  UINT64                 CommonVariableTotalSize;
  UINT64                 HwErrVariableTotalSize;

  CommonVariableTotalSize = 0;
  HwErrVariableTotalSize = 0;

  if(MaximumVariableStorageSize == NULL || RemainingVariableStorageSize == NULL || MaximumVariableSize == NULL || Attributes == 0) {
    return EFI_INVALID_PARAMETER;
  }
  
  if((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == 0) {
    //
    // Make sure the Attributes combination is supported by the platform.
    //
    return EFI_UNSUPPORTED;  
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    return EFI_INVALID_PARAMETER;
  } else if (EfiAtRuntime () && !(Attributes & EFI_VARIABLE_RUNTIME_ACCESS)) {
    //
    // Make sure RT Attribute is set if we are in Runtime phase.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    //
    // Make sure Hw Attribute is set with NV.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    // Not support authentiated variable write yet.
    //
    return EFI_UNSUPPORTED;
  }
  
  VariableStoreHeader = (VARIABLE_STORE_HEADER *) mGlobal->VariableBase[
                                (Attributes & EFI_VARIABLE_NON_VOLATILE) ? NonVolatile : Volatile
                                ];
  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size).
  //
  *MaximumVariableStorageSize   = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Harware error record variable needs larger size.
  //
  if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
    *MaximumVariableStorageSize = PcdGet32(PcdHwErrStorageSize);
    *MaximumVariableSize = PcdGet32(PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER);
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      ASSERT (PcdGet32(PcdHwErrStorageSize) < VariableStoreHeader->Size);
      *MaximumVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER) - PcdGet32(PcdHwErrStorageSize);
    }

    //
    // Let *MaximumVariableSize be PcdGet32(PcdMaxVariableSize) with the exception of the variable header size.
    //
    *MaximumVariableSize = PcdGet32(PcdMaxVariableSize) - sizeof (VARIABLE_HEADER);
  }
  
  //
  // Point to the starting address of the variables.
  //
  Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);

  //
  // Now walk through the related variable store.
  //
  while ((Variable < GetEndPointer (VariableStoreHeader)) && IsValidVariableHeader (Variable)) {
    NextVariable = GetNextVariablePtr (Variable);
    VariableSize = (UINT64) (UINTN) NextVariable - (UINT64) (UINTN) Variable;

    if (EfiAtRuntime ()) {
      //
      // we don't take the state of the variables in mind
      // when calculating RemainingVariableStorageSize,
      // since the space occupied by variables not marked with
      // VAR_ADDED is not allowed to be reclaimed in Runtime.
      //
      if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        HwErrVariableTotalSize += VariableSize;
      } else {
        CommonVariableTotalSize += VariableSize;
      }
    } else {
      //
      // Only care about Variables with State VAR_ADDED,because
      // the space not marked as VAR_ADDED is reclaimable now.
      //
      if ((Variable->State == VAR_ADDED) || (Variable->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))) {
        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          HwErrVariableTotalSize += VariableSize;
        } else {
          CommonVariableTotalSize += VariableSize;
        }
      }
    }

    //
    // Go to the next one
    //
    Variable = NextVariable;
  }
  
  if ((Attributes  & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD){
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - HwErrVariableTotalSize;
  } else {
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VariableServiceInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:
  This function does initialization for variable services

Arguments:

  ImageHandle   - The firmware allocated handle for the EFI image.
  SystemTable   - A pointer to the EFI System Table.

Returns:

  Status code.

  EFI_NOT_FOUND     - Variable store area not found.
  EFI_SUCCESS       - Variable services successfully initialized.

--*/
{
  EFI_STATUS                      Status;
  EFI_HANDLE                      NewHandle;
  VS_DEV                          *Dev;
  EFI_PEI_HOB_POINTERS            GuidHob;
  VARIABLE_HEADER                 *Variable;
  VARIABLE_HEADER                 *NextVariable;
  VARIABLE_STORE_HEADER           *VariableStoreHeader;
  EFI_FLASH_MAP_FS_ENTRY_DATA     *FlashMapEntryData;
  EFI_FLASH_SUBAREA_ENTRY         VariableStoreEntry;
  UINT64                          BaseAddress;
  UINT64                          Length;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR GcdDescriptor;

  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  (UINTN) sizeof (VARIABLE_GLOBAL),
                  (VOID**) &mGlobal
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (mGlobal, (UINTN) sizeof (VARIABLE_GLOBAL));

  GuidHob.Raw = GetHobList ();
  FlashMapEntryData = NULL;
  while ((GuidHob.Raw = GetNextGuidHob (&gEfiFlashMapHobGuid, GuidHob.Raw)) != NULL) {
    FlashMapEntryData = (EFI_FLASH_MAP_FS_ENTRY_DATA *) GET_GUID_HOB_DATA (GuidHob.Guid);
    if (FlashMapEntryData->AreaType == EFI_FLASH_AREA_EFI_VARIABLES) {
      break;
    }
    GuidHob.Raw = GET_NEXT_HOB (GuidHob); 
  }

  if (FlashMapEntryData == NULL) {
    DEBUG ((EFI_D_ERROR, "FSVariable: Could not find flash area for variable!\n"));
    Status = EFI_NOT_FOUND;
    return Status;
  }
  
  CopyMem(
    (VOID*)&VariableStoreEntry,
    (VOID*)&FlashMapEntryData->Entries[0],
    sizeof(EFI_FLASH_SUBAREA_ENTRY)
    );

  //
  // Mark the variable storage region of the FLASH as RUNTIME
  //
  BaseAddress = VariableStoreEntry.Base & (~EFI_PAGE_MASK);
  Length      = VariableStoreEntry.Length + (VariableStoreEntry.Base - BaseAddress);
  Length      = (Length + EFI_PAGE_SIZE - 1) & (~EFI_PAGE_MASK);
  Status      = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdDescriptor);
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    return Status;
  }
  Status = gDS->SetMemorySpaceAttributes (
                  BaseAddress,
                  Length,
                  GcdDescriptor.Attributes | EFI_MEMORY_RUNTIME
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    return Status;
  }

  Status = FileStorageConstructor (
             &mGlobal->VariableStore[NonVolatile], 
             &mGlobal->GoVirtualChildEvent[NonVolatile],
             VariableStoreEntry.Base,
             (UINT32) VariableStoreEntry.Length,
             FlashMapEntryData->VolumeId,
             FlashMapEntryData->FilePath
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Volatile Storage
  //
  Status = MemStorageConstructor (
             &mGlobal->VariableStore[Volatile],
             &mGlobal->GoVirtualChildEvent[Volatile],
             VOLATILE_VARIABLE_STORE_SIZE
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Scratch
  //
  Status = gBS->AllocatePool (
                  EfiRuntimeServicesData,
                  VARIABLE_SCRATCH_SIZE,
                  &mGlobal->Scratch
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // 1. NV Storage
  //
  Dev = DEV_FROM_THIS (mGlobal->VariableStore[NonVolatile]);
  VariableStoreHeader = (VARIABLE_STORE_HEADER *) VAR_DATA_PTR (Dev);
  if (GetVariableStoreStatus (VariableStoreHeader) == EfiValid) {
    if (~VariableStoreHeader->Size == 0) {
      VariableStoreHeader->Size = (UINT32) VariableStoreEntry.Length;
    }
  }
  //
  // Calculate LastVariableOffset
  //
  Variable = (VARIABLE_HEADER *) (VariableStoreHeader + 1);
  while (IsValidVariableHeader (Variable)) {
    UINTN VariableSize = 0;
    NextVariable = GetNextVariablePtr (Variable);
    VariableSize = NextVariable - Variable;
    if ((NextVariable->Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
      mGlobal->HwErrVariableTotalSize += HEADER_ALIGN (VariableSize);
    } else {
      mGlobal->CommonVariableTotalSize += HEADER_ALIGN (VariableSize);
    }
    Variable = NextVariable;
  }

  mGlobal->LastVariableOffset[NonVolatile] = (UINTN) Variable - (UINTN) VariableStoreHeader;
  mGlobal->VariableBase[NonVolatile]       = VariableStoreHeader;

  //
  // Reclaim if remaining space is too small
  //
  if ((VariableStoreHeader->Size - mGlobal->LastVariableOffset[NonVolatile]) < VARIABLE_RECLAIM_THRESHOLD) {
    Status = Reclaim (NonVolatile, NULL);
    if (EFI_ERROR (Status)) {
      //
      // Reclaim error
      // we cannot restore to original state
      //
      DEBUG ((EFI_D_ERROR, "FSVariable: Reclaim error (fatal error) - %r\n", Status));
      ASSERT_EFI_ERROR (Status);
    }
  }
  
  //
  // 2. Volatile Storage
  //
  Dev = DEV_FROM_THIS (mGlobal->VariableStore[Volatile]);
  VariableStoreHeader = (VARIABLE_STORE_HEADER *) VAR_DATA_PTR (Dev);
  mGlobal->VariableBase[Volatile] = VAR_DATA_PTR (Dev);  
  mGlobal->LastVariableOffset[Volatile] = sizeof (VARIABLE_STORE_HEADER);
  //
  // init store_header & body in memory.
  //
  mGlobal->VariableStore[Volatile]->Erase (mGlobal->VariableStore[Volatile]);
  mGlobal->VariableStore[Volatile]->Write (
                                   mGlobal->VariableStore[Volatile],
                                   0,
                                   sizeof (VARIABLE_STORE_HEADER),
                                   &mStoreHeaderTemplate
                                   );


  SystemTable->RuntimeServices->GetVariable         = DuetGetVariable;
  SystemTable->RuntimeServices->GetNextVariableName = GetNextVariableName;
  SystemTable->RuntimeServices->SetVariable         = SetVariable;

  SystemTable->RuntimeServices->QueryVariableInfo   = QueryVariableInfo;

  //
  // Now install the Variable Runtime Architectural Protocol on a new handle
  //
  NewHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &NewHandle,
                  &gEfiVariableArchProtocolGuid,
                  NULL,
                  &gEfiVariableWriteArchProtocolGuid,
                  NULL,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}



VOID
EFIAPI
OnVirtualAddressChangeFsv (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  UINTN Index;

  for (Index = 0; Index < MaxType; Index++) {
    mGlobal->GoVirtualChildEvent[Index] (Event, mGlobal->VariableStore[Index]);
    EfiConvertPointer (0, (VOID**) &mGlobal->VariableStore[Index]);
    EfiConvertPointer (0, &mGlobal->VariableBase[Index]);
  }
  EfiConvertPointer (0, (VOID **) &mGlobal->PlatformLangCodes);
  EfiConvertPointer (0, (VOID **) &mGlobal->LangCodes);
  EfiConvertPointer (0, (VOID **) &mGlobal->PlatformLang);
  EfiConvertPointer (0, &mGlobal->Scratch);
  EfiConvertPointer (0, (VOID**) &mGlobal);
}
