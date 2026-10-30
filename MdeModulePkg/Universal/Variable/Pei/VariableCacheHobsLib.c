/** @file
  Build variable runtime cache information HOBs from platform-provided memory.

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Variable.h"

/**
  This is the constructor to build the gEdkiiVariableRuntimeCacheInfoHobGuid GUIDed HOB in library.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  Failed to build VariableRuntimeCacheInfo GUIDed Hob.

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
    DEBUG ((DEBUG_WARN, "Failed to find VariableRuntimeCacheInfo memory allocation Hob\n"));
    // Do not treat this as an error, so that the system can continue booting even if the HOB is not found.
    Buffer = NULL;
    Pages  = 0;
  } else {
    MemoryAllocationHob = &((EFI_HOB_MEMORY_ALLOCATION *)Hob)->AllocDescriptor;
    Buffer              = (VOID *)(UINTN)MemoryAllocationHob->MemoryBaseAddress;
    Pages               = (UINTN)EFI_SIZE_TO_PAGES (MemoryAllocationHob->MemoryLength);
  }

  Status = BuildVariableRuntimeCacheInfoHobFromBuffer (NULL, Buffer, &Pages);
  if ((Buffer == NULL) && (Status == EFI_BUFFER_TOO_SMALL)) {
    DEBUG ((DEBUG_INFO, "%a Required pages: 0x%lx\n", __func__, Pages));
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to build VariableRuntimeCacheInfo Hob from buffer: %r\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "Successfully built VariableRuntimeCacheInfo Hob from buffer\n"));
  }

  return EFI_SUCCESS;
}
