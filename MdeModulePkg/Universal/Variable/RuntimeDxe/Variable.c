/** @file
  The common variable operation routines shared by DXE_RUNTIME variable
  module and DXE_SMM variable module.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable data. They may be input in SMM mode.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  VariableServiceGetNextVariableName () and VariableServiceQueryVariableInfo() are external API.
  They need check input parameter.

  VariableServiceGetVariable() and VariableServiceSetVariable() are external API
  to receive datasize and data buffer. The size should be checked carefully.

  VariableServiceSetVariable() should also check authenticate data to avoid buffer overflow,
  integer overflow. It should also check attribute to avoid authentication bypass.

Copyright (c) 2006 - 2022, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2015-2018 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) Microsoft Corporation.<BR>
Copyright (c) 2022, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"
#include "VariableNonVolatile.h"
#include "VariableParsing.h"
#include "VariableRuntimeCache.h"

VARIABLE_MODULE_GLOBAL *mVariableModuleGlobal = NULL;

///
/// Define a memory cache that improves the search performance for a variable.
/// For EmuNvMode == TRUE, it will be equal to NonVolatileVariableBase.
///
VARIABLE_STORE_HEADER  *mNvVariableCache = NULL;

///
/// Memory cache of Fv Header.
///
EFI_FIRMWARE_VOLUME_HEADER  *mNvFvHeaderCache = NULL;

///
/// The memory entry used for variable statistics data.
///
VARIABLE_INFO_ENTRY  *gVariableInfo = NULL;

///
/// The flag to indicate whether the platform has left the DXE phase of execution.
///
BOOLEAN  mEndOfDxe = FALSE;

///
/// It indicates the var check request source.
/// In the implementation, DXE is regarded as untrusted, and SMM is trusted.
///
VAR_CHECK_REQUEST_SOURCE  mRequestSource = VarCheckFromUntrusted;

//
// It will record the current boot error flag before EndOfDxe.
//
VAR_ERROR_FLAG  mCurrentBootVarErrFlag = VAR_ERROR_FLAG_NO_ERROR;

VARIABLE_ENTRY_PROPERTY  mVariableEntryProperty[] = {
  {
    &gEdkiiVarErrorFlagGuid,
    VAR_ERROR_FLAG_NAME,
    {
      VAR_CHECK_VARIABLE_PROPERTY_REVISION,
      VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY,
      VARIABLE_ATTRIBUTE_NV_BS_RT,
      sizeof (VAR_ERROR_FLAG),
      sizeof (VAR_ERROR_FLAG)
    }
  },
};

AUTH_VAR_LIB_CONTEXT_IN  mAuthContextIn = {
  AUTH_VAR_LIB_CONTEXT_IN_STRUCT_VERSION,
  //
  // StructSize, TO BE FILLED
  //
  0,
  //
  // MaxAuthVariableSize, TO BE FILLED
  //
  0,
  VariableExLibFindVariable,
  VariableExLibFindNextVariable,
  VariableExLibUpdateVariable,
  VariableExLibGetScratchBuffer,
  VariableExLibCheckRemainingSpaceForConsistency,
  VariableExLibAtRuntime,
};

AUTH_VAR_LIB_CONTEXT_OUT  mAuthContextOut;

/**

  This function writes data to the FWH at the correct LBA even if the LBAs
  are fragmented.

  @param Global                  Pointer to VARAIBLE_GLOBAL structure.
  @param Volatile                Point out the Variable is Volatile or Non-Volatile.
  @param SetByIndex              TRUE if target pointer is given as index.
                                 FALSE if target pointer is absolute.
  @param Fvb                     Pointer to the writable FVB protocol.
  @param DataPtrIndex            Pointer to the Data from the end of VARIABLE_STORE_HEADER
                                 structure.
  @param DataSize                Size of data to be written.
  @param Buffer                  Pointer to the buffer from which data is written.

  @retval EFI_INVALID_PARAMETER  Parameters not valid.
  @retval EFI_UNSUPPORTED        Fvb is a NULL for Non-Volatile variable update.
  @retval EFI_OUT_OF_RESOURCES   The remaining size is not enough.
  @retval EFI_SUCCESS            Variable store successfully updated.

**/
EFI_STATUS
UpdateVariableStore (
  IN  VARIABLE_GLOBAL                     *Global,
  IN  BOOLEAN                             Volatile,
  IN  BOOLEAN                             SetByIndex,
  IN  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb,
  IN  UINTN                               DataPtrIndex,
  IN  UINT32                              DataSize,
  IN  UINT8                               *Buffer
  )
{
  EFI_FV_BLOCK_MAP_ENTRY  *PtrBlockMapEntry;
  UINTN                   BlockIndex2;
  UINTN                   LinearOffset;
  UINTN                   CurrWriteSize;
  UINTN                   CurrWritePtr;
  UINT8                   *CurrBuffer;
  EFI_LBA                 LbaNumber;
  UINTN                   Size;
  VARIABLE_STORE_HEADER   *VolatileBase;
  EFI_PHYSICAL_ADDRESS    FvVolHdr;
  EFI_PHYSICAL_ADDRESS    DataPtr;
  EFI_STATUS              Status;

  FvVolHdr = 0;
  DataPtr  = DataPtrIndex;

  //
  // Check if the Data is Volatile.
  //
  if (!Volatile && !mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    if (Fvb == NULL) {
      return EFI_UNSUPPORTED;
    }

    Status = Fvb->GetPhysicalAddress (Fvb, &FvVolHdr);
    ASSERT_EFI_ERROR (Status);

    //
    // Data Pointer should point to the actual Address where data is to be
    // written.
    //
    if (SetByIndex) {
      DataPtr += mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
    }

    if ((DataPtr + DataSize) > (FvVolHdr + mNvFvHeaderCache->FvLength)) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // Data Pointer should point to the actual Address where data is to be
    // written.
    //
    if (Volatile) {
      VolatileBase = (VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
      if (SetByIndex) {
        DataPtr += mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
      }

      if ((DataPtr + DataSize) > ((UINTN)VolatileBase + VolatileBase->Size)) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      //
      // Emulated non-volatile variable mode.
      //
      if (SetByIndex) {
        DataPtr += (UINTN)mNvVariableCache;
      }

      if ((DataPtr + DataSize) > ((UINTN)mNvVariableCache + mNvVariableCache->Size)) {
        return EFI_OUT_OF_RESOURCES;
      }
    }

    //
    // If Volatile/Emulated Non-volatile Variable just do a simple mem copy.
    //
    CopyMem ((UINT8 *)(UINTN)DataPtr, Buffer, DataSize);
    return EFI_SUCCESS;
  }

  //
  // If we are here we are dealing with Non-Volatile Variables.
  //
  LinearOffset  = (UINTN)FvVolHdr;
  CurrWritePtr  = (UINTN)DataPtr;
  CurrWriteSize = DataSize;
  CurrBuffer    = Buffer;
  LbaNumber     = 0;

  if (CurrWritePtr < LinearOffset) {
    return EFI_INVALID_PARAMETER;
  }

  for (PtrBlockMapEntry = mNvFvHeaderCache->BlockMap; PtrBlockMapEntry->NumBlocks != 0; PtrBlockMapEntry++) {
    for (BlockIndex2 = 0; BlockIndex2 < PtrBlockMapEntry->NumBlocks; BlockIndex2++) {
      //
      // Check to see if the Variable Writes are spanning through multiple
      // blocks.
      //
      if ((CurrWritePtr >= LinearOffset) && (CurrWritePtr < LinearOffset + PtrBlockMapEntry->Length)) {
        if ((CurrWritePtr + CurrWriteSize) <= (LinearOffset + PtrBlockMapEntry->Length)) {
          Status = Fvb->Write (
                          Fvb,
                          LbaNumber,
                          (UINTN)(CurrWritePtr - LinearOffset),
                          &CurrWriteSize,
                          CurrBuffer
                          );
          return Status;
        } else {
          Size   = (UINT32)(LinearOffset + PtrBlockMapEntry->Length - CurrWritePtr);
          Status = Fvb->Write (
                          Fvb,
                          LbaNumber,
                          (UINTN)(CurrWritePtr - LinearOffset),
                          &Size,
                          CurrBuffer
                          );
          if (EFI_ERROR (Status)) {
            return Status;
          }

          CurrWritePtr  = LinearOffset + PtrBlockMapEntry->Length;
          CurrBuffer    = CurrBuffer + Size;
          CurrWriteSize = CurrWriteSize - Size;
        }
      }

      LinearOffset += PtrBlockMapEntry->Length;
      LbaNumber++;
    }
  }

  return EFI_SUCCESS;
}

/**
  Record variable error flag.

  @param[in] Flag               Variable error flag to record.
  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Attributes         Attributes of the variable.
  @param[in] VariableSize       Size of the variable.

**/
VOID
RecordVarErrorFlag (
  IN VAR_ERROR_FLAG  Flag,
  IN CHAR16          *VariableName,
  IN EFI_GUID        *VendorGuid,
  IN UINT32          Attributes,
  IN UINTN           VariableSize
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  VAR_ERROR_FLAG          *VarErrFlag;
  VAR_ERROR_FLAG          TempFlag;

  DEBUG_CODE_BEGIN ();
  DEBUG ((DEBUG_ERROR, "RecordVarErrorFlag (0x%02x) %s:%g - 0x%08x - 0x%x\n", Flag, VariableName, VendorGuid, Attributes, VariableSize));
  if (Flag == VAR_ERROR_FLAG_SYSTEM_ERROR) {
    if (AtRuntime ()) {
      DEBUG ((DEBUG_ERROR, "CommonRuntimeVariableSpace = 0x%x - CommonVariableTotalSize = 0x%x\n", mVariableModuleGlobal->CommonRuntimeVariableSpace, mVariableModuleGlobal->CommonVariableTotalSize));
    } else {
      DEBUG ((DEBUG_ERROR, "CommonVariableSpace = 0x%x - CommonVariableTotalSize = 0x%x\n", mVariableModuleGlobal->CommonVariableSpace, mVariableModuleGlobal->CommonVariableTotalSize));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "CommonMaxUserVariableSpace = 0x%x - CommonUserVariableTotalSize = 0x%x\n", mVariableModuleGlobal->CommonMaxUserVariableSpace, mVariableModuleGlobal->CommonUserVariableTotalSize));
  }

  DEBUG_CODE_END ();

  if (!mEndOfDxe) {
    //
    // Before EndOfDxe, just record the current boot variable error flag to local variable,
    // and leave the variable error flag in NV flash as the last boot variable error flag.
    // After EndOfDxe in InitializeVarErrorFlag (), the variable error flag in NV flash
    // will be initialized to this local current boot variable error flag.
    //
    mCurrentBootVarErrFlag &= Flag;
    return;
  }

  //
  // Record error flag (it should have be initialized).
  //
  Status = FindVariable (
             VAR_ERROR_FLAG_NAME,
             &gEdkiiVarErrorFlagGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  if (!EFI_ERROR (Status)) {
    VarErrFlag = (VAR_ERROR_FLAG *)GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
    TempFlag   = *VarErrFlag;
    TempFlag  &= Flag;
    if (TempFlag == *VarErrFlag) {
      return;
    }

    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               FALSE,
               FALSE,
               mVariableModuleGlobal->FvbInstance,
               (UINTN)VarErrFlag - (UINTN)mNvVariableCache + (UINTN)mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
               sizeof (TempFlag),
               &TempFlag
               );
    if (!EFI_ERROR (Status)) {
      //
      // Update the data in NV cache.
      //
      *VarErrFlag = TempFlag;
      Status      =  SynchronizeRuntimeVariableCache (
                       &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeNvCache,
                       0,
                       mNvVariableCache->Size
                       );
      ASSERT_EFI_ERROR (Status);
    }
  }
}

/**
  Initialize variable error flag.

  Before EndOfDxe, the variable indicates the last boot variable error flag,
  then it means the last boot variable error flag must be got before EndOfDxe.
  After EndOfDxe, the variable indicates the current boot variable error flag,
  then it means the current boot variable error flag must be got after EndOfDxe.

**/
VOID
InitializeVarErrorFlag (
  VOID
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;
  VAR_ERROR_FLAG          Flag;
  VAR_ERROR_FLAG          VarErrFlag;

  if (!mEndOfDxe) {
    return;
  }

  Flag = mCurrentBootVarErrFlag;
  DEBUG ((DEBUG_INFO, "Initialize variable error flag (%02x)\n", Flag));

  Status = FindVariable (
             VAR_ERROR_FLAG_NAME,
             &gEdkiiVarErrorFlagGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  if (!EFI_ERROR (Status)) {
    VarErrFlag = *((VAR_ERROR_FLAG *)GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat));
    if (VarErrFlag == Flag) {
      return;
    }
  }

  UpdateVariable (
    VAR_ERROR_FLAG_NAME,
    &gEdkiiVarErrorFlagGuid,
    &Flag,
    sizeof (Flag),
    VARIABLE_ATTRIBUTE_NV_BS_RT,
    0,
    0,
    &Variable,
    NULL
    );
}

/**
  Is user variable?

  @param[in] Variable   Pointer to variable header.

  @retval TRUE          User variable.
  @retval FALSE         System variable.

**/
BOOLEAN
IsUserVariable (
  IN VARIABLE_HEADER  *Variable
  )
{
  VAR_CHECK_VARIABLE_PROPERTY  Property;

  //
  // Only after End Of Dxe, the variables belong to system variable are fixed.
  // If PcdMaxUserNvStorageVariableSize is 0, it means user variable share the same NV storage with system variable,
  // then no need to check if the variable is user variable or not specially.
  //
  if (mEndOfDxe && (mVariableModuleGlobal->CommonMaxUserVariableSpace != mVariableModuleGlobal->CommonVariableSpace)) {
    if (VarCheckLibVariablePropertyGet (
          GetVariableNamePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
          GetVendorGuidPtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
          &Property
          ) == EFI_NOT_FOUND) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Calculate common user variable total size.

**/
VOID
CalculateCommonUserVariableTotalSize (
  VOID
  )
{
  VARIABLE_HEADER              *Variable;
  VARIABLE_HEADER              *NextVariable;
  UINTN                        VariableSize;
  VAR_CHECK_VARIABLE_PROPERTY  Property;

  //
  // Only after End Of Dxe, the variables belong to system variable are fixed.
  // If PcdMaxUserNvStorageVariableSize is 0, it means user variable share the same NV storage with system variable,
  // then no need to calculate the common user variable total size specially.
  //
  if (mEndOfDxe && (mVariableModuleGlobal->CommonMaxUserVariableSpace != mVariableModuleGlobal->CommonVariableSpace)) {
    Variable = GetStartPointer (mNvVariableCache);
    while (IsValidVariableHeader (Variable, GetEndPointer (mNvVariableCache), mVariableModuleGlobal->VariableGlobal.AuthFormat)) {
      NextVariable = GetNextVariablePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      VariableSize = (UINTN)NextVariable - (UINTN)Variable;
      if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
        if (VarCheckLibVariablePropertyGet (
              GetVariableNamePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
              GetVendorGuidPtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
              &Property
              ) == EFI_NOT_FOUND) {
          //
          // No property, it is user variable.
          //
          mVariableModuleGlobal->CommonUserVariableTotalSize += VariableSize;
        }
      }

      Variable = NextVariable;
    }
  }
}

/**
  Initialize variable quota.

**/
VOID
InitializeVariableQuota (
  VOID
  )
{
  if (!mEndOfDxe) {
    return;
  }

  InitializeVarErrorFlag ();
  CalculateCommonUserVariableTotalSize ();
}

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.
  If IgnoreRtCheck is TRUE, then we ignore the EFI_VARIABLE_RUNTIME_ACCESS attribute check
  at runtime when searching existing variable, only VariableName and VendorGuid are compared.
  Otherwise, variables without EFI_VARIABLE_RUNTIME_ACCESS are not visible at runtime.

  @param[in]   VariableName           Name of the variable to be found.
  @param[in]   VendorGuid             Vendor GUID to be found.
  @param[out]  PtrTrack               VARIABLE_POINTER_TRACK structure for output,
                                      including the range searched and the target position.
  @param[in]   Global                 Pointer to VARIABLE_GLOBAL structure, including
                                      base of volatile variable storage area, base of
                                      NV variable storage area, and a lock.
  @param[in]   IgnoreRtCheck          Ignore EFI_VARIABLE_RUNTIME_ACCESS attribute
                                      check at runtime when searching variable.

  @retval EFI_INVALID_PARAMETER       If VariableName is not an empty string, while
                                      VendorGuid is NULL.
  @retval EFI_SUCCESS                 Variable successfully found.
  @retval EFI_NOT_FOUND               Variable not found

**/
EFI_STATUS
FindVariable (
  IN  CHAR16                  *VariableName,
  IN  EFI_GUID                *VendorGuid,
  OUT VARIABLE_POINTER_TRACK  *PtrTrack,
  IN  VARIABLE_GLOBAL         *Global,
  IN  BOOLEAN                 IgnoreRtCheck
  )
{
  EFI_STATUS             Status;
  VARIABLE_STORE_HEADER  *VariableStoreHeader[VariableStoreTypeMax];
  VARIABLE_STORE_TYPE    Type;

  if ((VariableName[0] != 0) && (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // 0: Volatile, 1: HOB, 2: Non-Volatile.
  // The index and attributes mapping must be kept in this order as RuntimeServiceGetNextVariableName
  // make use of this mapping to implement search algorithm.
  //
  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)Global->VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)Global->HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  //
  // Find the variable by walk through HOB, volatile and non-volatile variable store.
  //
  for (Type = (VARIABLE_STORE_TYPE)0; Type < VariableStoreTypeMax; Type++) {
    if (VariableStoreHeader[Type] == NULL) {
      continue;
    }

    PtrTrack->StartPtr = GetStartPointer (VariableStoreHeader[Type]);
    PtrTrack->EndPtr   = GetEndPointer (VariableStoreHeader[Type]);
    PtrTrack->Volatile = (BOOLEAN)(Type == VariableStoreTypeVolatile);

    Status =  FindVariableEx (
                VariableName,
                VendorGuid,
                IgnoreRtCheck,
                PtrTrack,
                mVariableModuleGlobal->VariableGlobal.AuthFormat
                );
    if (!EFI_ERROR (Status)) {
      return Status;
    }
  }

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

  @retval The index of language in the language codes.

**/
UINTN
GetIndexFromSupportedLangCodes (
  IN  CHAR8    *SupportedLang,
  IN  CHAR8    *Lang,
  IN  BOOLEAN  Iso639Language
  )
{
  UINTN  Index;
  UINTN  CompareLength;
  UINTN  LanguageLength;

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
    for (LanguageLength = 0; Lang[LanguageLength] != '\0'; LanguageLength++) {
    }

    for (Index = 0; *SupportedLang != '\0'; Index++, SupportedLang += CompareLength) {
      //
      // Skip ';' characters in SupportedLang
      //
      for ( ; *SupportedLang != '\0' && *SupportedLang == ';'; SupportedLang++) {
      }

      //
      // Determine the length of the next language code in SupportedLang
      //
      for (CompareLength = 0; SupportedLang[CompareLength] != '\0' && SupportedLang[CompareLength] != ';'; CompareLength++) {
      }

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

  This code is used to get corresponding language strings in supported language codes. It can handle
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
  @param  Index                       The index in supported language codes.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval The language string in the language codes.

**/
CHAR8 *
GetLangFromSupportedLangCodes (
  IN  CHAR8    *SupportedLang,
  IN  UINTN    Index,
  IN  BOOLEAN  Iso639Language
  )
{
  UINTN  SubIndex;
  UINTN  CompareLength;
  CHAR8  *Supported;

  SubIndex  = 0;
  Supported = SupportedLang;
  if (Iso639Language) {
    //
    // According to the index of Lang string in SupportedLang string to get the language.
    // This code will be invoked in RUNTIME, therefore there is not a memory allocate/free operation.
    // In driver entry, it pre-allocates a runtime attribute memory to accommodate this string.
    //
    CompareLength                              = ISO_639_2_ENTRY_SIZE;
    mVariableModuleGlobal->Lang[CompareLength] = '\0';
    return CopyMem (mVariableModuleGlobal->Lang, SupportedLang + Index * CompareLength, CompareLength);
  } else {
    while (TRUE) {
      //
      // Take semicolon as delimitation, sequentially traverse supported language codes.
      //
      for (CompareLength = 0; *Supported != ';' && *Supported != '\0'; CompareLength++) {
        Supported++;
      }

      if ((*Supported == '\0') && (SubIndex != Index)) {
        //
        // Have completed the traverse, but not find corrsponding string.
        // This case is not allowed to happen.
        //
        ASSERT (FALSE);
        return NULL;
      }

      if (SubIndex == Index) {
        //
        // According to the index of Lang string in SupportedLang string to get the language.
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
      for ( ; *Supported != '\0' && *Supported == ';'; Supported++) {
      }
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
  @param[in]  Iso639Language      If not zero, then all language codes are assumed to be
                                  in ISO 639-2 format.  If zero, then all language
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
  IN UINTN        Iso639Language,
  ...
  )
{
  VA_LIST      Args;
  CHAR8        *Language;
  UINTN        CompareLength;
  UINTN        LanguageLength;
  CONST CHAR8  *Supported;
  CHAR8        *Buffer;

  if (SupportedLanguages == NULL) {
    return NULL;
  }

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
    if (Iso639Language == 0) {
      for (LanguageLength = 0; Language[LanguageLength] != 0 && Language[LanguageLength] != ';'; LanguageLength++) {
      }
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
        if (Iso639Language == 0) {
          //
          // Skip ';' characters in Supported
          //
          for ( ; *Supported != '\0' && *Supported == ';'; Supported++) {
          }

          //
          // Determine the length of the next language code in Supported
          //
          for (CompareLength = 0; Supported[CompareLength] != 0 && Supported[CompareLength] != ';'; CompareLength++) {
          }

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

          Buffer                = (Iso639Language != 0) ? mVariableModuleGlobal->Lang : mVariableModuleGlobal->PlatformLang;
          Buffer[CompareLength] = '\0';
          return CopyMem (Buffer, Supported, CompareLength);
        }
      }

      if (Iso639Language != 0) {
        //
        // If ISO 639 mode, then each language can only be tested once
        //
        LanguageLength = 0;
      } else {
        //
        // If RFC 4646 mode, then trim Language from the right to the next '-' character
        //
        for (LanguageLength--; LanguageLength > 0 && Language[LanguageLength] != '-'; LanguageLength--) {
        }
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
  This function is to check if the remaining variable space is enough to set
  all Variables from argument list successfully. The purpose of the check
  is to keep the consistency of the Variables to be in variable storage.

  Note: Variables are assumed to be in same storage.
  The set sequence of Variables will be same with the sequence of VariableEntry from argument list,
  so follow the argument sequence to check the Variables.

  @param[in] Attributes         Variable attributes for Variable entries.
  @param[in] Marker             VA_LIST style variable argument list.
                                The variable argument list with type VARIABLE_ENTRY_CONSISTENCY *.
                                A NULL terminates the list. The VariableSize of
                                VARIABLE_ENTRY_CONSISTENCY is the variable data size as input.
                                It will be changed to variable total size as output.

  @retval TRUE                  Have enough variable space to set the Variables successfully.
  @retval FALSE                 No enough variable space to set the Variables successfully.

**/
BOOLEAN
EFIAPI
CheckRemainingSpaceForConsistencyInternal (
  IN UINT32   Attributes,
  IN VA_LIST  Marker
  )
{
  EFI_STATUS                  Status;
  VA_LIST                     Args;
  VARIABLE_ENTRY_CONSISTENCY  *VariableEntry;
  UINT64                      MaximumVariableStorageSize;
  UINT64                      RemainingVariableStorageSize;
  UINT64                      MaximumVariableSize;
  UINTN                       TotalNeededSize;
  UINTN                       OriginalVarSize;
  VARIABLE_STORE_HEADER       *VariableStoreHeader;
  VARIABLE_POINTER_TRACK      VariablePtrTrack;
  VARIABLE_HEADER             *NextVariable;
  UINTN                       VarNameSize;
  UINTN                       VarDataSize;

  //
  // Non-Volatile related.
  //
  VariableStoreHeader = mNvVariableCache;

  Status = VariableServiceQueryVariableInfoInternal (
             Attributes,
             &MaximumVariableStorageSize,
             &RemainingVariableStorageSize,
             &MaximumVariableSize
             );
  ASSERT_EFI_ERROR (Status);

  TotalNeededSize = 0;
  VA_COPY (Args, Marker);
  VariableEntry = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  while (VariableEntry != NULL) {
    //
    // Calculate variable total size.
    //
    VarNameSize                 = StrSize (VariableEntry->Name);
    VarNameSize                += GET_PAD_SIZE (VarNameSize);
    VarDataSize                 = VariableEntry->VariableSize;
    VarDataSize                += GET_PAD_SIZE (VarDataSize);
    VariableEntry->VariableSize = HEADER_ALIGN (
                                    GetVariableHeaderSize (
                                      mVariableModuleGlobal->VariableGlobal.AuthFormat
                                      ) + VarNameSize + VarDataSize
                                    );

    TotalNeededSize += VariableEntry->VariableSize;
    VariableEntry    = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  }

  VA_END (Args);

  if (RemainingVariableStorageSize >= TotalNeededSize) {
    //
    // Already have enough space.
    //
    return TRUE;
  } else if (AtRuntime ()) {
    //
    // At runtime, no reclaim.
    // The original variable space of Variables can't be reused.
    //
    return FALSE;
  }

  VA_COPY (Args, Marker);
  VariableEntry = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  while (VariableEntry != NULL) {
    //
    // Check if Variable[Index] has been present and get its size.
    //
    OriginalVarSize           = 0;
    VariablePtrTrack.StartPtr = GetStartPointer (VariableStoreHeader);
    VariablePtrTrack.EndPtr   = GetEndPointer (VariableStoreHeader);
    Status                    = FindVariableEx (
                                  VariableEntry->Name,
                                  VariableEntry->Guid,
                                  FALSE,
                                  &VariablePtrTrack,
                                  mVariableModuleGlobal->VariableGlobal.AuthFormat
                                  );
    if (!EFI_ERROR (Status)) {
      //
      // Get size of Variable[Index].
      //
      NextVariable    = GetNextVariablePtr (VariablePtrTrack.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      OriginalVarSize = (UINTN)NextVariable - (UINTN)VariablePtrTrack.CurrPtr;
      //
      // Add the original size of Variable[Index] to remaining variable storage size.
      //
      RemainingVariableStorageSize += OriginalVarSize;
    }

    if (VariableEntry->VariableSize > RemainingVariableStorageSize) {
      //
      // No enough space for Variable[Index].
      //
      VA_END (Args);
      return FALSE;
    }

    //
    // Sub the (new) size of Variable[Index] from remaining variable storage size.
    //
    RemainingVariableStorageSize -= VariableEntry->VariableSize;
    VariableEntry                 = VA_ARG (Args, VARIABLE_ENTRY_CONSISTENCY *);
  }

  VA_END (Args);

  return TRUE;
}

/**
  This function is to check if the remaining variable space is enough to set
  all Variables from argument list successfully. The purpose of the check
  is to keep the consistency of the Variables to be in variable storage.

  Note: Variables are assumed to be in same storage.
  The set sequence of Variables will be same with the sequence of VariableEntry from argument list,
  so follow the argument sequence to check the Variables.

  @param[in] Attributes         Variable attributes for Variable entries.
  @param ...                    The variable argument list with type VARIABLE_ENTRY_CONSISTENCY *.
                                A NULL terminates the list. The VariableSize of
                                VARIABLE_ENTRY_CONSISTENCY is the variable data size as input.
                                It will be changed to variable total size as output.

  @retval TRUE                  Have enough variable space to set the Variables successfully.
  @retval FALSE                 No enough variable space to set the Variables successfully.

**/
BOOLEAN
EFIAPI
CheckRemainingSpaceForConsistency (
  IN UINT32  Attributes,
  ...
  )
{
  VA_LIST  Marker;
  BOOLEAN  Return;

  VA_START (Marker, Attributes);

  Return = CheckRemainingSpaceForConsistencyInternal (Attributes, Marker);

  VA_END (Marker);

  return Return;
}

/**
  Hook the operations in PlatformLangCodes, LangCodes, PlatformLang and Lang.

  When setting Lang/LangCodes, simultaneously update PlatformLang/PlatformLangCodes.

  According to UEFI spec, PlatformLangCodes/LangCodes are only set once in firmware initialization,
  and are read-only. Therefore, in variable driver, only store the original value for other use.

  @param[in] VariableName       Name of variable.

  @param[in] Data               Variable data.

  @param[in] DataSize           Size of data. 0 means delete.

  @retval EFI_SUCCESS           The update operation is successful or ignored.
  @retval EFI_WRITE_PROTECTED   Update PlatformLangCodes/LangCodes at runtime.
  @retval EFI_OUT_OF_RESOURCES  No enough variable space to do the update operation.
  @retval Others                Other errors happened during the update operation.

**/
EFI_STATUS
AutoUpdateLangVariable (
  IN  CHAR16  *VariableName,
  IN  VOID    *Data,
  IN  UINTN   DataSize
  )
{
  EFI_STATUS                  Status;
  CHAR8                       *BestPlatformLang;
  CHAR8                       *BestLang;
  UINTN                       Index;
  UINT32                      Attributes;
  VARIABLE_POINTER_TRACK      Variable;
  BOOLEAN                     SetLanguageCodes;
  VARIABLE_ENTRY_CONSISTENCY  VariableEntry[2];

  //
  // Don't do updates for delete operation
  //
  if (DataSize == 0) {
    return EFI_SUCCESS;
  }

  SetLanguageCodes = FALSE;

  if (StrCmp (VariableName, EFI_PLATFORM_LANG_CODES_VARIABLE_NAME) == 0) {
    //
    // PlatformLangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (AtRuntime ()) {
      return EFI_WRITE_PROTECTED;
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
  } else if (StrCmp (VariableName, EFI_LANG_CODES_VARIABLE_NAME) == 0) {
    //
    // LangCodes is a volatile variable, so it can not be updated at runtime.
    //
    if (AtRuntime ()) {
      return EFI_WRITE_PROTECTED;
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
    Status = FindVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
    if (!EFI_ERROR (Status)) {
      //
      // Update Lang
      //
      VariableName = EFI_PLATFORM_LANG_VARIABLE_NAME;
      Data         = GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      DataSize     = DataSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
    } else {
      Status = FindVariable (EFI_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
      if (!EFI_ERROR (Status)) {
        //
        // Update PlatformLang
        //
        VariableName = EFI_LANG_VARIABLE_NAME;
        Data         = GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
        DataSize     = DataSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
      } else {
        //
        // Neither PlatformLang nor Lang is set, directly return
        //
        return EFI_SUCCESS;
      }
    }
  }

  Status = EFI_SUCCESS;

  //
  // According to UEFI spec, "Lang" and "PlatformLang" is NV|BS|RT attributions.
  //
  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  if (StrCmp (VariableName, EFI_PLATFORM_LANG_VARIABLE_NAME) == 0) {
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
        // Check the variable space for both Lang and PlatformLang variable.
        //
        VariableEntry[0].VariableSize = ISO_639_2_ENTRY_SIZE + 1;
        VariableEntry[0].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[0].Name         = EFI_LANG_VARIABLE_NAME;

        VariableEntry[1].VariableSize = AsciiStrSize (BestPlatformLang);
        VariableEntry[1].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[1].Name         = EFI_PLATFORM_LANG_VARIABLE_NAME;
        if (!CheckRemainingSpaceForConsistency (VARIABLE_ATTRIBUTE_NV_BS_RT, &VariableEntry[0], &VariableEntry[1], NULL)) {
          //
          // No enough variable space to set both Lang and PlatformLang successfully.
          //
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          //
          // Successfully convert PlatformLang to Lang, and set the BestLang value into Lang variable simultaneously.
          //
          FindVariable (EFI_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);

          Status = UpdateVariable (
                     EFI_LANG_VARIABLE_NAME,
                     &gEfiGlobalVariableGuid,
                     BestLang,
                     ISO_639_2_ENTRY_SIZE + 1,
                     Attributes,
                     0,
                     0,
                     &Variable,
                     NULL
                     );
        }

        DEBUG ((DEBUG_INFO, "Variable Driver Auto Update PlatformLang, PlatformLang:%a, Lang:%a Status: %r\n", BestPlatformLang, BestLang, Status));
      }
    }
  } else if (StrCmp (VariableName, EFI_LANG_VARIABLE_NAME) == 0) {
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
        // Check the variable space for both PlatformLang and Lang variable.
        //
        VariableEntry[0].VariableSize = AsciiStrSize (BestPlatformLang);
        VariableEntry[0].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[0].Name         = EFI_PLATFORM_LANG_VARIABLE_NAME;

        VariableEntry[1].VariableSize = ISO_639_2_ENTRY_SIZE + 1;
        VariableEntry[1].Guid         = &gEfiGlobalVariableGuid;
        VariableEntry[1].Name         = EFI_LANG_VARIABLE_NAME;
        if (!CheckRemainingSpaceForConsistency (VARIABLE_ATTRIBUTE_NV_BS_RT, &VariableEntry[0], &VariableEntry[1], NULL)) {
          //
          // No enough variable space to set both PlatformLang and Lang successfully.
          //
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          //
          // Successfully convert Lang to PlatformLang, and set the BestPlatformLang value into PlatformLang variable simultaneously.
          //
          FindVariable (EFI_PLATFORM_LANG_VARIABLE_NAME, &gEfiGlobalVariableGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);

          Status = UpdateVariable (
                     EFI_PLATFORM_LANG_VARIABLE_NAME,
                     &gEfiGlobalVariableGuid,
                     BestPlatformLang,
                     AsciiStrSize (BestPlatformLang),
                     Attributes,
                     0,
                     0,
                     &Variable,
                     NULL
                     );
        }

        DEBUG ((DEBUG_INFO, "Variable Driver Auto Update Lang, Lang:%a, PlatformLang:%a Status: %r\n", BestLang, BestPlatformLang, Status));
      }
    }
  }

  if (SetLanguageCodes) {
    //
    // Continue to set PlatformLangCodes or LangCodes.
    //
    return EFI_SUCCESS;
  } else {
    return Status;
  }
}

/**
  Check if there's enough free space in storage to write the new variable.

  @param[in] NewVariable        Pointer to buffer of new variable.
  @param[in] VariableSize       Size of new variable.
  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Attributes         Attributes of the variable.
  @param[in] VolatileFlag       Volatile/non-volatile variable indicator.

  @retval EFI_SUCCESS           Enough free space on variable storage.
  @retval EFI_BUFFER_TOO_SMALL  There's not enough continuous free space.
  @retval EFI_OUT_OF_RESOURCES  There's not enough free space in total.
**/
EFI_STATUS
CheckVariableStoreSpace (
  IN  VARIABLE_HEADER     *NewVariable,
  IN  UINTN               VariableSize,
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  IN  UINT32              Attributes,
  IN  BOOLEAN             VolatileFlag
  )
{
  BOOLEAN                 IsCommonVariable;
  BOOLEAN                 IsCommonUserVariable;
  UINTN                   CommonVariableTotalSize;
  UINTN                   CommonUserVariableTotalSize;
  UINTN                   HwErrVariableTotalSize;
  VARIABLE_STORE_HEADER   *VarStore;

  if (NewVariable == NULL || VariableSize == 0) {
    return EFI_SUCCESS;
  }

  if (VolatileFlag) {
    VarStore = (VARIABLE_STORE_HEADER *)(UINTN)
               mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
    if ((UINT32)(VariableSize + mVariableModuleGlobal->VolatileLastVariableOffset)
        > VarStore->Size)
    {
      return EFI_BUFFER_TOO_SMALL;
    }
  } else {
    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) {
      IsCommonVariable      = TRUE;
      IsCommonUserVariable  = IsUserVariable (NewVariable);
    } else {
      IsCommonVariable      = FALSE;
      IsCommonUserVariable  = FALSE;
    }

    CommonVariableTotalSize = mVariableModuleGlobal->CommonVariableTotalSize + VariableSize;
    CommonUserVariableTotalSize = mVariableModuleGlobal->CommonUserVariableTotalSize + VariableSize;
    HwErrVariableTotalSize = mVariableModuleGlobal->HwErrVariableTotalSize + VariableSize;

    if (((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0 &&
         HwErrVariableTotalSize > PcdGet32 (PcdHwErrStorageSize))
        || (IsCommonVariable && CommonVariableTotalSize > mVariableModuleGlobal->CommonVariableSpace)
        || (IsCommonVariable &&
            AtRuntime () &&
            CommonVariableTotalSize > mVariableModuleGlobal->CommonRuntimeVariableSpace)
        || (IsCommonUserVariable &&
            CommonUserVariableTotalSize > mVariableModuleGlobal->CommonMaxUserVariableSpace))
    {
      if (AtRuntime ()) {
        if (IsCommonUserVariable &&
            ((VariableSize + mVariableModuleGlobal->CommonUserVariableTotalSize)
             > mVariableModuleGlobal->CommonMaxUserVariableSpace))
        {
          RecordVarErrorFlag (VAR_ERROR_FLAG_USER_ERROR, VariableName, VendorGuid,
                              Attributes, VariableSize);
        }

        if (IsCommonVariable &&
            ((VariableSize + mVariableModuleGlobal->CommonVariableTotalSize)
             > mVariableModuleGlobal->CommonRuntimeVariableSpace))
        {
          RecordVarErrorFlag (VAR_ERROR_FLAG_SYSTEM_ERROR, VariableName, VendorGuid,
                              Attributes, VariableSize);
        }

        return EFI_OUT_OF_RESOURCES;
      }

      return EFI_BUFFER_TOO_SMALL;
    }
  }

  return EFI_SUCCESS;
}

/**
  Fill specific data of auth-variable in buffer.

  @param[in] NewVariable        Pointer to buffer of new variable.
  @param[in] OldVariable        Pointer to buffer of old copy of the variable.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval None.

**/
VOID
SetVariableAuthData (
  IN  OUT AUTHENTICATED_VARIABLE_HEADER     *NewVariable,
  IN      AUTHENTICATED_VARIABLE_HEADER     *OldVariable,
  IN      UINT32                            Attributes,
  IN      UINT32                            KeyIndex,
  IN      UINT64                            MonotonicCount,
  IN      EFI_TIME                          *TimeStamp
  )
{
  NewVariable->PubKeyIndex    = KeyIndex;
  NewVariable->MonotonicCount = MonotonicCount;

  if (TimeStamp != NULL &&
      (Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    // In the case when the EFI_VARIABLE_APPEND_WRITE attribute is set, only
    // when the new TimeStamp value is later than the current timestamp associated
    // with the variable, we need associate the new timestamp with the updated value.
    //
    if ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0 &&
        OldVariable != NULL &&
        !VariableCompareTimeStampInternal (&OldVariable->TimeStamp, TimeStamp)) {
      TimeStamp = &OldVariable->TimeStamp;
    }

    CopyMem (&NewVariable->TimeStamp, TimeStamp, sizeof (EFI_TIME));
  } else {
    ZeroMem (&NewVariable->TimeStamp, sizeof (EFI_TIME));
  }
}

/**
  Fill the variable data buffer according to variable format on storage.

  @param[in] NewVariable        Pointer to buffer of new variable.
  @param[in] OldVariable        Pointer to buffer of old copy of the variable.
  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval Size of the new variable.

**/
UINTN
SetVariableData (
  IN  OUT VARIABLE_HEADER     *NewVariable,
  IN      VARIABLE_HEADER     *OldVariable,
  IN      CHAR16              *VariableName,
  IN      EFI_GUID            *VendorGuid,
  IN      VOID                *Data,
  IN      UINTN               DataSize,
  IN      UINT32              Attributes,
  IN      UINT32              KeyIndex,
  IN      UINT64              MonotonicCount,
  IN      EFI_TIME            *TimeStamp
  )
{
  EFI_STATUS  Status;
  BOOLEAN     AuthFormat;
  UINT8       *DataPtr;
  UINTN       NameSize;
  UINTN       OldDataSize;

  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  if (AuthFormat) {
    SetVariableAuthData (
      (AUTHENTICATED_VARIABLE_HEADER *)NewVariable,
      (AUTHENTICATED_VARIABLE_HEADER *)OldVariable,
      Attributes,
      KeyIndex,
      MonotonicCount,
      TimeStamp
      );
  }

  NewVariable->StartId    = VARIABLE_DATA;
  NewVariable->State      = VAR_ADDED;
  NewVariable->Reserved   = 0;
  NewVariable->Attributes = Attributes & (~EFI_VARIABLE_APPEND_WRITE);

  CopyMem (
    GetVendorGuidPtr (NewVariable, AuthFormat),
    VendorGuid,
    sizeof (EFI_GUID)
    );

  NameSize = StrSize (VariableName);
  SetNameSizeOfVariable (NewVariable, NameSize, AuthFormat);
  CopyMem (
    (UINT8 *)GetVariableNamePtr (NewVariable, AuthFormat),
    VariableName,
    NameSize
    );

  //
  // Set data size first otherwise we can't get correct data pointer in the
  // buffer of new variable.
  //
  SetDataSizeOfVariable (NewVariable, DataSize, AuthFormat);
  DataPtr = GetVariableDataPtr (NewVariable, AuthFormat);
  if ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0 &&
      OldVariable != NULL &&
      (OldVariable->State == VAR_ADDED ||
       OldVariable->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))) {
    //
    // Get old data, which might be encrypted.
    //
    OldDataSize = mVariableModuleGlobal->ScratchBufferSize
                  - ((UINTN)DataPtr - (UINTN)NewVariable);
    Status = ProtectedVariableLibGetByBuffer (
              OldVariable,
              DataPtr,
              (UINT32 *)&OldDataSize,
              AuthFormat
              );
    if (Status == EFI_UNSUPPORTED) {
      OldDataSize = DataSizeOfVariable (OldVariable, AuthFormat);
      CopyMem (DataPtr, GetVariableDataPtr (OldVariable, AuthFormat), OldDataSize);
    } else if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return 0;
    }

    DataPtr += OldDataSize;
    //
    // Update data size.
    //
    SetDataSizeOfVariable (NewVariable, DataSize + OldDataSize, AuthFormat);
  }
  CopyMem (DataPtr, Data, DataSize);

  //
  // The actual size of the variable stored in storage should include padding.
  //
  return ((UINTN)GetNextVariablePtr (NewVariable, AuthFormat) - (UINTN)NewVariable);
}

/**
  Update state of given variable as well as its cached copy.

  @param[in,out]  Variable        Pointer to the buffer of the variable.
  @param[in,out]  CacheVariable   Cache copy of the variable.
  @param[in]      NewState        New state value.
  @param[in]      Volatile        Volatile/non-volatile variable indicator.

  @retval EFI_SUCCESS     Variable state was updated successfully.
  @retval Others          Failed to update the variable state.

**/
EFI_STATUS
UpdateVariableState (
  IN  OUT VARIABLE_HEADER   *Variable,
  IN  OUT VARIABLE_HEADER   *CacheVariable,
  IN      UINT8             NewState,
  IN      BOOLEAN           Volatile
  )
{
  EFI_STATUS  Status;

  Status = UpdateVariableStore (
             &mVariableModuleGlobal->VariableGlobal,
             Volatile,
             FALSE,
             mVariableModuleGlobal->FvbInstance,
             (UINTN)&Variable->State,
             sizeof (NewState),
             &NewState
             );
  if (!EFI_ERROR (Status) && CacheVariable != NULL) {
    CacheVariable->State = NewState;
  }

  return Status;
}

/**
  Flush variable data to variable storage.

  @param[in]    VarStoreBase    Base address of variable storage.
  @param[in]    Offset          Offset to write the variable from.
  @param[out]   Offset          Offset from where next variable can be written.
  @param[in]    NewVariable     Pointer to the buffer of new variable.
  @param[in]    VariableSize    Size of new variable.
  @param[in]    Volatile        Volatile/non-volatile variable indicator.
  @param[in]    AuthFormat      Auth-variable indicator.

  @retval EFI_SUCCESS     Variable(s) were written successfully.
  @retval Others          Failed to write the variable data.

**/
EFI_STATUS
WriteVariable (
  EFI_PHYSICAL_ADDRESS              VarStoreBase,
  IN  OUT UINTN                     *Offset,
  IN  OUT VARIABLE_HEADER           **NewVariable,
  IN      UINT32                    VariableSize,
  IN      BOOLEAN                   Volatile,
  IN      BOOLEAN                   AuthFormat
  )
{
  EFI_STATUS Status;
  struct {
    UINTN   Offset;
    UINT8   *Buffer;
    UINT32  Size;
    UINT8   State;
  }                   WriteSteps[4];
  UINTN               Index;
  UINTN               Steps;
  VARIABLE_HEADER     *Variable;

  Variable = *NewVariable;
  if (Volatile) {
    //
    // For non-volatile variable, one step only :
    //
    WriteSteps[0].Offset = *Offset;
    WriteSteps[0].Buffer = (UINT8 *)Variable;;
    WriteSteps[0].Size   = VariableSize;

    Steps = 1;
  } else {
    //
    // Four steps for non-volatile variable:
    //
    // 1. Write variable header
    // 2. Set variable state to header valid
    // 3. Write variable name and data
    // 4. Set variable state to valid
    //
    Variable->State      = 0xff;
    WriteSteps[0].Offset = *Offset;
    WriteSteps[0].Buffer = (UINT8 *)Variable;;
    WriteSteps[0].Size   = (UINT32) GetVariableHeaderSize (AuthFormat);

    WriteSteps[1].State  = VAR_HEADER_VALID_ONLY;
    WriteSteps[1].Offset = *Offset + OFFSET_OF (VARIABLE_HEADER, State);
    WriteSteps[1].Buffer = &WriteSteps[1].State;
    WriteSteps[1].Size   = sizeof (Variable->State);

    WriteSteps[2].Offset = *Offset + GetVariableHeaderSize (AuthFormat);
    WriteSteps[2].Buffer = (UINT8 *)Variable + GetVariableHeaderSize (AuthFormat);
    WriteSteps[2].Size   = VariableSize - (UINT32)GetVariableHeaderSize (AuthFormat);

    WriteSteps[3].State  = VAR_ADDED;
    WriteSteps[3].Offset = *Offset + OFFSET_OF (VARIABLE_HEADER, State);
    WriteSteps[3].Buffer = &WriteSteps[3].State;
    WriteSteps[3].Size   = sizeof (Variable->State);

    Steps   = ARRAY_SIZE (WriteSteps);
  }

  for (Index = 0; Index < Steps; ++Index) {
    Status = UpdateVariableStore (
               &mVariableModuleGlobal->VariableGlobal,
               Volatile,
               TRUE,
               mVariableModuleGlobal->FvbInstance,
               WriteSteps[Index].Offset,
               WriteSteps[Index].Size,
               WriteSteps[Index].Buffer
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  Variable->State = VAR_ADDED;
  if (!Volatile) {
    CopyMem ((UINT8 *)mNvVariableCache + *Offset, Variable, VariableSize);
  }

  *NewVariable = (VARIABLE_HEADER *)((UINTN)VarStoreBase + *Offset);
  *Offset += HEADER_ALIGN (VariableSize);

  return EFI_SUCCESS;
}

/**
  Rebase the given variable pointer(s) to the equivalent one in given variable
  storage via VarStore.

  @param[in]      InVarTrackPtr     Pointer to current variable in cache.
  @param[in,out]  OutVarTrackPtr    Pointer to rebased variable against VarStore.
  @param[in]      VarStore          Start of variable storage to rebase against.
  @param[in]      VariableName      Name of variable.
  @param[in]      VendorGuid        Guid of variable.
  @param[in]      ByOffset          If TRUE, don't do variable search in VarStore.

  @retval EFI_SUCCESS           Variable(s) were deleted successfully.
  @retval EFI_INVALID_PARAMETER Invalid parameters passed.
  @retval EFI_NOT_FOUND         Given variable (VariableName & VendorGuid) was
                                not found in VarStore, if ByOffset is FALSE.

**/
EFI_STATUS
RebaseVariablePtr (
  IN      VARIABLE_POINTER_TRACK     *InVarTrackPtr,
      OUT VARIABLE_POINTER_TRACK     *OutVarTrackPtr,
  IN      VARIABLE_STORE_HEADER      *VarStore,
  IN      CHAR16                     *VariableName,
  IN      EFI_GUID                   *VendorGuid,
  IN      BOOLEAN                    ByOffset
  )
{
  EFI_STATUS               Status;
  BOOLEAN                 AuthFormat;
  VARIABLE_HEADER         *NewStart;

  if (InVarTrackPtr == NULL || OutVarTrackPtr == NULL || VarStore == NULL) {
    ASSERT (InVarTrackPtr != NULL);
    ASSERT (OutVarTrackPtr != NULL);
    ASSERT (VarStore != NULL);
    return EFI_INVALID_PARAMETER;
  }

  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  if ((InVarTrackPtr->CurrPtr == NULL)
      || (InVarTrackPtr->StartPtr == GetStartPointer (VarStore)))
  {
    CopyMem (OutVarTrackPtr, InVarTrackPtr, sizeof (VARIABLE_POINTER_TRACK));
    return EFI_SUCCESS;
  }

  NewStart = GetStartPointer (VarStore);
  if (ByOffset) {
    OutVarTrackPtr->CurrPtr = (VARIABLE_HEADER *)
                              ((UINTN)NewStart + ((UINTN)InVarTrackPtr->CurrPtr -
                                                  (UINTN)InVarTrackPtr->StartPtr));

    if (InVarTrackPtr->InDeletedTransitionPtr != NULL) {
      OutVarTrackPtr->InDeletedTransitionPtr =
        (VARIABLE_HEADER *)((UINTN)NewStart +
                            ((UINTN)InVarTrackPtr->InDeletedTransitionPtr -
                             (UINTN)InVarTrackPtr->StartPtr));
    } else {
      OutVarTrackPtr->InDeletedTransitionPtr = NULL;
    }

    OutVarTrackPtr->StartPtr = NewStart;
    OutVarTrackPtr->EndPtr   = GetEndPointer   (VarStore);
  } else {
    OutVarTrackPtr->StartPtr = NewStart;
    OutVarTrackPtr->EndPtr   = GetEndPointer   (VarStore);

    Status = FindVariableEx (VariableName, VendorGuid, FALSE, OutVarTrackPtr, AuthFormat);
    if (OutVarTrackPtr->CurrPtr == NULL || EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;;
    }
  }

  if (VarStore == mNvVariableCache
      || (UINTN)VarStore == (UINTN)mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase)
  {
    OutVarTrackPtr->Volatile = FALSE;
  }

  return EFI_SUCCESS;
}

/**
  Check if the given variable is from HOB.

  @param[in] CacheVariable      Pointer to current variable in cache.

  @retval TRUE    The variable is from HOB.
  @retval FALSE   The variable is NOT from HOB.

**/
BOOLEAN
IsHobVariable (
  IN VARIABLE_POINTER_TRACK      *CacheVariable
  )
{
  VARIABLE_STORE_HEADER *HobVarStore;

  HobVarStore = (VARIABLE_STORE_HEADER *)(UINTN)
                mVariableModuleGlobal->VariableGlobal.HobVariableBase;
  return (CacheVariable->CurrPtr != NULL &&
          HobVarStore != NULL &&
          CacheVariable->StartPtr == GetStartPointer (HobVarStore));
}

/**
  Get temporary buffer for a new variable data.

  @retval Pointer to the buffer address.

**/
VARIABLE_HEADER *
GetNewVariableBuffer (
  VOID
  )
{
  VARIABLE_HEADER         *NewVariable;
  VARIABLE_STORE_HEADER   *VarStore;

  //
  // Tricky part: Use scratch data area at the end of volatile variable store
  // as a temporary storage.
  //
  VarStore    = (VARIABLE_STORE_HEADER *)(UINTN)
                mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  NewVariable = GetEndPointer (VarStore);

  SetMem (NewVariable, mVariableModuleGlobal->ScratchBufferSize, 0xff);

  return NewVariable;
}

/**
  Delete old copies of variable completely.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Variable           Pointer to current variable on storage.
  @param[in] CacheVariable      Pointer to current variable in cache.
  @param[in] VolatileFlag       Auth-variable indicator.

  @retval EFI_SUCCESS           Variable(s) were deleted successfully.
  @retval Others                Failed to update variable state.

**/
EFI_STATUS
DeleteVariable (
  IN      CHAR16                      *VariableName,
  IN      EFI_GUID                    *VendorGuid,
  IN      VARIABLE_POINTER_TRACK      *Variable,
  IN  OUT VARIABLE_POINTER_TRACK      *CacheVariable,
  IN      BOOLEAN                     VolatileFlag
  )
{
  EFI_STATUS        Status;

  if (Variable->InDeletedTransitionPtr != NULL) {
    ASSERT (CacheVariable->InDeletedTransitionPtr != NULL);
    //
    // Both ADDED and IN_DELETED_TRANSITION variable are present,
    // set IN_DELETED_TRANSITION one to DELETED state first.
    //
    Status = UpdateVariableState (
               Variable->InDeletedTransitionPtr,
               CacheVariable->InDeletedTransitionPtr,
               CacheVariable->InDeletedTransitionPtr->State & VAR_DELETED,
               VolatileFlag
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  ASSERT (CacheVariable->CurrPtr != NULL);
  Status  = UpdateVariableState (
              Variable->CurrPtr,
              CacheVariable->CurrPtr,
              CacheVariable->CurrPtr->State & VAR_DELETED,
              VolatileFlag
              );

  if (!EFI_ERROR (Status)) {
    UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE,
                        FALSE, TRUE, FALSE, &gVariableInfo);
    if (!Variable->Volatile) {
      FlushHobVariableToFlash (VariableName, VendorGuid);
    }
  }

  return Status;
}

/**
  Check if it's the right time to update a variable.

  @param[in] Attributes         Attributes of a variable.

  @retval TRUE    It's ready for variable update.
  @retval FALSE   It's NOT ready for variable update.

**/
BOOLEAN
ReadyForUpdate (
  IN UINT32         Attributes
  )
{
  if (mVariableModuleGlobal->FvbInstance == NULL &&
      !mVariableModuleGlobal->VariableGlobal.EmuNvMode) {
    //
    // The FVB protocol is not ready, so the EFI_VARIABLE_WRITE_ARCH_PROTOCOL
    // is not installed.
    //
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      //
      // Trying to update NV variable prior to the installation of
      // EFI_VARIABLE_WRITE_ARCH_PROTOCOL
      //
      DEBUG ((DEBUG_ERROR,
              "Update NV variable before EFI_VARIABLE_WRITE_ARCH_PROTOCOL ready - %r\n",
              EFI_NOT_AVAILABLE_YET));
      return FALSE;
    } else if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
      //
      // Trying to update volatile authenticated variable prior to the
      // installation of EFI_VARIABLE_WRITE_ARCH_PROTOCOL. The authenticated
      // variable perhaps is not initialized, just return here.
      //
      DEBUG ((DEBUG_ERROR,
              "Update AUTH variable before EFI_VARIABLE_WRITE_ARCH_PROTOCOL ready - %r\n",
              EFI_NOT_AVAILABLE_YET));
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Check parameters associated with the variable to update.

  @param[in] Variable           Pointer to current variable on storage.
  @param[in] CacheVariable      Pointer to current variable in cache.
  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval EFI_SUCCESS           The variable is ok to be updated.
  @retval EFI_ALREADY_STARTED   No need to update the variable.
  @retval EFI_WRITE_PROTECTED   The variable cannot be updated.
  @retval EFI_INVALID_PARAMETER The variable attributes are not valid.
  @retval EFI_NOT_FOUND         Trying to delete non-existing variable.

**/
EFI_STATUS
ValidateVariableParameters (
  IN VARIABLE_POINTER_TRACK     *Variable,
  IN VARIABLE_POINTER_TRACK     *CacheVariable,
  IN CHAR16                     *VariableName,
  IN EFI_GUID                   *VendorGuid,
  IN VOID                       *Data,
  IN UINTN                      DataSize,
  IN UINT32                     Attributes,
  IN UINT32                     KeyIndex,
  IN UINT64                     MonotonicCount,
  IN EFI_TIME                   *TimeStamp
  )
{
  BOOLEAN     AuthFlag;

  AuthFlag = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  if ((DataSize == 0) && ((Attributes & EFI_VARIABLE_APPEND_WRITE) != 0)) {
    return EFI_ALREADY_STARTED;
  }

  if (Variable->CurrPtr != NULL) {
    //
    // Update/Delete existing variable.
    //
    if (AtRuntime ()) {
      //
      // If AtRuntime and the variable is Volatile and Runtime Access,
      // the volatile is ReadOnly, and SetVariable should be aborted and
      // return EFI_WRITE_PROTECTED.
      //
      if (Variable->Volatile) {
        return EFI_WRITE_PROTECTED;
      }
      //
      // Only variable that have NV attributes can be updated/deleted in Runtime.
      //
      if ((CacheVariable->CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // Only variable that have RT attributes can be updated/deleted in Runtime.
      //
      if ((CacheVariable->CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) {
        return EFI_INVALID_PARAMETER;
      }
    }

    //
    // Variable content unchanged and no need to update timestamp, just return.
    //
    if ((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0
        && TimeStamp == NULL
        && DataSizeOfVariable (CacheVariable->CurrPtr, AuthFlag) == DataSize
        && CompareMem (Data, GetVariableDataPtr (CacheVariable->CurrPtr, AuthFlag), DataSize) == 0)
    {
      UpdateVariableInfo (VariableName, VendorGuid, Variable->Volatile, FALSE,
                          TRUE, FALSE, FALSE, &gVariableInfo);
      return EFI_ALREADY_STARTED;
    }
  } else {
    //
    // Create a new variable.
    //

    //
    // Make sure we are trying to create a new variable. You cannot delete a new
    // variable.
    //
    if (DataSize == 0 ||
        (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS|EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0) {
      return EFI_NOT_FOUND;
    }

    //
    // Only variable have NV|RT attribute can be created in Runtime.
    //
    if (AtRuntime ()
        && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0
            || (Attributes & EFI_VARIABLE_NON_VOLATILE) == 0))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/**
  Update the variable region with Variable information. If EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is set,
  index of associated public key is needed.

  @param[in] VariableName       Name of variable.
  @param[in] VendorGuid         Guid of variable.
  @param[in] Data               Variable data.
  @param[in] DataSize           Size of data. 0 means delete.
  @param[in] Attributes         Attributes of the variable.
  @param[in] KeyIndex           Index of associated public key.
  @param[in] MonotonicCount     Value of associated monotonic count.
  @param[in, out] CacheVariable The variable information which is used to keep track of variable usage.
  @param[in] TimeStamp          Value of associated TimeStamp.

  @retval EFI_SUCCESS           The update operation is success.
  @retval EFI_OUT_OF_RESOURCES  Variable region is full, can not write other data into this region.

**/
EFI_STATUS
UpdateVariable (
  IN      CHAR16                  *VariableName,
  IN      EFI_GUID                *VendorGuid,
  IN      VOID                    *Data,
  IN      UINTN                   DataSize,
  IN      UINT32                  Attributes      OPTIONAL,
  IN      UINT32                  KeyIndex        OPTIONAL,
  IN      UINT64                  MonotonicCount  OPTIONAL,
  IN OUT  VARIABLE_POINTER_TRACK  *CacheVariable,
  IN      EFI_TIME                *TimeStamp      OPTIONAL
  )
{
  EFI_STATUS                          Status;
  VARIABLE_GLOBAL                     *VarGlobal;
  VARIABLE_HEADER                     *NewVariable;
  VARIABLE_HEADER                     *NextVariable;
  VARIABLE_HEADER                     *UpdatingVariable;
  UINTN                               VarSize;
  UINTN                               UpdateSize;
  UINTN                               Offset;
  VARIABLE_POINTER_TRACK              *Variable;
  VARIABLE_POINTER_TRACK              NvVariable;
  VARIABLE_STORE_HEADER               *VariableStoreHeader;
  VARIABLE_RUNTIME_CACHE              *VolatileCacheInstance;
  BOOLEAN                             IsCommonVariable;
  BOOLEAN                             IsCommonUserVariable;
  BOOLEAN                             DeleteFlag;
  BOOLEAN                             VolatileFlag;
  BOOLEAN                             HobVarOnlyFlag;
  EFI_PHYSICAL_ADDRESS                VarStoreBase;
  UINTN                               *LastVariableOffset;

  if (!ReadyForUpdate (Attributes)) {
    return EFI_NOT_AVAILABLE_YET;
  }

  VarGlobal = &mVariableModuleGlobal->VariableGlobal;

  if (((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0 && DataSize == 0)
      || Attributes == 0
      || (AtRuntime () && (Attributes & (EFI_VARIABLE_RUNTIME_ACCESS
                                         |EFI_VARIABLE_BOOTSERVICE_ACCESS)) == 0))
  {
    DeleteFlag = TRUE;
  } else {
    DeleteFlag = FALSE;
  }

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0
      || (CacheVariable->CurrPtr != NULL &&
          (CacheVariable->CurrPtr->Attributes & EFI_VARIABLE_NON_VOLATILE) != 0))
  {
    VolatileFlag = FALSE;
  } else {
    VolatileFlag = TRUE;
  }

  //
  // Check if CacheVariable points to the variable in variable HOB.
  // If yes, let CacheVariable points to the variable in NV variable cache.
  //
  HobVarOnlyFlag = FALSE;
  if (IsHobVariable (CacheVariable)) {
    Status = RebaseVariablePtr (CacheVariable, CacheVariable, mNvVariableCache,
                                VariableName, VendorGuid, FALSE);
    if (CacheVariable->CurrPtr == NULL || EFI_ERROR (Status)) {
      //
      // There is no matched variable in NV variable cache.
      //
      if (DeleteFlag) {
        //
        // Leave the deletion to FlushHobVariableToFlash() before return.
        //
        HobVarOnlyFlag = TRUE;
        Status = EFI_SUCCESS;
        goto Done;
      }
    }
  }

  //
  // Determine the physical position of variable store to update, due to cache
  // mechanims of variable service.
  //
  if ((CacheVariable->CurrPtr == NULL) || CacheVariable->Volatile) {
    //
    // - Add new variable (volatile or non-volatile); Or
    // - Update/delete volatile variable in place.
    //
    Variable = CacheVariable;
  } else {
    //
    // - Update/Delete existing NV variable.
    //    CacheVariable points to the variable in the memory copy of Flash area.
    //    Now let Variable points to the same variable in Flash area.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *)(UINTN)
                          VarGlobal->NonVolatileVariableBase;
    Variable = &NvVariable;
    Status = RebaseVariablePtr (CacheVariable, Variable, VariableStoreHeader,
                                VariableName, VendorGuid, TRUE);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Validate variable parameters.
  //
  Status = ValidateVariableParameters (Variable, CacheVariable, VariableName, VendorGuid,
                                       Data, DataSize, Attributes, KeyIndex, MonotonicCount,
                                       TimeStamp);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Add or update a variable. Allocate a buffer to hold it temporarily.
  //
  NewVariable = GetNewVariableBuffer ();

  //
  // Fill-up variable data first, if necessary.
  //
  IsCommonVariable = FALSE;
  IsCommonUserVariable = FALSE;
  if (DeleteFlag) {
    //
    // No need to fill up variable buffer when deleting a variable. But the
    // buffer is still needed if variable protection is employed.
    //
    VarSize = 0;
  } else {
    VarSize = SetVariableData (
                NewVariable,
                CacheVariable->CurrPtr,
                VariableName,
                VendorGuid,
                Data,
                DataSize,
                Attributes,
                KeyIndex,
                MonotonicCount,
                TimeStamp
                );

    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) {
      IsCommonVariable = TRUE;
      IsCommonUserVariable = IsUserVariable (NewVariable);
    }
  }

  //
  // We might need to do protection for non-volatile variable before flushing
  // the data to storage. A null version (meaning no protection) of following
  // APIs should simply return EFI_SUCCESS or EFI_UNSUPPORTED without any
  // changes to original data.
  //
  if (!VolatileFlag) {
    Status = ProtectedVariableLibUpdate (
                Variable->CurrPtr,
                Variable->InDeletedTransitionPtr,
                NewVariable,
                &VarSize
                );
    if (EFI_ERROR (Status) && Status != EFI_UNSUPPORTED) {
      return Status;
    }

    Status = EFI_SUCCESS;
  }

  //
  // Mark the old variable as in delete transition first. There's no such need
  // for deleting a variable, even if variable protection is employed.
  //
  if (!DeleteFlag
      && CacheVariable->CurrPtr != NULL
      && ((CacheVariable->CurrPtr->State == VAR_ADDED)
          || (CacheVariable->CurrPtr->State == (VAR_ADDED & VAR_IN_DELETED_TRANSITION))))
  {
    Status = UpdateVariableState (
               Variable->CurrPtr,
               CacheVariable->CurrPtr,
               CacheVariable->CurrPtr->State & VAR_IN_DELETED_TRANSITION,
               Variable->Volatile
               );
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  //
  // Have enough space to store the variable?
  //
  Status = CheckVariableStoreSpace (NewVariable, VarSize, VariableName,
                                    VendorGuid, Attributes, VolatileFlag);
  if (Status == EFI_OUT_OF_RESOURCES) {
    //
    // Not a chance.
    //
    goto Done;
  }

  //
  // Maybe not...
  //
  VarStoreBase = (VolatileFlag) ? VarGlobal->VolatileVariableBase
                                : VarGlobal->NonVolatileVariableBase;
  LastVariableOffset = (VolatileFlag)
                        ? &mVariableModuleGlobal->VolatileLastVariableOffset
                        : &mVariableModuleGlobal->NonVolatileLastVariableOffset;
  if (!EFI_ERROR (Status)) {
    //
    // There's enough free space at the tail of variable storage.
    //

    //
    // If non-volatile variable is protected, a separate variable (MetaDataHmacVar)
    // is always updated along with current updating variable. The buffer pointed
    // by NewVariable must have two variables. They should be written at this
    // time orderly.
    //
    NextVariable = NewVariable;
    UpdatingVariable = NULL;
    UpdateSize = 0;
    while (!EFI_ERROR (Status)
           && ((UINTN)NextVariable - (UINTN)NewVariable) < VarSize)
    {
      UpdatingVariable = NextVariable;
      NextVariable = GetNextVariablePtr (UpdatingVariable, VarGlobal->AuthFormat);
      UpdateSize = (UINTN)NextVariable - (UINTN)UpdatingVariable;

      Status = WriteVariable (VarStoreBase, LastVariableOffset, &UpdatingVariable,
                              (UINT32)UpdateSize, VolatileFlag, VarGlobal->AuthFormat);
    }

    //
    // UpdatingVariable must point to the last written variable. Restore it to
    // the first one so that we can calculate the offset in variable storage.
    //
    UpdatingVariable = (VARIABLE_HEADER *)((UINTN)UpdatingVariable + UpdateSize
                                           - VarSize);
    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
      mVariableModuleGlobal->HwErrVariableTotalSize += VarSize;
    } else {
      mVariableModuleGlobal->CommonVariableTotalSize += VarSize;
      if (IsCommonUserVariable) {
        mVariableModuleGlobal->CommonUserVariableTotalSize += VarSize;
      }
    }
    //
    // Mark the old variable(s) as deleted.
    //
    if (!EFI_ERROR (Status) && Variable->CurrPtr != NULL) {
      Status = DeleteVariable (VariableName, VendorGuid, Variable, CacheVariable,
                               VolatileFlag);
    }
  } else {
    //
    // There's not enough space at the tail of variable storage but there's
    // enough free space holes in the whole storage. Perform garbage collection
    // & reclaim operation, and integrate the new variable at the same time.
    //
    Status = Reclaim (
               VarStoreBase,
               LastVariableOffset,
               VolatileFlag,
               Variable,
               NewVariable,
               VarSize
               );
    UpdatingVariable = Variable->CurrPtr;

    if (EFI_ERROR (Status) &&
        (Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == 0) {
      //
      // Out of space.
      //
      IsCommonUserVariable  = IsUserVariable (NewVariable);
      IsCommonVariable      = TRUE;

      if (IsCommonUserVariable &&
          ((VarSize + mVariableModuleGlobal->CommonUserVariableTotalSize)
           > mVariableModuleGlobal->CommonMaxUserVariableSpace)) {
        RecordVarErrorFlag (VAR_ERROR_FLAG_USER_ERROR, VariableName, VendorGuid,
                            Attributes, VarSize);
      }
      if (IsCommonVariable &&
          ((VarSize + mVariableModuleGlobal->CommonVariableTotalSize)
           > mVariableModuleGlobal->CommonVariableSpace)) {
        RecordVarErrorFlag (VAR_ERROR_FLAG_SYSTEM_ERROR, VariableName, VendorGuid,
                            Attributes, VarSize);
      }
    }
  }

Done:
  if (!EFI_ERROR (Status)) {
    if (!VolatileFlag) {
      Offset = (UpdatingVariable != NULL) ? (UINTN)UpdatingVariable - (UINTN)VarStoreBase
                                          : 0;
      Status = ProtectedVariableLibWriteFinal (
                 NewVariable,
                 VarSize,
                 Offset
                 );
      if (EFI_ERROR (Status) && Status != EFI_UNSUPPORTED) {
        return Status;
      }

      Status = EFI_SUCCESS;
    }

    UpdateVariableInfo (VariableName, VendorGuid, VolatileFlag, FALSE, TRUE,
                        FALSE, FALSE, &gVariableInfo);
    //
    // HOB copy of the same variable is no longer needed, no matter it has
    // been deleted, updated or added from/to real variable storage.
    //
    if (HobVarOnlyFlag || !VolatileFlag) {
      FlushHobVariableToFlash (VariableName, VendorGuid);
    }

    if (!VolatileFlag) {
      VolatileCacheInstance = &(VarGlobal->VariableRuntimeCacheContext.VariableRuntimeNvCache);
    } else {
      VolatileCacheInstance = &(VarGlobal->VariableRuntimeCacheContext.VariableRuntimeVolatileCache);
    }

    if (VolatileCacheInstance->Store != NULL) {
      Status =  SynchronizeRuntimeVariableCache (
                  VolatileCacheInstance,
                  0,
                  VolatileCacheInstance->Store->Size
                  );
      ASSERT_EFI_ERROR (Status);
    }
  } else if (Status == EFI_ALREADY_STARTED) {
    //
    // Meaning nothing needs to be done. Just return success.
    //
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**

  This code finds variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize is external input.
  This function will do basic validation, before parse the data.

  @param VariableName               Name of Variable to be found.
  @param VendorGuid                 Variable vendor GUID.
  @param Attributes                 Attribute value of the variable found.
  @param DataSize                   Size of Data found. If size is less than the
                                    data, this value contains the required size.
  @param Data                       The buffer to return the contents of the variable. May be NULL
                                    with a zero DataSize in order to determine the size buffer needed.

  @return EFI_INVALID_PARAMETER     Invalid parameter.
  @return EFI_SUCCESS               Find the specified variable.
  @return EFI_NOT_FOUND             Not found.
  @return EFI_BUFFER_TO_SMALL       DataSize is too small for the result.

**/
EFI_STATUS
EFIAPI
VariableServiceGetVariable (
  IN      CHAR16    *VariableName,
  IN      EFI_GUID  *VendorGuid,
  OUT     UINT32    *Attributes OPTIONAL,
  IN OUT  UINTN     *DataSize,
  OUT     VOID      *Data OPTIONAL
  )
{
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  Variable;

  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] == 0) {
    return EFI_NOT_FOUND;
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  if ((Variable.CurrPtr == NULL) || EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Get data and its size
  //
  if (!Variable.Volatile) {
    //
    // Currently only non-volatile variable needs protection.
    //
    Status = ProtectedVariableLibGetByBuffer (
              Variable.CurrPtr,
              Data,
              (UINT32 *)DataSize,
              mVariableModuleGlobal->VariableGlobal.AuthFormat
              );
  }

  if (Variable.Volatile || Status == EFI_UNSUPPORTED) {
    Status = GetVariableData (Variable.CurrPtr, Data, (UINT32 *)DataSize, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  }

  if (!EFI_ERROR (Status)) {
    UpdateVariableInfo (VariableName, VendorGuid, Variable.Volatile, TRUE, FALSE, FALSE, FALSE, &gVariableInfo);
  }

Done:
  if (Status == EFI_SUCCESS || Status == EFI_BUFFER_TOO_SMALL) {
    if (Attributes != NULL && Variable.CurrPtr != NULL) {
      *Attributes = Variable.CurrPtr->Attributes;
    }
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**

  This code Finds the Next available variable.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param VariableNameSize           The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param VariableName               Pointer to variable name.
  @param VendorGuid                 Variable Vendor Guid.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_NOT_FOUND             The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL      The VariableNameSize is too small for the result.
                                    VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER     VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER     VariableName is NULL.
  @retval EFI_INVALID_PARAMETER     VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER     The input values of VariableName and VendorGuid are not a name and
                                    GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER     Null-terminator is not found in the first VariableNameSize bytes of
                                    the input VariableName buffer.

**/
EFI_STATUS
EFIAPI
VariableServiceGetNextVariableName (
  IN OUT  UINTN     *VariableNameSize,
  IN OUT  CHAR16    *VariableName,
  IN OUT  EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS             Status;
  UINTN                  MaxLen;
  UINTN                  VarNameSize;
  BOOLEAN                AuthFormat;
  VARIABLE_HEADER        *VariablePtr;
  VARIABLE_STORE_HEADER  *VariableStoreHeader[VariableStoreTypeMax];

  if ((VariableNameSize == NULL) || (VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

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

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // 0: Volatile, 1: HOB, 2: Non-Volatile.
  // The index and attributes mapping must be kept in this order as FindVariable
  // makes use of this mapping to implement search algorithm.
  //
  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  Status =  VariableServiceGetNextVariableInternal (
              VariableName,
              VendorGuid,
              VariableStoreHeader,
              &VariablePtr,
              AuthFormat
              );
  if (!EFI_ERROR (Status)) {
    VarNameSize = NameSizeOfVariable (VariablePtr, AuthFormat);
    ASSERT (VarNameSize != 0);
    if (VarNameSize <= *VariableNameSize) {
      CopyMem (
        VariableName,
        GetVariableNamePtr (VariablePtr, AuthFormat),
        VarNameSize
        );
      CopyMem (
        VendorGuid,
        GetVendorGuidPtr (VariablePtr, AuthFormat),
        sizeof (EFI_GUID)
        );
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *VariableNameSize = VarNameSize;
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**

  This code sets variable in storage blocks (Volatile or Non-Volatile).

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode, and datasize and data are external input.
  This function will do basic validation, before parse the data.
  This function will parse the authentication carefully to avoid security issues, like
  buffer overflow, integer overflow.
  This function will check attribute carefully to avoid authentication bypass.

  @param VariableName                     Name of Variable to be found.
  @param VendorGuid                       Variable vendor GUID.
  @param Attributes                       Attribute value of the variable found
  @param DataSize                         Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param Data                             Data pointer.

  @return EFI_INVALID_PARAMETER           Invalid parameter.
  @return EFI_SUCCESS                     Set successfully.
  @return EFI_OUT_OF_RESOURCES            Resource not enough to set variable.
  @return EFI_NOT_FOUND                   Not found.
  @return EFI_WRITE_PROTECTED             Variable is read-only.

**/
EFI_STATUS
EFIAPI
VariableServiceSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  VARIABLE_POINTER_TRACK  Variable;
  EFI_STATUS              Status;
  VARIABLE_HEADER         *NextVariable;
  EFI_PHYSICAL_ADDRESS    Point;
  UINTN                   PayloadSize;
  BOOLEAN                 AuthFormat;

  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  //
  // Check input parameters.
  //
  if ((VariableName == NULL) || (VariableName[0] == 0) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DataSize != 0) && (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for reserverd bit in variable attribute.
  // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated but we still allow
  // the delete operation of common authenticated variable at user physical presence.
  // So leave EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS attribute check to AuthVariableLib
  //
  if ((Attributes & (~(EFI_VARIABLE_ATTRIBUTES_MASK | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS))) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if the combination of attribute bits is valid.
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      return EFI_UNSUPPORTED;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  } else if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE) {
    //
    // Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
    if (!mVariableModuleGlobal->VariableGlobal.AuthSupport) {
      //
      // Not support authenticated variable write.
      //
      return EFI_INVALID_PARAMETER;
    }
  } else if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
    if (PcdGet32 (PcdHwErrStorageSize) == 0) {
      //
      // Not support harware error record variable variable.
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS and EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute
  // cannot be set both.
  //
  if (  ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
     && ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)){
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
    //
    //  If DataSize == AUTHINFO_SIZE and then PayloadSize is 0.
    //  Maybe it's the delete operation of common authenticated variable at user physical presence.
    //
    if (DataSize != AUTHINFO_SIZE) {
      return EFI_UNSUPPORTED;
    }

    PayloadSize = DataSize - AUTHINFO_SIZE;
  } else if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
    //
    // Sanity check for EFI_VARIABLE_AUTHENTICATION_2 descriptor.
    //
    if ((DataSize < OFFSET_OF_AUTHINFO2_CERT_DATA) ||
        (((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->AuthInfo.Hdr.dwLength > DataSize - (OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo))) ||
        (((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->AuthInfo.Hdr.dwLength < OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData))) {
      return EFI_SECURITY_VIOLATION;
    }

    //
    // The VariableSpeculationBarrier() call here is to ensure the above sanity
    // check for the EFI_VARIABLE_AUTHENTICATION_2 descriptor has been completed
    // before the execution of subsequent codes.
    //
    VariableSpeculationBarrier ();
    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);
  } else {
    PayloadSize = DataSize;
  }

  if ((UINTN)(~0) - PayloadSize < StrSize (VariableName)) {
    //
    // Prevent whole variable size overflow
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  //  The size of the VariableName, including the Unicode Null in bytes plus
  //  the DataSize is limited to maximum size of PcdGet32 (PcdMaxHardwareErrorVariableSize)
  //  bytes for HwErrRec#### variable.
  //
  if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    if (StrSize (VariableName) + PayloadSize >
        PcdGet32 (PcdMaxHardwareErrorVariableSize) - GetVariableHeaderSize (AuthFormat)) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    //  The size of the VariableName, including the Unicode Null in bytes plus
    //  the DataSize is limited to maximum size of Max(Auth|Volatile)VariableSize bytes.
    //
    if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
      if (StrSize (VariableName) + PayloadSize >
          mVariableModuleGlobal->MaxAuthVariableSize -
          GetVariableHeaderSize (AuthFormat)) {
        DEBUG ((DEBUG_ERROR,
          "%a: Failed to set variable '%s' with Guid %g\n",
          __FUNCTION__, VariableName, VendorGuid));
        DEBUG ((DEBUG_ERROR,
          "NameSize(0x%x) + PayloadSize(0x%x) > "
          "MaxAuthVariableSize(0x%x) - HeaderSize(0x%x)\n",
          StrSize (VariableName), PayloadSize,
          mVariableModuleGlobal->MaxAuthVariableSize,
          GetVariableHeaderSize (AuthFormat)
          ));
        return EFI_INVALID_PARAMETER;
      }
    } else if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      if (StrSize (VariableName) + PayloadSize >
          mVariableModuleGlobal->MaxVariableSize - GetVariableHeaderSize (AuthFormat)) {
        DEBUG ((DEBUG_ERROR,
          "%a: Failed to set variable '%s' with Guid %g\n",
          __FUNCTION__, VariableName, VendorGuid));
        DEBUG ((DEBUG_ERROR,
          "NameSize(0x%x) + PayloadSize(0x%x) > "
          "MaxVariableSize(0x%x) - HeaderSize(0x%x)\n",
          StrSize (VariableName), PayloadSize,
          mVariableModuleGlobal->MaxVariableSize,
          GetVariableHeaderSize (AuthFormat)
          ));
        return EFI_INVALID_PARAMETER;
      }
    } else {
      if (StrSize (VariableName) + PayloadSize >
          mVariableModuleGlobal->MaxVolatileVariableSize - GetVariableHeaderSize (AuthFormat)) {
        DEBUG ((DEBUG_ERROR,
          "%a: Failed to set variable '%s' with Guid %g\n",
          __FUNCTION__, VariableName, VendorGuid));
        DEBUG ((DEBUG_ERROR,
          "NameSize(0x%x) + PayloadSize(0x%x) > "
          "MaxVolatileVariableSize(0x%x) - HeaderSize(0x%x)\n",
          StrSize (VariableName), PayloadSize,
          mVariableModuleGlobal->MaxVolatileVariableSize,
          GetVariableHeaderSize (AuthFormat)
          ));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  //
  // Special Handling for MOR Lock variable.
  //
  Status = SetVariableCheckHandlerMor (VariableName, VendorGuid, Attributes, PayloadSize, (VOID *)((UINTN)Data + DataSize - PayloadSize));
  if (Status == EFI_ALREADY_STARTED) {
    //
    // EFI_ALREADY_STARTED means the SetVariable() action is handled inside of SetVariableCheckHandlerMor().
    // Variable driver can just return SUCCESS.
    //
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VarCheckLibSetVariableCheck (VariableName, VendorGuid, Attributes, PayloadSize, (VOID *)((UINTN)Data + DataSize - PayloadSize), mRequestSource);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Consider reentrant in MCA/INIT/NMI. It needs be reupdated.
  //
  if (1 < InterlockedIncrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState)) {
    Point = mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase;
    //
    // Parse non-volatile variable data and get last variable offset.
    //
    NextVariable  = GetStartPointer ((VARIABLE_STORE_HEADER *) (UINTN) Point);
    while (IsValidVariableHeader (NextVariable, GetEndPointer ((VARIABLE_STORE_HEADER *) (UINTN) Point), AuthFormat)) {
      NextVariable = GetNextVariablePtr (NextVariable, AuthFormat);
    }

    mVariableModuleGlobal->NonVolatileLastVariableOffset = (UINTN)NextVariable - (UINTN)Point;
  }

  //
  // Check whether the input variable is already existed.
  //
  Status = FindVariable (VariableName, VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, TRUE);
  if (!EFI_ERROR (Status)) {
    if (((Variable.CurrPtr->Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0) && AtRuntime ()) {
      Status = EFI_WRITE_PROTECTED;
      goto Done;
    }

    if ((Attributes != 0) && ((Attributes & (~EFI_VARIABLE_APPEND_WRITE)) != Variable.CurrPtr->Attributes)) {
      //
      // If a preexisting variable is rewritten with different attributes, SetVariable() shall not
      // modify the variable and shall return EFI_INVALID_PARAMETER. Two exceptions to this rule:
      // 1. No access attributes specified
      // 2. The only attribute differing is EFI_VARIABLE_APPEND_WRITE
      //
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((DEBUG_INFO, "[Variable]: Rewritten a preexisting variable(0x%08x) with different attributes(0x%08x) - %g:%s\n", Variable.CurrPtr->Attributes, Attributes, VendorGuid, VariableName));
      goto Done;
    }
  }

  if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
    //
    // Hook the operation of setting PlatformLangCodes/PlatformLang and LangCodes/Lang.
    //
    Status = AutoUpdateLangVariable (VariableName, Data, DataSize);
    if (EFI_ERROR (Status)) {
      //
      // The auto update operation failed, directly return to avoid inconsistency between PlatformLang and Lang.
      //
      goto Done;
    }
  }

  if (mVariableModuleGlobal->VariableGlobal.AuthSupport) {
    Status = AuthVariableLibProcessVariable (VariableName, VendorGuid, Data, DataSize, Attributes);
  } else {
    Status = UpdateVariable (VariableName, VendorGuid, Data, DataSize, Attributes, 0, 0, &Variable, NULL);
  }

Done:
  InterlockedDecrement (&mVariableModuleGlobal->VariableGlobal.ReentrantState);
  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  if (!AtRuntime ()) {
    if (!EFI_ERROR (Status)) {
      SecureBootHook (
        VariableName,
        VendorGuid
        );
    }
  }

  return Status;
}

/**

  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_SUCCESS                   Query successfully.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfoInternal (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  VARIABLE_HEADER         *Variable;
  VARIABLE_HEADER         *NextVariable;
  UINT64                  VariableSize;
  VARIABLE_STORE_HEADER   *VariableStoreHeader;
  UINT64                  CommonVariableTotalSize;
  UINT64                  HwErrVariableTotalSize;
  EFI_STATUS              Status;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;

  CommonVariableTotalSize = 0;
  HwErrVariableTotalSize  = 0;

  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    //
    // Query is Volatile related.
    //
    VariableStoreHeader = (VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase);
  } else {
    //
    // Query is Non-Volatile related.
    //
    VariableStoreHeader = mNvVariableCache;
  }

  //
  // Now let's fill *MaximumVariableStorageSize *RemainingVariableStorageSize
  // with the storage size (excluding the storage header size).
  //
  *MaximumVariableStorageSize = VariableStoreHeader->Size - sizeof (VARIABLE_STORE_HEADER);

  //
  // Harware error record variable needs larger size.
  //
  if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) {
    *MaximumVariableStorageSize = PcdGet32 (PcdHwErrStorageSize);
    *MaximumVariableSize        =  PcdGet32 (PcdMaxHardwareErrorVariableSize) -
                                  GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      if (AtRuntime ()) {
        *MaximumVariableStorageSize = mVariableModuleGlobal->CommonRuntimeVariableSpace;
      } else {
        *MaximumVariableStorageSize = mVariableModuleGlobal->CommonVariableSpace;
      }
    }

    //
    // Let *MaximumVariableSize be Max(Auth|Volatile)VariableSize with the exception of the variable header size.
    //
    if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
      *MaximumVariableSize =  mVariableModuleGlobal->MaxAuthVariableSize -
                             GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    } else if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
      *MaximumVariableSize =  mVariableModuleGlobal->MaxVariableSize -
                             GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    } else {
      *MaximumVariableSize =   mVariableModuleGlobal->MaxVolatileVariableSize -
                             GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    }
  }

  //
  // Point to the starting address of the variables.
  //
  Variable = GetStartPointer (VariableStoreHeader);

  //
  // Now walk through the related variable store.
  //
  while (IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader),
                                mVariableModuleGlobal->VariableGlobal.AuthFormat)) {
    NextVariable = GetNextVariablePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat);
    VariableSize = (UINT64)(UINTN)NextVariable - (UINT64)(UINTN)Variable;

    if (AtRuntime ()) {
      //
      // We don't take the state of the variables in mind
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
      // Only care about Variables with State VAR_ADDED, because
      // the space not marked as VAR_ADDED is reclaimable now.
      //
      if (Variable->State == VAR_ADDED) {
        if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
          HwErrVariableTotalSize += VariableSize;
        } else {
          CommonVariableTotalSize += VariableSize;
        }
      } else if (Variable->State == (VAR_IN_DELETED_TRANSITION & VAR_ADDED)) {
        //
        // If it is a IN_DELETED_TRANSITION variable,
        // and there is not also a same ADDED one at the same time,
        // this IN_DELETED_TRANSITION variable is valid.
        //
        VariablePtrTrack.StartPtr = GetStartPointer (VariableStoreHeader);
        VariablePtrTrack.EndPtr   = GetEndPointer (VariableStoreHeader);
        Status                    = FindVariableEx (
                                      GetVariableNamePtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
                                      GetVendorGuidPtr (Variable, mVariableModuleGlobal->VariableGlobal.AuthFormat),
                                      FALSE,
                                      &VariablePtrTrack,
                                      mVariableModuleGlobal->VariableGlobal.AuthFormat
                                      );
        if (!EFI_ERROR (Status) && (VariablePtrTrack.CurrPtr->State != VAR_ADDED)) {
          if ((Variable->Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
            HwErrVariableTotalSize += VariableSize;
          } else {
            CommonVariableTotalSize += VariableSize;
          }
        }
      }
    }

    //
    // Go to the next one.
    //
    Variable = NextVariable;
  }

  if ((Attributes  & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    *RemainingVariableStorageSize = *MaximumVariableStorageSize - HwErrVariableTotalSize;
  } else {
    if (*MaximumVariableStorageSize < CommonVariableTotalSize) {
      *RemainingVariableStorageSize = 0;
    } else {
      *RemainingVariableStorageSize = *MaximumVariableStorageSize - CommonVariableTotalSize;
    }
  }

  if (*RemainingVariableStorageSize < GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat)) {
    *MaximumVariableSize = 0;
  } else if ((*RemainingVariableStorageSize - GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat)) <
              *MaximumVariableSize
              ) {
    *MaximumVariableSize = *RemainingVariableStorageSize -
                           GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
  }

  return EFI_SUCCESS;
}

/**

  This code returns information about the EFI variables.

  Caution: This function may receive untrusted input.
  This function may be invoked in SMM mode. This function will do basic validation, before parse the data.

  @param Attributes                     Attributes bitmask to specify the type of variables
                                        on which to return information.
  @param MaximumVariableStorageSize     Pointer to the maximum size of the storage space available
                                        for the EFI variables associated with the attributes specified.
  @param RemainingVariableStorageSize   Pointer to the remaining size of the storage space available
                                        for EFI variables associated with the attributes specified.
  @param MaximumVariableSize            Pointer to the maximum size of an individual EFI variables
                                        associated with the attributes specified.

  @return EFI_INVALID_PARAMETER         An invalid combination of attribute bits was supplied.
  @return EFI_SUCCESS                   Query successfully.
  @return EFI_UNSUPPORTED               The attribute is not supported on this platform.

**/
EFI_STATUS
EFIAPI
VariableServiceQueryVariableInfo (
  IN  UINT32  Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  EFI_STATUS  Status;

  if ((MaximumVariableStorageSize == NULL) || (RemainingVariableStorageSize == NULL) || (MaximumVariableSize == NULL) || (Attributes == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    //  Deprecated attribute, make this check as highest priority.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == 0) {
    //
    // Make sure the Attributes combination is supported by the platform.
    //
    return EFI_UNSUPPORTED;
  } else if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE) {
    //
    // Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    return EFI_INVALID_PARAMETER;
  } else if (AtRuntime () && ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == 0)) {
    //
    // Make sure RT Attribute is set if we are in Runtime phase.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
    //
    // Make sure Hw Attribute is set with NV.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & VARIABLE_ATTRIBUTE_AT_AW) != 0) {
    if (!mVariableModuleGlobal->VariableGlobal.AuthSupport) {
      //
      // Not support authenticated variable write.
      //
      return EFI_UNSUPPORTED;
    }
  } else if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) != 0) {
    if (PcdGet32 (PcdHwErrStorageSize) == 0) {
      //
      // Not support harware error record variable variable.
      //
      return EFI_UNSUPPORTED;
    }
  }

  AcquireLockOnlyAtBootTime(&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  Status = VariableServiceQueryVariableInfoInternal (
             Attributes,
             MaximumVariableStorageSize,
             RemainingVariableStorageSize,
             MaximumVariableSize
             );

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
  return Status;
}

/**
  This function reclaims variable storage if free size is below the threshold.

  Caution: This function may be invoked at SMM mode.
  Care must be taken to make sure not security issue.

**/
VOID
ReclaimForOS (
  VOID
  )
{
  EFI_STATUS      Status;
  UINTN           RemainingCommonRuntimeVariableSpace;
  UINTN           RemainingHwErrVariableSpace;
  STATIC BOOLEAN  Reclaimed;

  //
  // This function will be called only once at EndOfDxe or ReadyToBoot event.
  //
  if (Reclaimed) {
    return;
  }

  Reclaimed = TRUE;

  Status = EFI_SUCCESS;

  if (mVariableModuleGlobal->CommonRuntimeVariableSpace < mVariableModuleGlobal->CommonVariableTotalSize) {
    RemainingCommonRuntimeVariableSpace = 0;
  } else {
    RemainingCommonRuntimeVariableSpace = mVariableModuleGlobal->CommonRuntimeVariableSpace - mVariableModuleGlobal->CommonVariableTotalSize;
  }

  RemainingHwErrVariableSpace = PcdGet32 (PcdHwErrStorageSize) - mVariableModuleGlobal->HwErrVariableTotalSize;

  //
  // Check if the free area is below a threshold.
  //
  if (((RemainingCommonRuntimeVariableSpace < mVariableModuleGlobal->MaxVariableSize) ||
       (RemainingCommonRuntimeVariableSpace < mVariableModuleGlobal->MaxAuthVariableSize)) ||
      ((PcdGet32 (PcdHwErrStorageSize) != 0) &&
       (RemainingHwErrVariableSpace < PcdGet32 (PcdMaxHardwareErrorVariableSize)))) {
    Status = Reclaim (
               mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
               &mVariableModuleGlobal->NonVolatileLastVariableOffset,
               FALSE,
               NULL,
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  Get maximum variable size, covering both non-volatile and volatile variables.

  @return Maximum variable size.

**/
UINTN
GetMaxVariableSize (
  VOID
  )
{
  UINTN  MaxVariableSize;

  MaxVariableSize = GetNonVolatileMaxVariableSize ();
  //
  // The condition below fails implicitly if PcdMaxVolatileVariableSize equals
  // the default zero value.
  //
  if (MaxVariableSize < PcdGet32 (PcdMaxVolatileVariableSize)) {
    MaxVariableSize = PcdGet32 (PcdMaxVolatileVariableSize);
  }

  return MaxVariableSize;
}

/**
  Flush the HOB variable to flash.

  @param[in] VariableName       Name of variable has been updated or deleted.
  @param[in] VendorGuid         Guid of variable has been updated or deleted.

**/
VOID
FlushHobVariableToFlash (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS              Status;
  VARIABLE_STORE_HEADER   *VariableStoreHeader;
  VARIABLE_HEADER         *Variable;
  VOID                    *VariableData;
  VARIABLE_POINTER_TRACK  VariablePtrTrack;
  BOOLEAN                 ErrorFlag;
  BOOLEAN                 AuthFormat;

  ErrorFlag  = FALSE;
  AuthFormat = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  //
  // Flush the HOB variable to flash.
  //
  if (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
    VariableStoreHeader = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
    //
    // Set HobVariableBase to 0, it can avoid SetVariable to call back.
    //
    mVariableModuleGlobal->VariableGlobal.HobVariableBase = 0;
    for ( Variable = GetStartPointer (VariableStoreHeader)
        ; IsValidVariableHeader (Variable, GetEndPointer (VariableStoreHeader), AuthFormat)
        ; Variable = GetNextVariablePtr (Variable, AuthFormat)
        ) {
      if (Variable->State != VAR_ADDED) {
        //
        // The HOB variable has been set to DELETED state in local.
        //
        continue;
      }

      ASSERT ((Variable->Attributes & EFI_VARIABLE_NON_VOLATILE) != 0);
      if ((VendorGuid == NULL) || (VariableName == NULL) ||
          !CompareGuid (VendorGuid, GetVendorGuidPtr (Variable, AuthFormat)) ||
          (StrCmp (VariableName, GetVariableNamePtr (Variable, AuthFormat)) != 0)) {
        VariableData = GetVariableDataPtr (Variable, AuthFormat);
        FindVariable (
          GetVariableNamePtr (Variable, AuthFormat),
          GetVendorGuidPtr (Variable, AuthFormat),
          &VariablePtrTrack,
          &mVariableModuleGlobal->VariableGlobal,
          FALSE
          );
        Status = UpdateVariable (
                   GetVariableNamePtr (Variable, AuthFormat),
                   GetVendorGuidPtr (Variable, AuthFormat),
                   VariableData,
                   DataSizeOfVariable (Variable, AuthFormat),
                   Variable->Attributes,
                   0,
                   0,
                   &VariablePtrTrack,
                   NULL
                   );
        DEBUG ((
          DEBUG_INFO,
          "Variable driver flush the HOB variable to flash: %g %s %r\n",
          GetVendorGuidPtr (Variable, AuthFormat),
          GetVariableNamePtr (Variable, AuthFormat),
          Status
          ));
      } else {
        //
        // The updated or deleted variable is matched with this HOB variable.
        // Don't break here because we will try to set other HOB variables
        // since this variable could be set successfully.
        //
        Status = EFI_SUCCESS;
      }

      if (!EFI_ERROR (Status)) {
        //
        // If set variable successful, or the updated or deleted variable is matched with the HOB variable,
        // set the HOB variable to DELETED state in local.
        //
        DEBUG ((
          DEBUG_INFO,
          "Variable driver set the HOB variable to DELETED state in local: %g %s\n",
          GetVendorGuidPtr (Variable, AuthFormat),
          GetVariableNamePtr (Variable, AuthFormat)
          ));
        Variable->State &= VAR_DELETED;
      } else {
        ErrorFlag = TRUE;
      }
    }

    if (mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeHobCache.Store != NULL) {
      Status =  SynchronizeRuntimeVariableCache (
                  &mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeHobCache,
                  0,
                  mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.VariableRuntimeHobCache.Store->Size
                  );
      ASSERT_EFI_ERROR (Status);
    }

    if (ErrorFlag) {
      //
      // We still have HOB variable(s) not flushed in flash.
      //
      mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VariableStoreHeader;
    } else {
      //
      // All HOB variables have been flushed in flash.
      //
      DEBUG ((DEBUG_INFO, "Variable driver: all HOB variables have been flushed in flash.\n"));
      if (mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.HobFlushComplete != NULL) {
        *(mVariableModuleGlobal->VariableGlobal.VariableRuntimeCacheContext.HobFlushComplete) = TRUE;
      }

      if (!AtRuntime ()) {
        FreePool ((VOID *)VariableStoreHeader);
      }
    }
  }
}

/**
  Initializes variable write service.

  @retval EFI_SUCCESS          Function successfully executed.
  @retval Others               Fail to initialize the variable service.

**/
EFI_STATUS
VariableWriteServiceInitialize (
  VOID
  )
{
  EFI_STATUS               Status;
  UINTN                    Index;
  UINT8                    Data;
  VARIABLE_ENTRY_PROPERTY  *VariableEntry;

  AcquireLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Check if the free area is really free.
  //
  for (Index = mVariableModuleGlobal->NonVolatileLastVariableOffset; Index < mNvVariableCache->Size; Index++) {
    Data = ((UINT8 *)mNvVariableCache)[Index];
    if (Data != 0xff) {
      //
      // There must be something wrong in variable store, do reclaim operation.
      //
      Status = Reclaim (
                 mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
                 &mVariableModuleGlobal->NonVolatileLastVariableOffset,
                 FALSE,
                 NULL,
                 NULL,
                 0
                 );
      if (EFI_ERROR (Status)) {
        ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);
        return Status;
      }

      break;
    }
  }

  FlushHobVariableToFlash (NULL, NULL);

  Status = EFI_SUCCESS;
  ZeroMem (&mAuthContextOut, sizeof (mAuthContextOut));
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    //
    // Authenticated variable initialize.
    //
    mAuthContextIn.StructSize          = sizeof (AUTH_VAR_LIB_CONTEXT_IN);
    mAuthContextIn.MaxAuthVariableSize =  mVariableModuleGlobal->MaxAuthVariableSize -
                                         GetVariableHeaderSize (mVariableModuleGlobal->VariableGlobal.AuthFormat);
    Status = AuthVariableLibInitialize (&mAuthContextIn, &mAuthContextOut);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Variable driver will work with auth variable support!\n"));
      mVariableModuleGlobal->VariableGlobal.AuthSupport = TRUE;
      if (mAuthContextOut.AuthVarEntry != NULL) {
        for (Index = 0; Index < mAuthContextOut.AuthVarEntryCount; Index++) {
          VariableEntry = &mAuthContextOut.AuthVarEntry[Index];
          Status        = VarCheckLibVariablePropertySet (
                            VariableEntry->Name,
                            VariableEntry->Guid,
                            &VariableEntry->VariableProperty
                            );
          ASSERT_EFI_ERROR (Status);
        }
      }
    } else if (Status == EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_INFO, "NOTICE - AuthVariableLibInitialize() returns %r!\n", Status));
      DEBUG ((DEBUG_INFO, "Variable driver will continue to work without auth variable support!\n"));
      mVariableModuleGlobal->VariableGlobal.AuthSupport = FALSE;
      Status = EFI_SUCCESS;
    }
  }

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < ARRAY_SIZE (mVariableEntryProperty); Index++) {
      VariableEntry = &mVariableEntryProperty[Index];
      Status = VarCheckLibVariablePropertySet (VariableEntry->Name, VariableEntry->Guid, &VariableEntry->VariableProperty);
      ASSERT_EFI_ERROR (Status);
    }
  }

  ReleaseLockOnlyAtBootTime (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock);

  //
  // Initialize MOR Lock variable.
  //
  MorLockInit ();

  return Status;
}

/**
  Convert normal variable storage to the allocated auth variable storage.

  @param[in]  NormalVarStorage  Pointer to the normal variable storage header

  @retval the allocated auth variable storage
**/
VOID *
ConvertNormalVarStorageToAuthVarStorage (
  VARIABLE_STORE_HEADER  *NormalVarStorage
  )
{
  VARIABLE_HEADER                *StartPtr;
  UINT8                          *NextPtr;
  VARIABLE_HEADER                *EndPtr;
  UINTN                          AuthVarStorageSize;
  AUTHENTICATED_VARIABLE_HEADER  *AuthStartPtr;
  VARIABLE_STORE_HEADER          *AuthVarStorage;

  AuthVarStorageSize = sizeof (VARIABLE_STORE_HEADER);
  //
  // Set AuthFormat as FALSE for normal variable storage
  //
  mVariableModuleGlobal->VariableGlobal.AuthFormat = FALSE;

  //
  // Calculate Auth Variable Storage Size
  //
  StartPtr = GetStartPointer (NormalVarStorage);
  EndPtr   = GetEndPointer (NormalVarStorage);
  while (StartPtr < EndPtr) {
    if (StartPtr->State == VAR_ADDED) {
      AuthVarStorageSize = HEADER_ALIGN (AuthVarStorageSize);
      AuthVarStorageSize += sizeof (AUTHENTICATED_VARIABLE_HEADER);
      AuthVarStorageSize += StartPtr->NameSize + GET_PAD_SIZE (StartPtr->NameSize);
      AuthVarStorageSize += StartPtr->DataSize + GET_PAD_SIZE (StartPtr->DataSize);
    }

    StartPtr = GetNextVariablePtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  }

  //
  // Allocate Runtime memory for Auth Variable Storage
  //
  AuthVarStorage = AllocateRuntimeZeroPool (AuthVarStorageSize);
  ASSERT (AuthVarStorage != NULL);
  if (AuthVarStorage == NULL) {
    return NULL;
  }

  //
  // Copy Variable from Normal storage to Auth storage
  //
  StartPtr     = GetStartPointer (NormalVarStorage);
  EndPtr       = GetEndPointer (NormalVarStorage);
  AuthStartPtr = (AUTHENTICATED_VARIABLE_HEADER *)GetStartPointer (AuthVarStorage);
  while (StartPtr < EndPtr) {
    if (StartPtr->State == VAR_ADDED) {
      AuthStartPtr = (AUTHENTICATED_VARIABLE_HEADER *)HEADER_ALIGN (AuthStartPtr);
      //
      // Copy Variable Header
      //
      AuthStartPtr->StartId    = StartPtr->StartId;
      AuthStartPtr->State      = StartPtr->State;
      AuthStartPtr->Attributes = StartPtr->Attributes;
      AuthStartPtr->NameSize   = StartPtr->NameSize;
      AuthStartPtr->DataSize   = StartPtr->DataSize;
      CopyGuid (&AuthStartPtr->VendorGuid, &StartPtr->VendorGuid);
      //
      // Copy Variable Name
      //
      NextPtr = (UINT8 *)(AuthStartPtr + 1);
      CopyMem (
        NextPtr,
        GetVariableNamePtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat),
        AuthStartPtr->NameSize
        );
      //
      // Copy Variable Data
      //
      NextPtr = NextPtr + AuthStartPtr->NameSize + GET_PAD_SIZE (AuthStartPtr->NameSize);
      CopyMem (NextPtr, GetVariableDataPtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat), AuthStartPtr->DataSize);
      //
      // Go to next variable
      //
      AuthStartPtr = (AUTHENTICATED_VARIABLE_HEADER *)(NextPtr + AuthStartPtr->DataSize + GET_PAD_SIZE (AuthStartPtr->DataSize));
    }

    StartPtr = GetNextVariablePtr (StartPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  }

  //
  // Update Auth Storage Header
  //
  AuthVarStorage->Format = NormalVarStorage->Format;
  AuthVarStorage->State  = NormalVarStorage->State;
  AuthVarStorage->Size   = (UINT32)((UINTN)AuthStartPtr - (UINTN)AuthVarStorage);
  CopyGuid (&AuthVarStorage->Signature, &gEfiAuthenticatedVariableGuid);
  ASSERT (AuthVarStorage->Size <= AuthVarStorageSize);

  //
  // Restore AuthFormat
  //
  mVariableModuleGlobal->VariableGlobal.AuthFormat = TRUE;
  return AuthVarStorage;
}

/**
  Get HOB variable store.

  @param[in] VariableGuid       NV variable store signature.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
GetHobVariableStore (
  IN EFI_GUID  *VariableGuid
  )
{
  VARIABLE_STORE_HEADER  *VariableStoreHeader;
  UINT64                 VariableStoreLength;
  EFI_HOB_GUID_TYPE      *GuidHob;
  BOOLEAN                NeedConvertNormalToAuth;

  //
  // Make sure there is no more than one Variable HOB.
  //
  DEBUG_CODE_BEGIN ();
  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    if ((GetNextGuidHob (&gEfiAuthenticatedVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
      DEBUG ((DEBUG_ERROR, "ERROR: Found two Auth Variable HOBs\n"));
      ASSERT (FALSE);
    } else if (GetFirstGuidHob (&gEfiVariableGuid) != NULL) {
      DEBUG ((DEBUG_ERROR, "ERROR: Found one Auth + one Normal Variable HOBs\n"));
      ASSERT (FALSE);
    }
  } else {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      if ((GetNextGuidHob (&gEfiVariableGuid, GET_NEXT_HOB (GuidHob)) != NULL)) {
        DEBUG ((DEBUG_ERROR, "ERROR: Found two Normal Variable HOBs\n"));
        ASSERT (FALSE);
      }
    }
  }

  DEBUG_CODE_END ();

  //
  // Combinations supported:
  // 1. Normal NV variable store +
  //    Normal HOB variable store
  // 2. Auth NV variable store +
  //    Auth HOB variable store
  // 3. Auth NV variable store +
  //    Normal HOB variable store (code will convert it to Auth Format)
  //
  NeedConvertNormalToAuth = FALSE;
  GuidHob                 = GetFirstGuidHob (VariableGuid);
  if ((GuidHob == NULL) && (VariableGuid == &gEfiAuthenticatedVariableGuid)) {
    //
    // Try getting it from normal variable HOB
    //
    GuidHob                 = GetFirstGuidHob (&gEfiVariableGuid);
    NeedConvertNormalToAuth = TRUE;
  }

  if (GuidHob != NULL) {
    VariableStoreHeader = GET_GUID_HOB_DATA (GuidHob);
    VariableStoreLength = GuidHob->Header.HobLength - sizeof (EFI_HOB_GUID_TYPE);
    if (GetVariableStoreStatus (VariableStoreHeader) == EfiValid) {
      if (!NeedConvertNormalToAuth) {
        mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateRuntimeCopyPool ((UINTN)VariableStoreLength, (VOID *)VariableStoreHeader);
      } else {
        mVariableModuleGlobal->VariableGlobal.HobVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)ConvertNormalVarStorageToAuthVarStorage ((VOID *)VariableStoreHeader);
      }

      if (mVariableModuleGlobal->VariableGlobal.HobVariableBase == 0) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      DEBUG ((DEBUG_ERROR, "HOB Variable Store header is corrupted!\n"));
    }
  }

  return EFI_SUCCESS;
}

/**
  Initializes variable store area for non-volatile and volatile variable.

  @retval EFI_SUCCESS           Function successfully executed.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate enough memory resource.

**/
EFI_STATUS
VariableCommonInitialize (
  VOID
  )
{
  EFI_STATUS             Status;
  VARIABLE_STORE_HEADER  *VolatileVariableStore;
  UINTN                  ScratchSize;
  EFI_GUID               *VariableGuid;

  //
  // Allocate runtime memory for variable driver global structure.
  //
  mVariableModuleGlobal = AllocateRuntimeZeroPool (sizeof (VARIABLE_MODULE_GLOBAL));
  if (mVariableModuleGlobal == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeLock (&mVariableModuleGlobal->VariableGlobal.VariableServicesLock, TPL_NOTIFY);

  //
  // Init non-volatile variable store.
  //
  Status = InitNonVolatileVariableStore ();
  if (EFI_ERROR (Status)) {
    FreePool (mVariableModuleGlobal);
    return Status;
  }

  //
  // mVariableModuleGlobal->VariableGlobal.AuthFormat
  // has been initialized in InitNonVolatileVariableStore().
  //
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    DEBUG ((DEBUG_INFO, "Variable driver will work with auth variable format!\n"));
    //
    // Set AuthSupport to FALSE first, VariableWriteServiceInitialize() will initialize it.
    //
    mVariableModuleGlobal->VariableGlobal.AuthSupport = FALSE;
    VariableGuid = &gEfiAuthenticatedVariableGuid;
  } else {
    DEBUG ((DEBUG_INFO, "Variable driver will work without auth variable support!\n"));
    mVariableModuleGlobal->VariableGlobal.AuthSupport = FALSE;
    VariableGuid                                      = &gEfiVariableGuid;
  }

  //
  // Get HOB variable store.
  //
  Status = GetHobVariableStore (VariableGuid);
  if (EFI_ERROR (Status)) {
    if (mNvFvHeaderCache != NULL) {
      FreePool (mNvFvHeaderCache);
    }

    FreePool (mVariableModuleGlobal);
    return Status;
  }

  mVariableModuleGlobal->MaxVolatileVariableSize = ((PcdGet32 (PcdMaxVolatileVariableSize) != 0) ?
                                                    PcdGet32 (PcdMaxVolatileVariableSize) :
                                                    mVariableModuleGlobal->MaxVariableSize
                                                    );
  //
  // Allocate memory for volatile variable store, note that there is a scratch space to store scratch data.
  //
  ScratchSize = GetMaxVariableSize () * 2;
  mVariableModuleGlobal->ScratchBufferSize = ScratchSize;
  VolatileVariableStore = AllocateRuntimePool (PcdGet32 (PcdVariableStoreSize) + ScratchSize);
  if (VolatileVariableStore == NULL) {
    if (mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
      FreePool ((VOID *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase);
    }

    if (mNvFvHeaderCache != NULL) {
      FreePool (mNvFvHeaderCache);
    }

    FreePool (mVariableModuleGlobal);
    return EFI_OUT_OF_RESOURCES;
  }

  SetMem (VolatileVariableStore, PcdGet32 (PcdVariableStoreSize) + ScratchSize, 0xff);

  //
  // Initialize Variable Specific Data.
  //
  mVariableModuleGlobal->VariableGlobal.VolatileVariableBase = (EFI_PHYSICAL_ADDRESS)(UINTN)VolatileVariableStore;
  mVariableModuleGlobal->VolatileLastVariableOffset          = (UINTN)GetStartPointer (VolatileVariableStore) - (UINTN)VolatileVariableStore;

  CopyGuid (&VolatileVariableStore->Signature, VariableGuid);
  VolatileVariableStore->Size      = PcdGet32 (PcdVariableStoreSize);
  VolatileVariableStore->Format    = VARIABLE_STORE_FORMATTED;
  VolatileVariableStore->State     = VARIABLE_STORE_HEALTHY;
  VolatileVariableStore->Reserved  = 0;
  VolatileVariableStore->Reserved1 = 0;

  return EFI_SUCCESS;
}

/**
  Get the proper fvb handle and/or fvb protocol by the given Flash address.

  @param[in]  Address       The Flash address.
  @param[out] FvbHandle     In output, if it is not NULL, it points to the proper FVB handle.
  @param[out] FvbProtocol   In output, if it is not NULL, it points to the proper FVB protocol.

**/
EFI_STATUS
GetFvbInfoByAddress (
  IN  EFI_PHYSICAL_ADDRESS                Address,
  OUT EFI_HANDLE                          *FvbHandle OPTIONAL,
  OUT EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  **FvbProtocol OPTIONAL
  )
{
  EFI_STATUS                          Status;
  EFI_HANDLE                          *HandleBuffer;
  UINTN                               HandleCount;
  UINTN                               Index;
  EFI_PHYSICAL_ADDRESS                FvbBaseAddress;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_ATTRIBUTES_2                Attributes;
  UINTN                               BlockSize;
  UINTN                               NumberOfBlocks;

  HandleBuffer = NULL;
  //
  // Get all FVB handles.
  //
  Status = GetFvbCountAndBuffer (&HandleCount, &HandleBuffer);
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get the FVB to access variable store.
  //
  Fvb = NULL;
  for (Index = 0; Index < HandleCount; Index += 1, Status = EFI_NOT_FOUND, Fvb = NULL) {
    Status = GetFvbByHandle (HandleBuffer[Index], &Fvb);
    if (EFI_ERROR (Status)) {
      Status = EFI_NOT_FOUND;
      break;
    }

    //
    // Ensure this FVB protocol supported Write operation.
    //
    Status = Fvb->GetAttributes (Fvb, &Attributes);
    if (EFI_ERROR (Status) || ((Attributes & EFI_FVB2_WRITE_STATUS) == 0)) {
      continue;
    }

    //
    // Compare the address and select the right one.
    //
    Status = Fvb->GetPhysicalAddress (Fvb, &FvbBaseAddress);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Assume one FVB has one type of BlockSize.
    //
    Status = Fvb->GetBlockSize (Fvb, 0, &BlockSize, &NumberOfBlocks);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((Address >= FvbBaseAddress) && (Address < (FvbBaseAddress + BlockSize * NumberOfBlocks))) {
      if (FvbHandle != NULL) {
        *FvbHandle = HandleBuffer[Index];
      }

      if (FvbProtocol != NULL) {
        *FvbProtocol = Fvb;
      }

      Status = EFI_SUCCESS;
      break;
    }
  }

  FreePool (HandleBuffer);

  if (Fvb == NULL) {
    Status = EFI_NOT_FOUND;
  }

  return Status;
}
