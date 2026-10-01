/** @file
  Variable runtime cache sizing and HOB construction.

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableCacheHobs.h"
#include "VariableCommon.h"

#include <Pi/PiMultiPhase.h>
#include <Guid/VariableRuntimeCacheInfo.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MmUnblockMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/SafeIntLib.h>
#include <Library/VariableFlashInfoLib.h>

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

  @param[in]  NvAuthFlag         Pointer to the authenticated variable flag. When NULL, the variable store format is unknown, and the cache info will reserve enough space for either supported format.

  @retval Maximum of Hob variable cache size.

**/
UINTN
CalculateHobVariableCacheSize (
  IN  BOOLEAN  *NvAuthFlag OPTIONAL
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

  if (NvAuthFlag == NULL) {
    if (StoreInfo.AuthFlag) {
      return VariableStoreHeader->Size;
    }

    return MAX (
             (UINTN)VariableStoreHeader->Size,
             CalculateAuthVarStorageSize (&StoreInfo, VariableStoreHeader)
             );
  }

  // Ensured NvAuthFlag is not NULL at this point
  if (*NvAuthFlag == StoreInfo.AuthFlag) {
    return VariableStoreHeader->Size;
  } else {
    //
    // Normal NV variable store + Auth HOB variable store is not supported
    //
    ASSERT (*NvAuthFlag && (!StoreInfo.AuthFlag));

    //
    // Need to calculate auth variable storage size converted from normal variable storage
    //
    return CalculateAuthVarStorageSize (&StoreInfo, VariableStoreHeader);
  }
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

  @param  NvAuthFlag  Pointer to the authenticated variable flag. When NULL, the variable store format is unknown, and the cache info will reserve enough space for either supported format.
  @param  Buffer      Pointer to the buffer containing the cache info. This parameter is optional and can be NULL.
  @param  Pages       Pointer to the number of pages in the buffer. On input, it specifies the number of pages
                      allocated for the buffer. On output, it returns the actual number of pages needed by the cache info.

  @retval EFI_SUCCESS           The HOB was built successfully.
  @retval EFI_INVALID_PARAMETER One or more of the input parameters are NULL or invalid.
  @retval EFI_BUFFER_TOO_SMALL  The buffer provided is too small to hold the cache info.
  @retval EFI_DEVICE_ERROR      The HOB could not be built due to a device error.
**/
EFI_STATUS
EFIAPI
BuildVariableRuntimeCacheInfoHobFromBuffer (
  IN       BOOLEAN  *NvAuthFlag OPTIONAL,
  IN       VOID     *Buffer OPTIONAL,
  IN OUT   UINTN    *Pages
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
  // Calculate needed pages for CACHE_INFO_FLAG.
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
  // Calculate needed pages for VolatileCache.
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
  // Calculate needed pages for NVCache.
  //
  BufferSize = CalculateNvVariableCacheSize ();
  if (BufferSize > 0) {
    NeededPages = EFI_SIZE_TO_PAGES (BufferSize);
    if ((Buffer != NULL) && (NeededPages + IndexPages <= *Pages)) {
      TempHobBuffer.RuntimeNvCacheBuffer = (UINTN)Buffer + EFI_PAGES_TO_SIZE (IndexPages);
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
  // Calculate needed pages for HobCache.
  //
  BufferSize = CalculateHobVariableCacheSize (NvAuthFlag);
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
  if (VariableRuntimeCacheInfo == NULL) {
    ASSERT (VariableRuntimeCacheInfo != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (VariableRuntimeCacheInfo, &TempHobBuffer, sizeof (VARIABLE_RUNTIME_CACHE_INFO));

  return EFI_SUCCESS;
}
