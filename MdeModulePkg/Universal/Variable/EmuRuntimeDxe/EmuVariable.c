/** @file

  Emulation Variable services operate on the runtime volatile memory.
  The nonvolatile variable space doesn't exist.

Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Variable.h"

///
/// Don't use module globals after the SetVirtualAddress map is signaled
///
ESAL_VARIABLE_GLOBAL  *mVariableModuleGlobal;

VARIABLE_INFO_ENTRY *gVariableInfo = NULL;

///
/// The size of a 3 character ISO639 language code.
///
#define ISO_639_2_ENTRY_SIZE    3

/**
  Update the variable region with Variable information. These are the same 
  arguments as the EFI Variable services.

  @param[in] VariableName       Name of variable

  @param[in] VendorGuid         Guid of variable

  @param[in] Data               Variable data

  @param[in] DataSize           Size of data. 0 means delete

  @param[in] Attributes 	      Attribues of the variable

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

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param  VariableName                Name of the variable to be found.
  @param  VendorGuid                  Vendor GUID to be found.
  @param  PtrTrack                    VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param  Global                      Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found.

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global
  );

/**
  Acquires lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiAcquireLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiAcquireLock() at boot time, and simply returns
  at runtime

  @param  Lock         A pointer to the lock to acquire

**/
VOID
AcquireLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiAcquireLock (Lock);
  }
}

/**
  Releases lock only at boot time. Simply returns at runtime.

  This is a temperary function which will be removed when
  EfiReleaseLock() in UefiLib can handle the call in UEFI
  Runtimer driver in RT phase.
  It calls EfiReleaseLock() at boot time, and simply returns
  at runtime

  @param  Lock         A pointer to the lock to release

**/
VOID
ReleaseLockOnlyAtBootTime (
  IN EFI_LOCK  *Lock
  )
{
  if (!EfiAtRuntime ()) {
    EfiReleaseLock (Lock);
  }
}

/**
  Gets pointer to the variable data.

  This function gets the pointer to the variable data according
  to the input pointer to the variable header.

  @param  Variable      Pointer to the variable header.

  @return Pointer to variable data

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  if (Variable->StartId != VARIABLE_DATA) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  return (UINT8 *) ((UINTN) GET_VARIABLE_NAME_PTR (Variable) + Variable->NameSize + GET_PAD_SIZE (Variable->NameSize));
}

/**
  Gets pointer to header of the next potential variable.

  This function gets the pointer to the next potential variable header
  according to the input point to the variable header.  The return value
  is not a valid variable if the input variable was the last variable
  in the variabl store.

  @param  Variable      Pointer to header of the next variable

  @return Pointer to next variable header.
  @retval NULL  Input was not a valid variable header.

**/
VARIABLE_HEADER *
GetNextPotentialVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  VARIABLE_HEADER *VarHeader;

  if (Variable->StartId != VARIABLE_DATA) {
    return NULL;
  }
  //
  // Be careful about pad size for alignment
  //
  VarHeader = (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) GetVariableDataPtr (Variable) + Variable->DataSize + GET_PAD_SIZE (Variable->DataSize));

  return VarHeader;
}

/**
  Gets pointer to header of the next variable.

  This function gets the pointer to the next variable header according
  to the input point to the variable header.

  @param  Variable      Pointer to header of the next variable

  @return Pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_HEADER   *Variable
  )
{
  VARIABLE_HEADER *VarHeader;

  VarHeader = GetNextPotentialVariablePtr (Variable);

  if ((VarHeader == NULL) || (VarHeader->StartId != VARIABLE_DATA)) {
    return NULL;
  }

  return VarHeader;
}

/**
  Updates LastVariableOffset variable for the given variable store.

  LastVariableOffset points to the offset to use for the next variable
  when updating the variable store.

  @param[in]   VariableStore       Pointer to the start of the variable store
  @param[out]  LastVariableOffset  Offset to put the next new variable in

**/
VOID
InitializeLocationForLastVariableOffset (
  IN  VARIABLE_STORE_HEADER *VariableStore,
  OUT UINTN                 *LastVariableOffset
  )
{
  VARIABLE_HEADER *VarHeader;

  *LastVariableOffset       = sizeof (VARIABLE_STORE_HEADER);
  VarHeader = (VARIABLE_HEADER*) ((UINT8*)VariableStore + *LastVariableOffset);
  while (VarHeader->StartId == VARIABLE_DATA) {
    VarHeader = GetNextPotentialVariablePtr (VarHeader);

    if (VarHeader != NULL) {
      *LastVariableOffset = (UINTN) VarHeader - (UINTN) VariableStore;
    } else {
      return;
    }
  }
}

/**
  Gets pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param  VolHeader     Pointer to the variale store header

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER       *VolHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) VolHeader + VolHeader->Size);
}

/**
  Routine used to track statistical information about variable usage. 
  The data is stored in the EFI system table so it can be accessed later.
  VariableInfo.efi can dump out the table. Only Boot Services variable 
  accesses are tracked by this code. The PcdVariableCollectStatistics
  build flag controls if this feature is enabled. 

  A read that hits in the cache will have Read and Cache true for 
  the transaction. Data is allocated by this routine, but never
  freed.

  @param[in] VariableName   Name of the Variable to track
  @param[in] VendorGuid     Guid of the Variable to track
  @param[in] Volatile       TRUE if volatile FALSE if non-volatile
  @param[in] Read           TRUE if GetVariable() was called
  @param[in] Write          TRUE if SetVariable() was called
  @param[in] Delete         TRUE if deleted via SetVariable()
  @param[in] Cache          TRUE for a cache hit.

**/
VOID
UpdateVariableInfo (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  IN  BOOLEAN                 Volatile,
  IN  BOOLEAN                 Read,
  IN  BOOLEAN                 Write,
  IN  BOOLEAN                 Delete,
  IN  BOOLEAN                 Cache
  )
{
  VARIABLE_INFO_ENTRY   *Entry;

  if (FeaturePcdGet (PcdVariableCollectStatistics)) {

    if (EfiAtRuntime ()) {
      // Don't collect statistics at runtime
      return;
    }

    if (gVariableInfo == NULL) {
      //
      // on the first call allocate a entry and place a pointer to it in
      // the EFI System Table
      //
      gVariableInfo = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
      ASSERT (gVariableInfo != NULL);

      CopyGuid (&gVariableInfo->VendorGuid, VendorGuid);
      gVariableInfo->Name = AllocateZeroPool (StrSize (VariableName));
      ASSERT (gVariableInfo->Name != NULL);
      StrCpyS (gVariableInfo->Name, StrSize(VariableName)/sizeof(CHAR16), VariableName);
      gVariableInfo->Volatile = Volatile;

      gBS->InstallConfigurationTable (&gEfiVariableGuid, gVariableInfo);
    }

    
    for (Entry = gVariableInfo; Entry != NULL; Entry = Entry->Next) {
      if (CompareGuid (VendorGuid, &Entry->VendorGuid)) {
        if (StrCmp (VariableName, Entry->Name) == 0) {
          if (Read) {
            Entry->ReadCount++;
          }
          if (Write) {
            Entry->WriteCount++;
          }
          if (Delete) {
            Entry->DeleteCount++;
          }
          if (Cache) {
            Entry->CacheCount++;
          }

          return;
        }
      }

      if (Entry->Next == NULL) {
        //
        // If the entry is not in the table add it.
        // Next iteration of the loop will fill in the data
        //
        Entry->Next = AllocateZeroPool (sizeof (VARIABLE_INFO_ENTRY));
        ASSERT (Entry->Next != NULL);

        CopyGuid (&Entry->Next->VendorGuid, VendorGuid);
        Entry->Next->Name = AllocateZeroPool (StrSize (VariableName));
        ASSERT (Entry->Next->Name != NULL);
        StrCpyS (Entry->Next->Name, StrSize(VariableName)/sizeof(CHAR16), VariableName);
        Entry->Next->Volatile = Volatile;
      }

    }
  }
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
    mVariableModuleGlobal->Lang[CompareLength] = '\0';
    return CopyMem (mVariableModuleGlobal->Lang, SupportedLang + Index * CompareLength, CompareLength);

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
        mVariableModuleGlobal->PlatformLang[CompareLength] = '\0';
        return CopyMem (mVariableModuleGlobal->PlatformLang, Supported - CompareLength, CompareLength);
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

          Buffer = Iso639Language ? mVariableModuleGlobal->Lang : mVariableModuleGlobal->PlatformLang;
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
    if (mVariableModuleGlobal->PlatformLangCodes != NULL) {
      FreePool (mVariableModuleGlobal->PlatformLangCodes);
    }
    mVariableModuleGlobal->PlatformLangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mVariableModuleGlobal->PlatformLangCodes != NULL);

    //
    // PlatformLang holds a single language from PlatformLangCodes, 
    // so the size of PlatformLangCodes is enough for the PlatformLang.
    //
    if (mVariableModuleGlobal->PlatformLang != NULL) {
      FreePool (mVariableModuleGlobal->PlatformLang);
    }
    mVariableModuleGlobal->PlatformLang = AllocateRuntimePool (DataSize);
    ASSERT (mVariableModuleGlobal->PlatformLang != NULL);

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
    if (mVariableModuleGlobal->LangCodes != NULL) {
      FreePool (mVariableModuleGlobal->LangCodes);
    }
    mVariableModuleGlobal->LangCodes = AllocateRuntimeCopyPool (DataSize, Data);
    ASSERT (mVariableModuleGlobal->LangCodes != NULL);
  }

  if (SetLanguageCodes 
      && (mVariableModuleGlobal->PlatformLangCodes != NULL)
      && (mVariableModuleGlobal->LangCodes != NULL)) {
    //
    // Update Lang if PlatformLang is already set
    // Update PlatformLang if Lang is already set
    //
    Status = FindVariable (L"PlatformLang", &gEfiGlobalVariableGuid, &Variable, (VARIABLE_GLOBAL *) mVariableModuleGlobal);
    if (!EFI_ERROR (Status)) {
      //
      // Update Lang
      //
      VariableName = L"PlatformLang";
      Data         = GetVariableDataPtr (Variable.CurrPtr);
      DataSize     = Variable.CurrPtr->DataSize;
    } else {
      Status = FindVariable (L"Lang", &gEfiGlobalVariableGuid, &Variable, (VARIABLE_GLOBAL *) mVariableModuleGlobal);
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
    if ((mVariableModuleGlobal->PlatformLangCodes != NULL) && (mVariableModuleGlobal->LangCodes != NULL)) {
      //
      // When setting PlatformLang, firstly get most matched language string from supported language codes.
      //
      BestPlatformLang = VariableGetBestLanguage (mVariableModuleGlobal->PlatformLangCodes, FALSE, Data, NULL);
      if (BestPlatformLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mVariableModuleGlobal->PlatformLangCodes, BestPlatformLang, FALSE);

        //
        // Get the corresponding ISO639 language tag according to RFC4646 language tag.
        //
        BestLang = GetLangFromSupportedLangCodes (mVariableModuleGlobal->LangCodes, Index, TRUE);

        //
        // Successfully convert PlatformLang to Lang, and set the BestLang value into Lang variable simultaneously.
        //
        FindVariable (L"Lang", &gEfiGlobalVariableGuid, &Variable, (VARIABLE_GLOBAL *)mVariableModuleGlobal);

        Status = UpdateVariable (L"Lang", &gEfiGlobalVariableGuid, BestLang, ISO_639_2_ENTRY_SIZE + 1, Attributes, &Variable);

        DEBUG ((EFI_D_INFO, "Variable Driver Auto Update PlatformLang, PlatformLang:%a, Lang:%a\n", BestPlatformLang, BestLang));

        ASSERT_EFI_ERROR(Status);
      }
    }

  } else if (StrCmp (VariableName, L"Lang") == 0) {
    //
    // Update PlatformLang when PlatformLangCodes/LangCodes were set.
    //
    if ((mVariableModuleGlobal->PlatformLangCodes != NULL) && (mVariableModuleGlobal->LangCodes != NULL)) {
      //
      // When setting Lang, firstly get most matched language string from supported language codes.
      //
      BestLang = VariableGetBestLanguage (mVariableModuleGlobal->LangCodes, TRUE, Data, NULL);
      if (BestLang != NULL) {
        //
        // Get the corresponding index in language codes.
        //
        Index = GetIndexFromSupportedLangCodes (mVariableModuleGlobal->LangCodes, BestLang, TRUE);

        //
        // Get the corresponding RFC4646 language tag according to ISO639 language tag.
        //
        BestPlatformLang = GetLangFromSupportedLangCodes (mVariableModuleGlobal->PlatformLangCodes, Index, FALSE);

        //
        // Successfully convert Lang to PlatformLang, and set the BestPlatformLang value into PlatformLang variable simultaneously.
        //
        FindVariable (L"PlatformLang", &gEfiGlobalVariableGuid, &Variable, (VARIABLE_GLOBAL *)mVariableModuleGlobal);

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
  IN      CHAR16          *VariableName,
  IN      EFI_GUID        *VendorGuid,
  IN      VOID            *Data,
  IN      UINTN           DataSize,
  IN      UINT32          Attributes OPTIONAL,
  IN      VARIABLE_POINTER_TRACK *Variable
  )
{
  EFI_STATUS              Status;
  VARIABLE_HEADER         *NextVariable;
  UINTN                   VarNameSize;
  UINTN                   VarNameOffset;
  UINTN                   VarDataOffset;
  UINTN                   VarSize;
  VARIABLE_GLOBAL         *Global;
  UINTN                   NonVolatileVarableStoreSize;

  Global = &mVariableModuleGlobal->VariableGlobal[Physical];

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
      if (Variable->Volatile) {
        Status = EFI_WRITE_PROTECTED;
        goto Done;
      }
      //
      // Only variable have NV attribute can be updated/deleted in Runtime
      //
      if ((Variable->CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
    }

    //
    // Setting a data variable with no access, or zero DataSize attributes
    // specified causes it to be deleted.
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      Variable->CurrPtr->State &= VAR_DELETED;
      UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, FALSE, TRUE, FALSE);
      Status = EFI_SUCCESS;
      goto Done;
    }

    //
    // If the variable is marked valid and the same data has been passed in
    // then return to the caller immediately.
    //
    if (Variable->CurrPtr->DataSize == DataSize &&
        CompareMem (Data, GetVariableDataPtr (Variable->CurrPtr), DataSize) == 0
          ) {
      Status = EFI_SUCCESS;
      goto Done;
    } else if (Variable->CurrPtr->State == VAR_ADDED) {
      //
      // Mark the old variable as in delete transition
      //
      Variable->CurrPtr->State &= VAR_IN_DELETED_TRANSITION;
    }
    
  } else {
    //
    // No found existing variable, Create a new variable
    //  
    
    //
    // Make sure we are trying to create a new variable.
    // Setting a data variable with no access, or zero DataSize attributes means to delete it.    
    //
    if (DataSize == 0 || (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
        
    //
    // Only variable have NV|RT attribute can be created in Runtime
    //
    if (EfiAtRuntime () &&
        (((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) || ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0))) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }         
  }
  
  //
  // Function part - create a new variable and copy the data.
  // Both update a variable and create a variable will come here.
  //
  
  VarNameOffset = sizeof (VARIABLE_HEADER);
  VarNameSize   = StrSize (VariableName);
  VarDataOffset = VarNameOffset + VarNameSize + GET_PAD_SIZE (VarNameSize);
  VarSize       = VarDataOffset + DataSize + GET_PAD_SIZE (DataSize);

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
    NonVolatileVarableStoreSize = ((VARIABLE_STORE_HEADER *)(UINTN)(Global->NonVolatileVariableBase))->Size;
    if ((((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) 
      && ((HEADER_ALIGN (VarSize) + mVariableModuleGlobal->HwErrVariableTotalSize) > PcdGet32 (PcdHwErrStorageSize)))
      || (((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) 
      && ((HEADER_ALIGN (VarSize) + mVariableModuleGlobal->CommonVariableTotalSize) > NonVolatileVarableStoreSize - sizeof (VARIABLE_STORE_HEADER) - PcdGet32 (PcdHwErrStorageSize)))) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    NextVariable  = (VARIABLE_HEADER *) (UINT8 *) (mVariableModuleGlobal->NonVolatileLastVariableOffset
                      + (UINTN) Global->NonVolatileVariableBase);
    mVariableModuleGlobal->NonVolatileLastVariableOffset += HEADER_ALIGN (VarSize);

    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
      mVariableModuleGlobal->HwErrVariableTotalSize += HEADER_ALIGN (VarSize);
    } else {
      mVariableModuleGlobal->CommonVariableTotalSize += HEADER_ALIGN (VarSize);
    }
  } else {
    if ((UINT32) (HEADER_ALIGN (VarSize) + mVariableModuleGlobal->VolatileLastVariableOffset) >
          ((VARIABLE_STORE_HEADER *) ((UINTN) (Global->VolatileVariableBase)))->Size
          ) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    NextVariable    = (VARIABLE_HEADER *) (UINT8 *) (mVariableModuleGlobal->VolatileLastVariableOffset
                        + (UINTN) Global->VolatileVariableBase);
    mVariableModuleGlobal->VolatileLastVariableOffset += HEADER_ALIGN (VarSize);
  }

  NextVariable->StartId     = VARIABLE_DATA;
  NextVariable->Attributes  = Attributes;
  NextVariable->State       = VAR_ADDED;
  NextVariable->Reserved    = 0;

  //
  // There will be pad bytes after Data, the NextVariable->NameSize and
  // NextVariable->NameSize should not include pad size so that variable
  // service can get actual size in GetVariable
  //
  NextVariable->NameSize  = (UINT32)VarNameSize;
  NextVariable->DataSize  = (UINT32)DataSize;

  CopyMem (&NextVariable->VendorGuid, VendorGuid, sizeof (EFI_GUID));
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarNameOffset),
    VariableName,
    VarNameSize
    );
  CopyMem (
    (UINT8 *) ((UINTN) NextVariable + VarDataOffset),
    Data,
    DataSize
    );

  //
  // Mark the old variable as deleted
  //
  if (Variable->CurrPtr != NULL) {
    Variable->CurrPtr->State &= VAR_DELETED;
  }

  UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE, TRUE, FALSE, FALSE);

  Status = EFI_SUCCESS;

Done:
  return Status;
}

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  Otherwise, VariableName and VendorGuid are compared.

  @param  VariableName                Name of the variable to be found.
  @param  VendorGuid                  Vendor GUID to be found.
  @param  PtrTrack                    VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param  Global                      Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found.

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global
  )
{
  VARIABLE_HEADER       *Variable[2];
  VARIABLE_STORE_HEADER *VariableStoreHeader[2];
  UINTN                 Index;

  //
  // 0: Non-Volatile, 1: Volatile
  //
  VariableStoreHeader[0]  = (VARIABLE_STORE_HEADER *) ((UINTN) Global->NonVolatileVariableBase);
  VariableStoreHeader[1]  = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);

  //
  // Start Pointers for the variable.
  // Actual Data Pointer where data can be written.
  //
  Variable[0] = (VARIABLE_HEADER *) HEADER_ALIGN (VariableStoreHeader[0] + 1);
  Variable[1] = (VARIABLE_HEADER *) HEADER_ALIGN (VariableStoreHeader[1] + 1);

  if (VariableName[0] != 0 && VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Find the variable by walk through non-volatile and volatile variable store
  //
  for (Index = 0; Index < 2; Index++) {
    PtrTrack->StartPtr  = (VARIABLE_HEADER *) HEADER_ALIGN (VariableStoreHeader[Index] + 1);
    PtrTrack->EndPtr    = GetEndPointer (VariableStoreHeader[Index]);

    while ((Variable[Index] < GetEndPointer (VariableStoreHeader[Index])) && (Variable[Index] != NULL)) {
      if (Variable[Index]->StartId == VARIABLE_DATA && Variable[Index]->State == VAR_ADDED) {
        if (!(EfiAtRuntime () && ((Variable[Index]->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0))) {
          if (VariableName[0] == 0) {
            PtrTrack->CurrPtr   = Variable[Index];
            PtrTrack->Volatile  = (BOOLEAN) Index;
            return EFI_SUCCESS;
          } else {
            if (CompareGuid (VendorGuid, &Variable[Index]->VendorGuid)) {
              if (CompareMem (VariableName, GET_VARIABLE_NAME_PTR (Variable[Index]), Variable[Index]->NameSize) == 0) {
                PtrTrack->CurrPtr   = Variable[Index];
                PtrTrack->Volatile  = (BOOLEAN) Index;
                return EFI_SUCCESS;
              }
            }
          }
        }
      }

      Variable[Index] = GetNextVariablePtr (Variable[Index]);
    }
  }
  PtrTrack->CurrPtr = NULL;
  return EFI_NOT_FOUND;
}

/**
  This code finds variable in storage blocks (Volatile or Non-Volatile).
  
  @param  VariableName           A Null-terminated Unicode string that is the name of
                                 the vendor's variable.
  @param  VendorGuid             A unique identifier for the vendor.
  @param  Attributes             If not NULL, a pointer to the memory location to return the 
                                 attributes bitmask for the variable.
  @param  DataSize               Size of Data found. If size is less than the
                                 data, this value contains the required size.
  @param  Data                   On input, the size in bytes of the return Data buffer.  
                                 On output, the size of data returned in Data.
  @param  Global                 Pointer to VARIABLE_GLOBAL structure

  @retval EFI_SUCCESS            The function completed successfully. 
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   DataSize is too small for the result.  DataSize has 
                                 been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableName or VendorGuid or DataSize is NULL.

**/
EFI_STATUS
EFIAPI
EmuGetVariable (
  IN      CHAR16            *VariableName,
  IN      EFI_GUID          *VendorGuid,
  OUT     UINT32            *Attributes OPTIONAL,
  IN OUT  UINTN             *DataSize,
  OUT     VOID              *Data,
  IN      VARIABLE_GLOBAL   *Global
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarDataSize;
  EFI_STATUS              Status;
  UINT8                   *VariableDataPtr;

  if (VariableName == NULL || VendorGuid == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] == 0) {
    return EFI_NOT_FOUND;
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  //
  // Find existing variable
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Get data size
  //
  VarDataSize = Variable.CurrPtr->DataSize;
  if (*DataSize >= VarDataSize) {
    if (Data == NULL) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }
    VariableDataPtr = GetVariableDataPtr (Variable.CurrPtr);
    ASSERT (VariableDataPtr != NULL);
    
    CopyMem (Data, VariableDataPtr, VarDataSize);
    if (Attributes != NULL) {
      *Attributes = Variable.CurrPtr->Attributes;
    }

    *DataSize = VarDataSize;
    UpdateVariableInfo (VariableName, VendorGuid, Variable.Volatile, TRUE, FALSE, FALSE, FALSE);
    Status = EFI_SUCCESS;
    goto Done;
  } else {
    *DataSize = VarDataSize;
    Status = EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

/**

  This code Finds the Next available variable.

  @param  VariableNameSize       The size of the VariableName buffer. The size must be large enough to fit input
                                 string supplied in VariableName buffer.
  @param  VariableName           On input, supplies the last VariableName that was returned by GetNextVariableName().
                                 On output, returns the Null-terminated Unicode string of the current variable.
  @param  VendorGuid             On input, supplies the last VendorGuid that was returned by GetNextVariableName().
                                 On output, returns the VendorGuid of the current variable.
  @param  Global                 Pointer to VARIABLE_GLOBAL structure.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   The VariableNameSize is too small for the result.
                                 VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableNameSize or VariableName or VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  The input values of VariableName and VendorGuid are not a name and
                                 GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER  Null-terminator is not found in the first VariableNameSize bytes of
                                 the input VariableName buffer.

**/
EFI_STATUS
EFIAPI
EmuGetNextVariableName (
  IN OUT  UINTN             *VariableNameSize,
  IN OUT  CHAR16            *VariableName,
  IN OUT  EFI_GUID          *VendorGuid,
  IN      VARIABLE_GLOBAL   *Global
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  UINTN                   VarNameSize;
  EFI_STATUS              Status;
  UINTN                   MaxLen;

  if (VariableNameSize == NULL || VariableName == NULL || VendorGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the possible maximum length of name string, including the Null terminator.
  //
  MaxLen = *VariableNameSize / sizeof (CHAR16);
  if ((MaxLen == 0) || (StrnLenS (VariableName, MaxLen) == MaxLen)) {
    //
    // Null-terminator is not found in the first VariableNameSize bytes of the input VariableName buffer,
    // follow spec to return EFI_INVALID_PARAMETER.
    //
    return EFI_INVALID_PARAMETER;
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  if (Variable.CurrPtr == NULL || EFI_ERROR (Status)) {
    //
    // For VariableName is an empty string, FindVariable() will try to find and return
    // the first qualified variable, and if FindVariable() returns error (EFI_NOT_FOUND)
    // as no any variable is found, still go to return the error (EFI_NOT_FOUND).
    //
    if (VariableName[0] != 0) {
      //
      // For VariableName is not an empty string, and FindVariable() returns error as
      // VariableName and VendorGuid are not a name and GUID of an existing variable,
      // there is no way to get next variable, follow spec to return EFI_INVALID_PARAMETER.
      //
      Status = EFI_INVALID_PARAMETER;
    }
    goto Done;
  }

  while (TRUE) {
    if (VariableName[0] != 0) {
      //
      // If variable name is not NULL, get next variable
      //
      Variable.CurrPtr = GetNextVariablePtr (Variable.CurrPtr);
    }
    //
    // If both volatile and non-volatile variable store are parsed,
    // return not found
    //
    if (Variable.CurrPtr >= Variable.EndPtr || Variable.CurrPtr == NULL) {
      Variable.Volatile = (BOOLEAN) (Variable.Volatile ^ ((BOOLEAN) 0x1));
      if (Variable.Volatile) {
        Variable.StartPtr = (VARIABLE_HEADER *) HEADER_ALIGN ((UINTN) (Global->VolatileVariableBase + sizeof (VARIABLE_STORE_HEADER)));
        Variable.EndPtr = (VARIABLE_HEADER *) GetEndPointer ((VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase));
      } else {
        Status = EFI_NOT_FOUND;
        goto Done;
      }

      Variable.CurrPtr = Variable.StartPtr;
      if (Variable.CurrPtr->StartId != VARIABLE_DATA) {
        continue;
      }
    }
    //
    // Variable is found
    //
    if (Variable.CurrPtr->StartId == VARIABLE_DATA && Variable.CurrPtr->State == VAR_ADDED) {
      if (!(EfiAtRuntime () && ((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0))) {
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
        goto Done;
      }
    }
  }

Done:
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;

}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  @param  VariableName           A Null-terminated Unicode string that is the name of the vendor's
                                 variable.  Each VariableName is unique for each 
                                 VendorGuid.  VariableName must contain 1 or more 
                                 Unicode characters.  If VariableName is an empty Unicode 
                                 string, then EFI_INVALID_PARAMETER is returned.
  @param  VendorGuid             A unique identifier for the vendor
  @param  Attributes             Attributes bitmask to set for the variable
  @param  DataSize               The size in bytes of the Data buffer.  A size of zero causes the
                                 variable to be deleted.
  @param  Data                   The contents for the variable
  @param  Global                 Pointer to VARIABLE_GLOBAL structure
  @param  VolatileOffset         The offset of last volatile variable
  @param  NonVolatileOffset      The offset of last non-volatile variable

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as 
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the 
                                 DataSize exceeds the maximum allowed, or VariableName is an empty 
                                 Unicode string, or VendorGuid is NULL.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only or cannot be deleted.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
EmuSetVariable (
  IN CHAR16                  *VariableName,
  IN EFI_GUID                *VendorGuid,
  IN UINT32                  Attributes,
  IN UINTN                   DataSize,
  IN VOID                    *Data,
  IN VARIABLE_GLOBAL         *Global,
  IN UINTN                   *VolatileOffset,
  IN UINTN                   *NonVolatileOffset
  )
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

  
  if ((UINTN)(~0) - DataSize < StrSize(VariableName)){
    //
    // Prevent whole variable size overflow 
    // 
    return EFI_INVALID_PARAMETER;
  }

  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of PcdGet32 (PcdMaxHardwareErrorVariableSize)
  //  bytes for HwErrRec, and PcdGet32 (PcdMaxVariableSize) bytes for the others.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if (StrSize (VariableName) + DataSize > PcdGet32 (PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER)) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // According to UEFI spec, HARDWARE_ERROR_RECORD variable name convention should be L"HwErrRecXXXX"
    //
    if (StrnCmp(VariableName, L"HwErrRec", StrLen(L"HwErrRec")) != 0) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of PcdGet32 (PcdMaxVariableSize) bytes.
  //
    if (StrSize (VariableName) + DataSize > PcdGet32 (PcdMaxVariableSize) - sizeof (VARIABLE_HEADER)) {
      return EFI_INVALID_PARAMETER;
    }  
  }

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  //
  // Check whether the input variable is already existed
  //
  
  Status = FindVariable (VariableName, VendorGuid, &Variable, Global);

  //
  // Hook the operation of setting PlatformLangCodes/PlatformLang and LangCodes/Lang
  //
  AutoUpdateLangVariable (VariableName, Data, DataSize);

  Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes, &Variable);

  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return Status;
}

/**

  This code returns information about the EFI variables.

  @param  Attributes                   Attributes bitmask to specify the type of variables
                                       on which to return information.
  @param  MaximumVariableStorageSize   On output the maximum size of the storage space available for 
                                       the EFI variables associated with the attributes specified.  
  @param  RemainingVariableStorageSize Returns the remaining size of the storage space available for EFI 
                                       variables associated with the attributes specified.
  @param  MaximumVariableSize          Returns the maximum size of an individual EFI variable 
                                       associated with the attributes specified.
  @param  Global                       Pointer to VARIABLE_GLOBAL structure.

  @retval EFI_SUCCESS                  Valid answer returned.
  @retval EFI_INVALID_PARAMETER        An invalid combination of attribute bits was supplied
  @retval EFI_UNSUPPORTED              The attribute is not supported on this platform, and the 
                                       MaximumVariableStorageSize, RemainingVariableStorageSize, 
                                       MaximumVariableSize are undefined.

**/
EFI_STATUS
EFIAPI
EmuQueryVariableInfo (
  IN  UINT32                 Attributes,
  OUT UINT64                 *MaximumVariableStorageSize,
  OUT UINT64                 *RemainingVariableStorageSize,
  OUT UINT64                 *MaximumVariableSize,
  IN  VARIABLE_GLOBAL        *Global
  )
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
  } else if (EfiAtRuntime () && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
    //
    //   Make sure RT Attribute is set if we are in Runtime phase.
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

  AcquireLockOnlyAtBootTime(&Global->VariableServicesLock);

  if((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) Global->VolatileVariableBase);
  } else {
    //
    // Query is Non-Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *) ((UINTN) Global->NonVolatileVariableBase);
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size)
  //
  *MaximumVariableStorageSize   = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Harware error record variable needs larger size.
  //
  if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
    *MaximumVariableStorageSize = PcdGet32 (PcdHwErrStorageSize);
    *MaximumVariableSize = PcdGet32 (PcdMaxHardwareErrorVariableSize) - sizeof (VARIABLE_HEADER);
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      ASSERT (PcdGet32 (PcdHwErrStorageSize) < VariableStoreHeader->Size);
      *MaximumVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER) - PcdGet32 (PcdHwErrStorageSize);
    }

    //
    // Let *MaximumVariableSize be PcdGet32 (PcdMaxVariableSize) with the exception of the variable header size.
    //
    *MaximumVariableSize = PcdGet32 (PcdMaxVariableSize) - sizeof (VARIABLE_HEADER);
  }

  //
  // Point to the starting address of the variables.
  //
  Variable = (VARIABLE_HEADER *) HEADER_ALIGN (VariableStoreHeader + 1);

  //
  // Now walk through the related variable store.
  //
  while (Variable < GetEndPointer (VariableStoreHeader)) {
    NextVariable = GetNextVariablePtr(Variable);
    if (NextVariable == NULL) {
      break;
    }
    VariableSize = (UINT64) (UINTN) NextVariable - (UINT64) (UINTN) Variable;

    if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
      HwErrVariableTotalSize += VariableSize;
    } else {
      CommonVariableTotalSize += VariableSize;
    }

    //
    // Go to the next one.
    //
    Variable = NextVariable;
  }

  if ((Attributes  & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD){
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - HwErrVariableTotalSize;
  } else {
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
  }

  if (*RemainingVariableStorageSize < sizeof (VARIABLE_HEADER)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - sizeof (VARIABLE_HEADER)) < *MaximumVariableSize) {
    *MaximumVariableSize = *RemainingVariableStorageSize - sizeof (VARIABLE_HEADER);
  }
  
  ReleaseLockOnlyAtBootTime (&Global->VariableServicesLock);
  return EFI_SUCCESS;
}

/**
  Initializes variable store area.

  This function allocates memory space for variable store area and initializes its attributes.

  @param  VolatileStore  Indicates if the variable store is volatile.

**/
EFI_STATUS
InitializeVariableStore (
  IN  BOOLEAN               VolatileStore
  )
{
  EFI_STATUS            Status;
  VARIABLE_STORE_HEADER *VariableStore;
  BOOLEAN               FullyInitializeStore;
  EFI_PHYSICAL_ADDRESS  *VariableBase;
  UINTN                 *LastVariableOffset;
  VARIABLE_STORE_HEADER *VariableStoreHeader;
  VARIABLE_HEADER       *Variable;
  VOID                  *VariableData;
  EFI_HOB_GUID_TYPE     *GuidHob;

  FullyInitializeStore = TRUE;

  if (VolatileStore) {
    VariableBase = &mVariableModuleGlobal->VariableGlobal[Physical].VolatileVariableBase;
    LastVariableOffset = &mVariableModuleGlobal->VolatileLastVariableOffset;
  } else {
    VariableBase = &mVariableModuleGlobal->VariableGlobal[Physical].NonVolatileVariableBase;
    LastVariableOffset = &mVariableModuleGlobal->NonVolatileLastVariableOffset;
  }

  //
  // Note that in EdkII variable driver implementation, Hardware Error Record type variable
  // is stored with common variable in the same NV region. So the platform integrator should
  // ensure that the value of PcdHwErrStorageSize is less than or equal to the value of 
  // PcdVariableStoreSize.
  //
  ASSERT (PcdGet32 (PcdHwErrStorageSize) <= PcdGet32 (PcdVariableStoreSize));

  //
  // Allocate memory for variable store.
  //
  if (VolatileStore || (PcdGet64 (PcdEmuVariableNvStoreReserved) == 0)) {
    VariableStore = (VARIABLE_STORE_HEADER *) AllocateRuntimePool (PcdGet32 (PcdVariableStoreSize));
  } else {
    //
    // A memory location has been reserved for the NV variable store.  Certain
    // platforms may be able to preserve a memory range across system resets,
    // thereby providing better NV variable emulation.
    //
    VariableStore =
      (VARIABLE_STORE_HEADER *)(VOID*)(UINTN)
        PcdGet64 (PcdEmuVariableNvStoreReserved);
    if (
         (VariableStore->Size == PcdGet32 (PcdVariableStoreSize)) &&
         (VariableStore->Format == VARIABLE_STORE_FORMATTED) &&
         (VariableStore->State == VARIABLE_STORE_HEALTHY)
       ) {
      DEBUG((
        EFI_D_INFO,
        "Variable Store reserved at %p appears to be valid\n",
        VariableStore
        ));
      FullyInitializeStore = FALSE;
    }
  }

  if (NULL == VariableStore) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (FullyInitializeStore) {
    SetMem (VariableStore, PcdGet32 (PcdVariableStoreSize), 0xff);
  }

  //
  // Variable Specific Data
  //
  *VariableBase             = (EFI_PHYSICAL_ADDRESS) (UINTN) VariableStore;
  InitializeLocationForLastVariableOffset (VariableStore, LastVariableOffset);

  CopyGuid (&VariableStore->Signature, &gEfiVariableGuid);
  VariableStore->Size       = PcdGet32 (PcdVariableStoreSize);
  VariableStore->Format     = VARIABLE_STORE_FORMATTED;
  VariableStore->State      = VARIABLE_STORE_HEALTHY;
  VariableStore->Reserved   = 0;
  VariableStore->Reserved1  = 0;

  if (!VolatileStore) {
    //
    // Get HOB variable store.
    //
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      VariableStoreHeader = (VARIABLE_STORE_HEADER *) GET_GUID_HOB_DATA (GuidHob);
      if (CompareGuid (&VariableStoreHeader->Signature, &gEfiVariableGuid) &&
          (VariableStoreHeader->Format == VARIABLE_STORE_FORMATTED) &&
          (VariableStoreHeader->State == VARIABLE_STORE_HEALTHY)
         ) {
        DEBUG ((EFI_D_INFO, "HOB Variable Store appears to be valid.\n"));
        //
        // Flush the HOB variable to Emulation Variable storage.
        //
        for ( Variable = (VARIABLE_HEADER *) HEADER_ALIGN (VariableStoreHeader + 1)
            ; (Variable < GetEndPointer (VariableStoreHeader) && (Variable != NULL))
            ; Variable = GetNextVariablePtr (Variable)
            ) {
          ASSERT (Variable->State == VAR_ADDED);
          ASSERT ((Variable->Attributes & EFI_VARIABLE_NON_VOLATILE) != 0);
          VariableData = GetVariableDataPtr (Variable);
          Status = EmuSetVariable (
                     GET_VARIABLE_NAME_PTR (Variable),
                     &Variable->VendorGuid,
                     Variable->Attributes,
                     Variable->DataSize,
                     VariableData,
                     &mVariableModuleGlobal->VariableGlobal[Physical],
                     &mVariableModuleGlobal->VolatileLastVariableOffset,
                     &mVariableModuleGlobal->NonVolatileLastVariableOffset
                     );
          ASSERT_EFI_ERROR (Status);
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Initializes variable store area for non-volatile and volatile variable.

  This function allocates and initializes memory space for global context of ESAL
  variable service and variable store area for non-volatile and volatile variable.

  @param  ImageHandle           The Image handle of this driver.
  @param  SystemTable           The pointer of EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
EFIAPI
VariableCommonInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Allocate memory for mVariableModuleGlobal
  //
  mVariableModuleGlobal = (ESAL_VARIABLE_GLOBAL *) AllocateRuntimeZeroPool (
                                                    sizeof (ESAL_VARIABLE_GLOBAL)
                                                   );
  if (NULL == mVariableModuleGlobal) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiInitializeLock(&mVariableModuleGlobal->VariableGlobal[Physical].VariableServicesLock, TPL_NOTIFY);

  //
  // Intialize volatile variable store
  //
  Status = InitializeVariableStore (TRUE);
  if (EFI_ERROR (Status)) {
    FreePool(mVariableModuleGlobal);
    return Status;
  }
  //
  // Intialize non volatile variable store
  //
  Status = InitializeVariableStore (FALSE);

  return Status;
}
