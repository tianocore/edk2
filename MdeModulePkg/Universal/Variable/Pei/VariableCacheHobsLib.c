/** @file
  Build variable runtime cache information HOBs from platform-provided memory.

Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariableCacheHobs.h"

#include <Pi/PiMultiPhase.h>
#include <Guid/VariableRuntimeCacheInfo.h>

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>

/**
  This is the constructor to build the gEdkiiVariableRuntimeCacheInfoHobGuid GUIDed HOB in library.

  The tagged allocation must be page aligned and have type EfiRuntimeServicesData.
  SEC callers should provide a static carveout. Dynamic PEI allocations should
  use initialized runtime memory bins with sufficient capacity for S4 stability.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   The allocation base is zero, the base or length is
                                  not page aligned, or the memory type is not EfiRuntimeServicesData.
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
    if ((MemoryAllocationHob->MemoryBaseAddress == 0) ||
        !IS_ALIGNED (MemoryAllocationHob->MemoryBaseAddress, EFI_PAGE_SIZE))
    {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Variable runtime cache allocation base 0x%lx must be nonzero and page aligned\n",
        __func__,
        MemoryAllocationHob->MemoryBaseAddress
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (!IS_ALIGNED (MemoryAllocationHob->MemoryLength, EFI_PAGE_SIZE)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Variable runtime cache allocation length 0x%lx is not page aligned\n",
        __func__,
        MemoryAllocationHob->MemoryLength
        ));
      return EFI_INVALID_PARAMETER;
    }

    if (MemoryAllocationHob->MemoryType != EfiRuntimeServicesData) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Variable runtime cache allocation memory type %u is not EfiRuntimeServicesData\n",
        __func__,
        MemoryAllocationHob->MemoryType
        ));
      return EFI_INVALID_PARAMETER;
    }

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
