/** @file
  Provides variable driver extended services.

Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"
#include "VariableParsing.h"
#include "VariableRuntimeCache.h"

/**
  Finds variable in storage blocks of volatile and non-volatile storage areas.

  This code finds variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the variable found.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
EFI_STATUS
EFIAPI
VariableExLibFindVariable (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  EFI_STATUS                    Status;
  VARIABLE_POINTER_TRACK        Variable;
  AUTHENTICATED_VARIABLE_HEADER *AuthVariable;
  PROTECTED_VARIABLE_INFO       VarInfo;

  Status = FindVariable (
             VariableName,
             VendorGuid,
             &Variable,
             &mVariableModuleGlobal->VariableGlobal,
             FALSE
             );
  if (EFI_ERROR (Status)) {
    AuthVariableInfo->Data           = NULL;
    AuthVariableInfo->DataSize       = 0;
    AuthVariableInfo->Attributes     = 0;
    AuthVariableInfo->PubKeyIndex    = 0;
    AuthVariableInfo->MonotonicCount = 0;
    AuthVariableInfo->TimeStamp      = NULL;
    return Status;
  }

  AuthVariableInfo->NameSize        = NameSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->VariableName    = GetVariableNamePtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->VendorGuid      = GetVendorGuidPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->DataSize        = DataSizeOfVariable (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Data            = GetVariableDataPtr (Variable.CurrPtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Attributes      = Variable.CurrPtr->Attributes;
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *) Variable.CurrPtr;
    AuthVariableInfo->PubKeyIndex     = AuthVariable->PubKeyIndex;
    AuthVariableInfo->MonotonicCount  = ReadUnaligned64 (&(AuthVariable->MonotonicCount));
    AuthVariableInfo->TimeStamp       = &AuthVariable->TimeStamp;
  }

  CopyMem (&VarInfo.Header, AuthVariableInfo, sizeof (VarInfo.Header));

  VarInfo.Buffer        = Variable.CurrPtr;
  VarInfo.PlainData     = NULL;
  VarInfo.PlainDataSize = 0;
  VarInfo.Flags.Auth    = mVariableModuleGlobal->VariableGlobal.AuthFormat;

  //
  // In case the variable is encrypted.
  //
  Status = ProtectedVariableLibGetByInfo (&VarInfo);
  if (!EFI_ERROR (Status)) {
    if (VarInfo.PlainData != NULL) {
      AuthVariableInfo->Data      = VarInfo.PlainData;
      AuthVariableInfo->DataSize  = VarInfo.PlainDataSize;
    }
  }

  return EFI_SUCCESS;
}

/**
  Finds next variable in storage blocks of volatile and non-volatile storage areas.

  This code finds next variable in storage blocks of volatile and non-volatile storage areas.
  If VariableName is an empty string, then we just return the first
  qualified variable without comparing VariableName and VendorGuid.

  @param[in]  VariableName          Name of the variable to be found.
  @param[in]  VendorGuid            Variable vendor GUID to be found.
  @param[out] AuthVariableInfo      Pointer to AUTH_VARIABLE_INFO structure for
                                    output of the next variable.

  @retval EFI_INVALID_PARAMETER     If VariableName is not an empty string,
                                    while VendorGuid is NULL.
  @retval EFI_SUCCESS               Variable successfully found.
  @retval EFI_NOT_FOUND             Variable not found

**/
EFI_STATUS
EFIAPI
VariableExLibFindNextVariable (
  IN  CHAR16              *VariableName,
  IN  EFI_GUID            *VendorGuid,
  OUT AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  EFI_STATUS                    Status;
  VARIABLE_HEADER               *VariablePtr;
  AUTHENTICATED_VARIABLE_HEADER *AuthVariablePtr;
  VARIABLE_STORE_HEADER         *VariableStoreHeader[VariableStoreTypeMax];
  PROTECTED_VARIABLE_INFO       VarInfo;

  VariableStoreHeader[VariableStoreTypeVolatile] = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase;
  VariableStoreHeader[VariableStoreTypeHob]      = (VARIABLE_STORE_HEADER *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase;
  VariableStoreHeader[VariableStoreTypeNv]       = mNvVariableCache;

  Status = VariableServiceGetNextVariableInternal (
             VariableName,
             VendorGuid,
             VariableStoreHeader,
             &VariablePtr,
             mVariableModuleGlobal->VariableGlobal.AuthFormat
             );
  if (EFI_ERROR (Status)) {
    AuthVariableInfo->VariableName   = NULL;
    AuthVariableInfo->VendorGuid     = NULL;
    AuthVariableInfo->Data           = NULL;
    AuthVariableInfo->DataSize       = 0;
    AuthVariableInfo->Attributes     = 0;
    AuthVariableInfo->PubKeyIndex    = 0;
    AuthVariableInfo->MonotonicCount = 0;
    AuthVariableInfo->TimeStamp      = NULL;
    return Status;
  }

  AuthVariableInfo->NameSize        = NameSizeOfVariable (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->VariableName    = GetVariableNamePtr (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->VendorGuid      = GetVendorGuidPtr (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->DataSize        = DataSizeOfVariable (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Data            = GetVariableDataPtr (VariablePtr, mVariableModuleGlobal->VariableGlobal.AuthFormat);
  AuthVariableInfo->Attributes      = VariablePtr->Attributes;
  if (mVariableModuleGlobal->VariableGlobal.AuthFormat) {
    AuthVariablePtr                  = (AUTHENTICATED_VARIABLE_HEADER *)VariablePtr;
    AuthVariableInfo->PubKeyIndex    = AuthVariablePtr->PubKeyIndex;
    AuthVariableInfo->MonotonicCount = ReadUnaligned64 (&(AuthVariablePtr->MonotonicCount));
    AuthVariableInfo->TimeStamp      = &AuthVariablePtr->TimeStamp;
  }

  CopyMem (&VarInfo.Header, AuthVariableInfo, sizeof (VarInfo.Header));

  VarInfo.Buffer        = VariablePtr;
  VarInfo.PlainData     = NULL;
  VarInfo.PlainDataSize = 0;

  Status = ProtectedVariableLibGetByInfo (&VarInfo);
  if (!EFI_ERROR (Status)) {
    if (VarInfo.PlainData != NULL) {
      AuthVariableInfo->Data      = VarInfo.PlainData;
      AuthVariableInfo->DataSize  = VarInfo.PlainDataSize;
    }
  }

  return EFI_SUCCESS;
}

/**
  Update the variable region with Variable information.

  @param[in] AuthVariableInfo       Pointer AUTH_VARIABLE_INFO structure for
                                    input of the variable.

  @retval EFI_SUCCESS               The update operation is success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.

**/
EFI_STATUS
EFIAPI
VariableExLibUpdateVariable (
  IN AUTH_VARIABLE_INFO  *AuthVariableInfo
  )
{
  VARIABLE_POINTER_TRACK  Variable;

  FindVariable (AuthVariableInfo->VariableName, AuthVariableInfo->VendorGuid, &Variable, &mVariableModuleGlobal->VariableGlobal, FALSE);
  return UpdateVariable (
           AuthVariableInfo->VariableName,
           AuthVariableInfo->VendorGuid,
           AuthVariableInfo->Data,
           AuthVariableInfo->DataSize,
           AuthVariableInfo->Attributes,
           AuthVariableInfo->PubKeyIndex,
           AuthVariableInfo->MonotonicCount,
           &Variable,
           AuthVariableInfo->TimeStamp
           );
}

/**
  Get scratch buffer.

  @param[in, out] ScratchBufferSize Scratch buffer size. If input size is greater than
                                    the maximum supported buffer size, this value contains
                                    the maximum supported buffer size as output.
  @param[out]     ScratchBuffer     Pointer to scratch buffer address.

  @retval EFI_SUCCESS       Get scratch buffer successfully.
  @retval EFI_UNSUPPORTED   If input size is greater than the maximum supported buffer size.

**/
EFI_STATUS
EFIAPI
VariableExLibGetScratchBuffer (
  IN OUT UINTN  *ScratchBufferSize,
  OUT    VOID   **ScratchBuffer
  )
{
  UINTN  MaxBufferSize;

  MaxBufferSize = mVariableModuleGlobal->ScratchBufferSize;
  if (*ScratchBufferSize > MaxBufferSize) {
    *ScratchBufferSize = MaxBufferSize;
    return EFI_UNSUPPORTED;
  }

  *ScratchBuffer = GetEndPointer ((VARIABLE_STORE_HEADER *)((UINTN)mVariableModuleGlobal->VariableGlobal.VolatileVariableBase));
  return EFI_SUCCESS;
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
VariableExLibCheckRemainingSpaceForConsistency (
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
  Return TRUE if at OS runtime.

  @retval TRUE If at OS runtime.
  @retval FALSE If at boot time.

**/
BOOLEAN
EFIAPI
VariableExLibAtRuntime (
  VOID
  )
{
  return AtRuntime ();
}

/**
  Update partial data of a variable on NV storage and/or cached copy.

  @param[in]  VariableInfo  Pointer to a variable with detailed information.
  @param[in]  Offset        Offset to write from.
  @param[in]  Size          Size of data Buffer to update.
  @param[in]  Buffer        Pointer to data buffer to update.

  @retval EFI_SUCCESS             The variable data was updated successfully.
  @retval EFI_UNSUPPORTED         If this function is called directly in runtime.
  @retval EFI_INVALID_PARAMETER   If VariableInfo, Buffer or Size are not valid.
  @retval Others                  Failed to update NV storage or variable cache.

**/
EFI_STATUS
EFIAPI
VariableExLibUpdateNvVariable (
  IN  PROTECTED_VARIABLE_INFO     *VariableInfo,
  IN  UINTN                       Offset,
  IN  UINT32                      Size,
  IN  UINT8                       *Buffer
  )
{
  EFI_STATUS                           Status;
  VARIABLE_GLOBAL                     *Global;
  VARIABLE_RUNTIME_CACHE              *CacheInstance;
  VARIABLE_HEADER                     *VariableCache;

  if (mVariableModuleGlobal == NULL || mNvVariableCache == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Flush the cache to store.
  //
  if (Size == (UINT32)-1) {
    Status = FtwVariableSpace (
              mVariableModuleGlobal->VariableGlobal.NonVolatileVariableBase,
              mNvVariableCache
              );
    if (!EFI_ERROR (Status)
        && mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
      FlushHobVariableToFlash (NULL, NULL);
      if(mVariableModuleGlobal->VariableGlobal.HobVariableBase != 0) {
        FreePool ((VOID *)(UINTN)mVariableModuleGlobal->VariableGlobal.HobVariableBase);
        mVariableModuleGlobal->VariableGlobal.HobVariableBase = 0;
      }
    }

    return Status;
  }

  if (VariableInfo == NULL
      || VariableInfo->StoreIndex == VAR_INDEX_INVALID
      || Buffer == NULL
      || Size == 0)
  {
    ASSERT (VariableInfo != NULL);
    ASSERT (VariableInfo->StoreIndex != VAR_INDEX_INVALID);
    ASSERT (Buffer != NULL);
    ASSERT (Size != 0);
    return EFI_INVALID_PARAMETER;
  }

  Global = &mVariableModuleGlobal->VariableGlobal;

  VariableCache = (VARIABLE_HEADER *)((UINTN)mNvVariableCache + (UINTN)VariableInfo->StoreIndex);

  ASSERT (StrCmp (VariableInfo->Header.VariableName,
                  GetVariableNamePtr (VariableCache, Global->AuthFormat)) == 0);
  ASSERT (CompareGuid (VariableInfo->Header.VendorGuid,
                       GetVendorGuidPtr (VariableCache, Global->AuthFormat)));

  //
  // Forcibly update part data of flash copy of the variable ...
  //
  Status =  UpdateVariableStore (
              Global,
              FALSE,
              FALSE,
              mVariableModuleGlobal->FvbInstance,
              (UINTN)(Global->NonVolatileVariableBase + VariableInfo->StoreIndex + Offset),
              Size,
              Buffer
              );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // ... as well as the local cached copy.
  //
  CopyMem ((VOID *)((UINTN)VariableCache + Offset), Buffer, Size);

  //
  // Sync remote cached copy.
  //
  CacheInstance = &Global->VariableRuntimeCacheContext.VariableRuntimeNvCache;
  if (CacheInstance->Store != NULL) {
    Status =  SynchronizeRuntimeVariableCache (
                CacheInstance,
                (UINTN)VariableInfo->StoreIndex + Offset,
                Size
                );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

