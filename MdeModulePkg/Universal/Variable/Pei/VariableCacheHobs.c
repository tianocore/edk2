/** @file
  Implement ReadOnly Variable Services required by PEIM and install
  PEI ReadOnly Varaiable2 PPI. These services operates the non volatile storage space.

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"

/**

  Gets the pointer to the first variable header in given variable store area.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the first variable header.

**/
VARIABLE_HEADER *
GetStartPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The start of variable store.
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN (VarStoreHeader + 1);
}

/**

  Gets the pointer to the end of the variable storage area.

  This function gets pointer to the end of the variable storage
  area, according to the input variable store header.

  @param[in] VarStoreHeader  Pointer to the Variable Store Header.

  @return Pointer to the end of the variable storage area.

**/
VARIABLE_HEADER *
GetEndPointer (
  IN VARIABLE_STORE_HEADER  *VarStoreHeader
  )
{
  //
  // The end of variable store
  //
  return (VARIABLE_HEADER *)HEADER_ALIGN ((UINTN)VarStoreHeader + VarStoreHeader->Size);
}

/**
  This code gets the size of variable header.

  @param AuthFlag   Authenticated variable flag.

  @return Size of variable header in bytes in type UINTN.

**/
UINTN
GetVariableHeaderSize (
  IN  BOOLEAN  AuthFlag
  )
{
  UINTN  Value;

  if (AuthFlag) {
    Value = sizeof (AUTHENTICATED_VARIABLE_HEADER);
  } else {
    Value = sizeof (VARIABLE_HEADER);
  }

  return Value;
}

/**
  This code gets the size of name of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
NameSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFlag) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->NameSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->NameSize;
  }
}

/**
  This code gets the size of data of variable.

  @param  Variable  Pointer to the Variable Header.
  @param  AuthFlag  Authenticated variable flag.

  @return Size of variable in bytes in type UINTN.

**/
UINTN
DataSizeOfVariable (
  IN  VARIABLE_HEADER  *Variable,
  IN  BOOLEAN          AuthFlag
  )
{
  AUTHENTICATED_VARIABLE_HEADER  *AuthVariable;

  AuthVariable = (AUTHENTICATED_VARIABLE_HEADER *)Variable;
  if (AuthFlag) {
    if ((AuthVariable->State == (UINT8)(-1)) ||
        (AuthVariable->DataSize == (UINT32)(-1)) ||
        (AuthVariable->NameSize == (UINT32)(-1)) ||
        (AuthVariable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)AuthVariable->DataSize;
  } else {
    if ((Variable->State == (UINT8)(-1)) ||
        (Variable->DataSize == (UINT32)(-1)) ||
        (Variable->NameSize == (UINT32)(-1)) ||
        (Variable->Attributes == (UINT32)(-1)))
    {
      return 0;
    }

    return (UINTN)Variable->DataSize;
  }
}

/**
  This code gets the pointer to the variable name.

  @param   Variable  Pointer to the Variable Header.
  @param   AuthFlag  Authenticated variable flag.

  @return  A CHAR16* pointer to Variable Name.

**/
CHAR16 *
GetVariableNamePtr (
  IN VARIABLE_HEADER  *Variable,
  IN BOOLEAN          AuthFlag
  )
{
  return (CHAR16 *)((UINTN)Variable + GetVariableHeaderSize (AuthFlag));
}

/**
  This code gets the pointer to the variable data.

  @param   Variable         Pointer to the Variable Header.
  @param   VariableHeader   Pointer to the Variable Header that has consecutive content.
  @param   AuthFlag         Authenticated variable flag.

  @return  A UINT8* pointer to Variable Data.

**/
UINT8 *
GetVariableDataPtr (
  IN  VARIABLE_HEADER  *Variable,
  IN  VARIABLE_HEADER  *VariableHeader,
  IN  BOOLEAN          AuthFlag
  )
{
  UINTN  Value;

  //
  // Be careful about pad size for alignment
  //
  Value  =  (UINTN)GetVariableNamePtr (Variable, AuthFlag);
  Value += NameSizeOfVariable (VariableHeader, AuthFlag);
  Value += GET_PAD_SIZE (NameSizeOfVariable (VariableHeader, AuthFlag));

  return (UINT8 *)Value;
}

/**
  This code gets the pointer to the next variable header.

  @param  StoreInfo         Pointer to variable store info structure.
  @param  Variable          Pointer to the Variable Header.
  @param  VariableHeader    Pointer to the Variable Header that has consecutive content.

  @return  A VARIABLE_HEADER* pointer to next variable header.

**/
VARIABLE_HEADER *
GetNextVariablePtr (
  IN  VARIABLE_STORE_INFO  *StoreInfo,
  IN  VARIABLE_HEADER      *Variable,
  IN  VARIABLE_HEADER      *VariableHeader
  )
{
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  EFI_PHYSICAL_ADDRESS  SpareAddress;
  UINTN                 Value;

  Value  =  (UINTN)GetVariableDataPtr (Variable, VariableHeader, StoreInfo->AuthFlag);
  Value += DataSizeOfVariable (VariableHeader, StoreInfo->AuthFlag);
  Value += GET_PAD_SIZE (DataSizeOfVariable (VariableHeader, StoreInfo->AuthFlag));
  //
  // Be careful about pad size for alignment
  //
  Value = HEADER_ALIGN (Value);

  if (StoreInfo->FtwLastWriteData != NULL) {
    TargetAddress = StoreInfo->FtwLastWriteData->TargetAddress;
    SpareAddress  = StoreInfo->FtwLastWriteData->SpareAddress;
    if (((UINTN)Variable < (UINTN)TargetAddress) && (Value >= (UINTN)TargetAddress)) {
      //
      // Next variable is in spare block.
      //
      Value = (UINTN)SpareAddress + (Value - (UINTN)TargetAddress);
    }
  }

  return (VARIABLE_HEADER *)Value;
}

/**
  Get HOB variable store.

  @param[out] StoreInfo             Return the store info.
  @param[out] VariableStoreHeader   Return variable store header.

**/
VOID
GetHobVariableStore (
  OUT VARIABLE_STORE_INFO    *StoreInfo,
  OUT VARIABLE_STORE_HEADER  **VariableStoreHeader
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

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

  GuidHob = GetFirstGuidHob (&gEfiAuthenticatedVariableGuid);
  if (GuidHob != NULL) {
    *VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
    StoreInfo->AuthFlag  = TRUE;
  } else {
    GuidHob = GetFirstGuidHob (&gEfiVariableGuid);
    if (GuidHob != NULL) {
      *VariableStoreHeader = (VARIABLE_STORE_HEADER *)GET_GUID_HOB_DATA (GuidHob);
      StoreInfo->AuthFlag  = FALSE;
    }
  }
}

/**
  Calculate the auth variable storage size converted from normal variable storage.

  @param[in]  StoreInfo         Pointer to the store info
  @param[in]  NormalHobVarStorage  Pointer to the normal variable storage header

  @retval the auth variable storage size
**/
UINTN
CalculateAuthVarStorageSize (
  IN  VARIABLE_STORE_INFO    *StoreInfo,
  IN  VARIABLE_STORE_HEADER  *NormalHobVarStorage
  )
{
  VARIABLE_HEADER  *StartPtr;
  VARIABLE_HEADER  *EndPtr;
  UINTN            AuthVarStroageSize;

  AuthVarStroageSize = sizeof (VARIABLE_STORE_HEADER);

  //
  // Calculate Auth Variable Storage Size
  //
  StartPtr = GetStartPointer (NormalHobVarStorage);
  EndPtr   = GetEndPointer (NormalHobVarStorage);
  while (StartPtr < EndPtr) {
    if (StartPtr->State == VAR_ADDED) {
      AuthVarStroageSize  = HEADER_ALIGN (AuthVarStroageSize);
      AuthVarStroageSize += sizeof (AUTHENTICATED_VARIABLE_HEADER);
      AuthVarStroageSize += StartPtr->NameSize + GET_PAD_SIZE (StartPtr->NameSize);
      AuthVarStroageSize += StartPtr->DataSize + GET_PAD_SIZE (StartPtr->DataSize);
    }

    StartPtr = GetNextVariablePtr (StoreInfo, StartPtr, StartPtr);
  }

  return AuthVarStroageSize;
}

/**
  Calculate Hob variable cache size.

  The returned size covers the following combination supported by the variable
  driver:
  1. Auth HOB variable store (with Auth NV variable store): the HOB store
     is used as is.
  2. Normal HOB variable store + Normal NV variable store: the HOB store is
     used as is.
  3. Normal HOB variable store + Auth NV variable store: the HOB store is
     converted to Auth format.

  @retval Maximum of Hob variable cache size.

**/
UINTN
CalculateHobVariableCacheSize (
  VOID
  )
{
  VARIABLE_STORE_INFO    StoreInfo;
  VARIABLE_STORE_HEADER  *VariableStoreHeader;

  VariableStoreHeader = NULL;
  ZeroMem (&StoreInfo, sizeof (VARIABLE_STORE_INFO));
  GetHobVariableStore (&StoreInfo, &VariableStoreHeader);

  if (VariableStoreHeader == NULL) {
    return 0;
  }

  if (StoreInfo.AuthFlag) {
    return VariableStoreHeader->Size;
  }

  return MAX (
           (UINTN)VariableStoreHeader->Size,
           CalculateAuthVarStorageSize (&StoreInfo, VariableStoreHeader)
           );
}

/**
  Calculate Nv variable cache size.

  Regardless of the NV auth flag status of the variable store, subtracting the
  minimum FV header size makes the result no smaller than the actual variable
  store size.

  @retval Maximum of Nv variable cache size.

**/
UINTN
CalculateNvVariableCacheSize (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  NvStorageBase;
  UINT32                NvStorageSize;
  UINT64                NvStorageSize64;

  if (PcdGetBool (PcdEmuVariableNvModeEnable)) {
    return PcdGet32 (PcdVariableStoreSize);
  }

  Status = GetVariableFlashNvStorageInfo (&NvStorageBase, &NvStorageSize64);
  ASSERT_EFI_ERROR (Status);

  Status = SafeUint64ToUint32 (NvStorageSize64, &NvStorageSize);
  ASSERT_EFI_ERROR (Status);

  return NvStorageSize - sizeof (EFI_FIRMWARE_VOLUME_HEADER);
}

/**
  Build the variable runtime cache info HOB from a buffer.

  @param  Buffer  Pointer to the buffer containing the cache info. This parameter is optional and can be NULL.
  @param  Pages   Pointer to the number of pages in the buffer. On input, it specifies the number of pages
                  allocated for the buffer. On output, it returns the actual number of pages needed by the cache info.

  @retval EFI_SUCCESS           The HOB was built successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL or invalid.
  @retval EFI_BUFFER_TOO_SMALL   The buffer provided is too small to hold the cache info.
  @retval EFI_DEVICE_ERROR      The HOB could not be built due to a device error.
**/
EFI_STATUS
EFIAPI
BuildVariableRuntimeCacheInfoHobFromBuffer (
  IN       VOID   *Buffer OPTIONAL,
  IN OUT   UINTN  *Pages
  )
{
  VARIABLE_RUNTIME_CACHE_INFO  TempHobBuffer;
  VARIABLE_RUNTIME_CACHE_INFO  *VariableRuntimeCacheInfo;
  EFI_STATUS                   Status;
  UINTN                        BufferSize;
  UINTN                        IndexPages;
  UINTN                        NeededPages;

  if (Pages == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Buffer == NULL) && (*Pages != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&TempHobBuffer, sizeof (VARIABLE_RUNTIME_CACHE_INFO));

  IndexPages = 0;

  //
  // AllocateRuntimePages for CACHE_INFO_FLAG and unblock it.
  //
  NeededPages = EFI_SIZE_TO_PAGES (sizeof (CACHE_INFO_FLAG));
  if ((Buffer != NULL) && (NeededPages + IndexPages <= *Pages)) {
    TempHobBuffer.CacheInfoFlagBuffer = (UINTN)Buffer + IndexPages * EFI_PAGE_SIZE;
  }

  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: CACHE_INFO_FLAG Buffer is: 0x%lx, number of pages is: 0x%x\n",
    TempHobBuffer.CacheInfoFlagBuffer,
    NeededPages
    ));

  IndexPages += NeededPages;

  //
  // AllocateRuntimePages for VolatileCache and unblock it.
  //
  BufferSize = PcdGet32 (PcdVariableStoreSize);
  if (BufferSize > 0) {
    NeededPages = EFI_SIZE_TO_PAGES (BufferSize);
    if ((Buffer != NULL) && (NeededPages + IndexPages <= *Pages)) {
      TempHobBuffer.RuntimeVolatileCacheBuffer = (UINTN)Buffer + IndexPages * EFI_PAGE_SIZE;
      TempHobBuffer.RuntimeVolatileCachePages  = NeededPages;
    }
  } else {
    NeededPages = 0;
  }

  IndexPages += NeededPages;

  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: Volatile cache Buffer is: 0x%lx, number of pages is: 0x%lx\n",
    TempHobBuffer.RuntimeVolatileCacheBuffer,
    TempHobBuffer.RuntimeVolatileCachePages
    ));

  //
  // AllocateRuntimePages for NVCache and unblock it.
  //
  BufferSize = CalculateNvVariableCacheSize ();
  if (BufferSize > 0) {
    NeededPages = EFI_SIZE_TO_PAGES (BufferSize);
    if ((Buffer != NULL) && (NeededPages + IndexPages <= *Pages)) {
      TempHobBuffer.RuntimeNvCacheBuffer = (UINTN)Buffer + IndexPages * EFI_PAGE_SIZE;
      TempHobBuffer.RuntimeNvCachePages  = NeededPages;
    }
  } else {
    NeededPages = 0;
  }

  IndexPages += NeededPages;

  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: NV cache Buffer is: 0x%lx, number of pages is: 0x%lx\n",
    TempHobBuffer.RuntimeNvCacheBuffer,
    TempHobBuffer.RuntimeNvCachePages
    ));

  //
  // AllocateRuntimePages for HobCache and unblock it.
  //
  BufferSize = CalculateHobVariableCacheSize ();
  if (BufferSize > 0) {
    NeededPages = EFI_SIZE_TO_PAGES (BufferSize);
    if ((Buffer != NULL) && (NeededPages + IndexPages <= *Pages)) {
      TempHobBuffer.RuntimeHobCacheBuffer = (UINTN)Buffer + IndexPages * EFI_PAGE_SIZE;
      TempHobBuffer.RuntimeHobCachePages  = NeededPages;
    }
  } else {
    NeededPages = 0;
  }

  IndexPages += NeededPages;

  DEBUG ((
    DEBUG_INFO,
    "PeiVariable: HOB cache Buffer is: 0x%lx, number of pages is: 0x%lx\n",
    TempHobBuffer.RuntimeHobCacheBuffer,
    TempHobBuffer.RuntimeHobCachePages
    ));

  if (IndexPages > *Pages) {
    DEBUG ((
      DEBUG_WARN,
      "PeiVariable: Not enough pages, required: 0x%lx, available: 0x%lx\n",
      IndexPages,
      *Pages
      ));
    *Pages = IndexPages;
    return EFI_BUFFER_TOO_SMALL;
  }

  // We are all good here, unblock the memory requests for the runtime caches.
  Status = MmUnblockMemoryRequest (
             (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer,
             IndexPages
             );
  if ((Status != EFI_UNSUPPORTED) && EFI_ERROR (Status)) {
    return Status;
  }

  VariableRuntimeCacheInfo = BuildGuidHob (&gEdkiiVariableRuntimeCacheInfoHobGuid, sizeof (VARIABLE_RUNTIME_CACHE_INFO));
  ASSERT (VariableRuntimeCacheInfo != NULL);
  CopyMem (VariableRuntimeCacheInfo, &TempHobBuffer, sizeof (VARIABLE_RUNTIME_CACHE_INFO));

  return EFI_SUCCESS;
}

/**
  This is the constructor to build the gEdkiiVariableRuntimeCacheInfoHobGuid HOB in library.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  Failed to build VariableRuntimeCacheInfo Hob.

**/
EFI_STATUS
EFIAPI
VariableCacheHobsLibConstructor (
  VOID
  )
{
  EFI_STATUS                        Status;
  UINTN                             Pages;
  EFI_HOB_MEMORY_ALLOCATION_HEADER  *MemoryAllocationHob;
  VOID                              *HobStart;
  VOID                              *Hob;
  VOID                              *Buffer;

  if (!FeaturePcdGet (PcdEnableVariableRuntimeCache)) {
    return EFI_SUCCESS;
  }

  HobStart = GetHobList ();
  if (HobStart == NULL) {
    return EFI_NOT_STARTED;
  }

  Hob = GetNextMemoryAllocationGuidHob (
          &gEdkiiVariableRuntimeCacheInfoHobGuid,
          HobStart
          );
  if (Hob == NULL) {
    DEBUG ((DEBUG_WARN, "Failed to find VariableRuntimeCacheInfo Hob\n"));
    // Do not treat this as an error, so that the system can continue booting even if the HOB is not found.
    Buffer = NULL;
    Pages  = 0;
  } else {
    MemoryAllocationHob = &((EFI_HOB_MEMORY_ALLOCATION *)Hob)->AllocDescriptor;
    Buffer              = (VOID *)(UINTN)MemoryAllocationHob->MemoryBaseAddress;
    Pages               = (UINTN)EFI_SIZE_TO_PAGES (MemoryAllocationHob->MemoryLength);
  }

  Status = BuildVariableRuntimeCacheInfoHobFromBuffer ((VOID *)(UINTN)Buffer, &Pages);
  if ((Buffer == NULL) && (Status == EFI_BUFFER_TOO_SMALL)) {
    DEBUG ((DEBUG_INFO, "%a Required pages: 0x%lx\n", __func__, Pages));
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to build VariableRuntimeCacheInfo Hob from buffer: %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "Successfully built VariableRuntimeCacheInfo Hob from buffer\n"));
  }

  return EFI_SUCCESS;
}
