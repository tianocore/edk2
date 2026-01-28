/** @file

  Shared logic between cores to work with memory bins for S4 resume stability.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Pi/PiMultiPhase.h>

#include <Guid/MemoryTypeInformation.h>

#include <Pi/PiHob.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Library/MemoryBinLib.h>

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             | \
                                EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                EFI_RESOURCE_ATTRIBUTE_PERSISTENT          | \
                                EFI_RESOURCE_ATTRIBUTE_SPECIAL_PURPOSE     )

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     | \
                                   EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                   EFI_RESOURCE_ATTRIBUTE_TESTED      )

EFI_PHYSICAL_ADDRESS  mDefaultMaximumAddress = MAX_ALLOC_ADDRESS;
EFI_PHYSICAL_ADDRESS  mDefaultBaseAddress    = MAX_ALLOC_ADDRESS;

EFI_MEMORY_TYPE_INFORMATION  gMemoryTypeInformation[EfiMaxMemoryType + 1] = {
  { EfiReservedMemoryType,      0 },
  { EfiLoaderCode,              0 },
  { EfiLoaderData,              0 },
  { EfiBootServicesCode,        0 },
  { EfiBootServicesData,        0 },
  { EfiRuntimeServicesCode,     0 },
  { EfiRuntimeServicesData,     0 },
  { EfiConventionalMemory,      0 },
  { EfiUnusableMemory,          0 },
  { EfiACPIReclaimMemory,       0 },
  { EfiACPIMemoryNVS,           0 },
  { EfiMemoryMappedIO,          0 },
  { EfiMemoryMappedIOPortSpace, 0 },
  { EfiPalCode,                 0 },
  { EfiPersistentMemory,        0 },
  { EfiUnacceptedMemoryType,    0 },
  { EfiMaxMemoryType,           0 }
};

BOOLEAN  mMemoryTypeInformationInitialized = FALSE;

EFI_MEMORY_TYPE_STATISTICS  mMemoryTypeStatistics[EfiMaxMemoryType + 1] = {
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE },  // EfiReservedMemoryType
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiLoaderCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiLoaderData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiBootServicesCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiBootServicesData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE  },  // EfiRuntimeServicesCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE  },  // EfiRuntimeServicesData
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiConventionalMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiUnusableMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE },  // EfiACPIReclaimMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE },  // EfiACPIMemoryNVS
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiMemoryMappedIO
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiMemoryMappedIOPortSpace
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  TRUE  },  // EfiPalCode
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE },  // EfiPersistentMemory
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, TRUE,  FALSE },  // EfiUnacceptedMemoryType
  { 0, MAX_ALLOC_ADDRESS, 0, 0, EfiMaxMemoryType, FALSE, FALSE }   // EfiMaxMemoryType
};

/**
  Calculate total memory bin size needed.

  @param BinTop The top address of the memory bins. This is an optional parameter.
                If non-zero, alignment requirements will be considered in the calculation.

  @return The total memory bin size needed.

**/
UINT64
CalculateTotalMemoryBinSizeNeeded (
  UINTN  BinTop
  )
{
  UINTN   Index;
  UINT64  TotalSize;
  UINTN   Granularity;

  Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;

  //
  // Loop through each memory type in the order specified by the gMemoryTypeInformation[] array
  //
  TotalSize = 0;
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    if ((gMemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
        (gMemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
        (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
        (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
    {
      Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
    }

    // gMemoryTypeInformation[Index].NumberOfPages is already aligned to the allocation granularity
    TotalSize += LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);

    // BinTop is optional
    if (BinTop == 0) {
      continue;
    }

    BinTop    -= (UINTN)LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
    TotalSize += (BinTop & (Granularity - 1));
    BinTop    &= ~(Granularity - 1);
  }

  return TotalSize;
}

/**
  Get the Memory Type Information HOB if it exists and populate gMemoryTypeInformation.

  @return EFI_STATUS                      On EFI_SUCCESS, gMemoryTypeInformation points to the
                                          Memory Type Information.
  @return EFI_NOT_FOUND                   No valid Memory Type Information HOB found.
**/
EFI_STATUS
EFIAPI
PopulateMemoryTypeInformation (
  VOID
  )
{
  UINTN                        DataSize;
  EFI_MEMORY_TYPE_INFORMATION  *EfiMemoryTypeInformation;
  EFI_HOB_GUID_TYPE            *GuidHob;
  UINTN                        Index;
  UINT32                       Granularity;

  GuidHob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);
  if (GuidHob != NULL) {
    EfiMemoryTypeInformation = GET_GUID_HOB_DATA (GuidHob);
    DataSize                 = GET_GUID_HOB_DATA_SIZE (GuidHob);
    if ((EfiMemoryTypeInformation != NULL) && (DataSize > 0) && (DataSize <= (EfiMaxMemoryType + 1) * sizeof (EFI_MEMORY_TYPE_INFORMATION))) {
      CopyMem (&gMemoryTypeInformation, EfiMemoryTypeInformation, DataSize);

      for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
        //
        // Make sure the memory type in the gMemoryTypeInformation[] array is valid
        //
        if (gMemoryTypeInformation[Index].Type > EfiMaxMemoryType) {
          continue;
        }

        if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
          if ((gMemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
              (gMemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
              (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
              (gMemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
          {
            Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
          } else {
            Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
          }

          // Align the number of pages to the allocation granularity
          gMemoryTypeInformation[Index].NumberOfPages = (UINT32)RShiftU64 (ALIGN_VALUE (LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT), Granularity), EFI_PAGE_SHIFT);
        }
      }

      return EFI_SUCCESS;
    }

    DEBUG ((DEBUG_ERROR, "%a: Invalid Memory Type Information HOB data\n", __func__));
    ASSERT (FALSE);
  }

  DEBUG ((DEBUG_ERROR, "%a: No Memory Type Information HOB found\n", __func__));

  return EFI_NOT_FOUND;
}

/**
 Look for Resource Descriptor HOB with a ResourceType of System Memory
 and an Owner GUID of gEfiMemoryTypeInformationGuid. If more than 1 is
 found, then return NULL.

  @param HobStart                         Pointer to the start of the HOB list.

  @return Non-NULL                        The pointer to the singular MemoryTypeInformation Resource Descriptor HOB.
  @return NULL                            No valid MemoryTypeInformation Resource Descriptor HOB found.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
EFIAPI
GetMemoryTypeInformationResourceHob (
  IN  VOID  **HobStart
  )
{
  UINTN                        Count;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *MemoryTypeInformationResourceHob;

  if (HobStart == NULL) {
    return NULL;
  }

  //
  // See if a Memory Type Information HOB is available
  //
  MemoryTypeInformationResourceHob = NULL;
  Count                            = 0;
  for (Hob.Raw = *HobStart; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    ResourceHob = Hob.ResourceDescriptor;
    if (!CompareGuid (&ResourceHob->Owner, &gEfiMemoryTypeInformationGuid)) {
      continue;
    }

    Count++;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }

    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    if (ResourceHob->ResourceLength >= CalculateTotalMemoryBinSizeNeeded ((UINTN)(ResourceHob->PhysicalStart + ResourceHob->ResourceLength))) {
      MemoryTypeInformationResourceHob = ResourceHob;
    }
  }

  if (Count > 1) {
    return NULL;
  }

  return MemoryTypeInformationResourceHob;
}

/**
  Sets the preferred memory range to use for the Memory Type Information bins.
  This service must be called before fist call to CoreAddMemoryDescriptor().

  If the location of the Memory Type Information bins has already been
  established or the size of the range provides is smaller than all the
  Memory Type Information bins, then the range provides is not used.

  @param  Start   The start address of the Memory Type Information range.
  @param  Length  The size, in bytes, of the Memory Type Information range.
**/
VOID
EFIAPI
CoreSetMemoryTypeInformationRange (
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINT64                Length
  )
{
  EFI_PHYSICAL_ADDRESS  Top;
  EFI_MEMORY_TYPE       Type;
  UINTN                 Index;
  UINT64                Size;

  //
  // Return if Memory Type Information bin locations have already been set
  //
  if (mMemoryTypeInformationInitialized) {
    DEBUG ((DEBUG_ERROR, "%a: Ignored. Bins already set.\n", __func__));
    return;
  }

  //
  // Return if size of the Memory Type Information bins is greater than Length
  //
  Size = CalculateTotalMemoryBinSizeNeeded ((UINTN)(Start + Length));

  if (Size > Length) {
    return;
  }

  //
  // Loop through each memory type in the order specified by the
  // gMemoryTypeInformation[] array
  //
  Top = Start + Length;
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(gMemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      mMemoryTypeStatistics[Type].MaximumAddress = Top - 1;
      Top                                       -= LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
      mMemoryTypeStatistics[Type].BaseAddress    = Top;

      //
      // If the current base address is the lowest address so far, then update
      // the default maximum address
      //
      if (mMemoryTypeStatistics[Type].BaseAddress < mDefaultMaximumAddress) {
        mDefaultMaximumAddress = mMemoryTypeStatistics[Type].BaseAddress - 1;
      }

      mMemoryTypeStatistics[Type].NumberOfPages   = EFI_SIZE_TO_PAGES ((UINTN)BinSize);
      gMemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  //
  // If the number of pages reserved for a memory type is 0, then all
  // allocations for that type should be in the default range.
  //
  for (Type = (EFI_MEMORY_TYPE)0; Type < EfiMaxMemoryType; Type++) {
    for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
      if (Type == (EFI_MEMORY_TYPE)gMemoryTypeInformation[Index].Type) {
        mMemoryTypeStatistics[Type].InformationIndex = Index;
      }
    }

    mMemoryTypeStatistics[Type].CurrentNumberOfPages = 0;
    if (mMemoryTypeStatistics[Type].MaximumAddress == MAX_ALLOC_ADDRESS) {
      mMemoryTypeStatistics[Type].MaximumAddress = mDefaultMaximumAddress;
    }
  }

  mMemoryTypeInformationInitialized = TRUE;
}

/**
  Allocate memory bins for each memory type as specified in gMemoryTypeInformation.

  If all the memory types cannot be allocated, then all previously allocated
  memory types are freed and the function returns. If this function fails, it will log and expect to be called
  again when more memory is added to the system.
**/
VOID
EFIAPI
AllocateMemoryTypeInformationBins (
  VOID
  )
{
  UINTN                 Index;
  EFI_MEMORY_TYPE       Type;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  EFI_PHYSICAL_ADDRESS  LastBinAddress;
  UINT64                RequiredSize;

  //
  // Check to see if the statistics for the different memory types have already been established
  //
  if (mMemoryTypeInformationInitialized) {
    return;
  }

  BaseAddress  = 0;
  RequiredSize = CalculateTotalMemoryBinSizeNeeded (0);
  if (RequiredSize == 0) {
    mMemoryTypeInformationInitialized = TRUE;
    return;
  }

  DEBUG ((DEBUG_ERROR, "%a: Attempting to allocate 0x%llx bytes for all memory bins\n", __func__, RequiredSize));

  // To ensure we get a contiguous range of memory for our bins, we will attempt to allocate
  // all of the memory needed in one go. If that works, we can then carve it up into the individual bins. We allocate
  // reserved pages to ensure runtime page allocation granularity is taken into account.
  BaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateReservedPages (
                                               (UINTN)RShiftU64 (RequiredSize, EFI_PAGE_SHIFT)
                                               );

  if (BaseAddress == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Could not allocate contiguous pages for all memory bins\n", __func__));
    ASSERT (FALSE);
    return;
  }

  LastBinAddress         = BaseAddress + RequiredSize;
  mDefaultMaximumAddress = BaseAddress - 1;

  //
  // Loop through each memory type in the order specified by the gMemoryTypeInformation[] array
  //
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(gMemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      mMemoryTypeStatistics[Type].BaseAddress    = LastBinAddress - LShiftU64 (gMemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
      mMemoryTypeStatistics[Type].MaximumAddress = LastBinAddress - 1;
      LastBinAddress                             = mMemoryTypeStatistics[Type].BaseAddress;
    }
  }

  //
  // There was enough system memory for all the the memory types were allocated.  So,
  // those memory areas can be freed for future allocations, and all future memory
  // allocations can occur within their respective bins
  //
  FreePages (
    (VOID *)(UINTN)BaseAddress,
    (UINTN)RShiftU64 (RequiredSize, EFI_PAGE_SHIFT)
    );
  for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(gMemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (gMemoryTypeInformation[Index].NumberOfPages != 0) {
      mMemoryTypeStatistics[Type].NumberOfPages   = gMemoryTypeInformation[Index].NumberOfPages;
      gMemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  //
  // If the number of pages reserved for a memory type is 0, then all allocations for that type
  // should be in the default range.
  //
  for (Type = (EFI_MEMORY_TYPE)0; Type < EfiMaxMemoryType; Type++) {
    for (Index = 0; gMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
      if (Type == (EFI_MEMORY_TYPE)gMemoryTypeInformation[Index].Type) {
        mMemoryTypeStatistics[Type].InformationIndex = Index;
      }
    }

    mMemoryTypeStatistics[Type].CurrentNumberOfPages = 0;
    if (mMemoryTypeStatistics[Type].MaximumAddress == MAX_ALLOC_ADDRESS) {
      mMemoryTypeStatistics[Type].MaximumAddress = mDefaultMaximumAddress;
    }
  }

  mMemoryTypeInformationInitialized = TRUE;
}

/**
  Update memory type statistics upon memory allocation and free.

  @param OldType          The original memory type of the memory region.
  @param NewType          The new memory type of the memory region.
  @param Start            The starting physical address of the memory region.
  @param NumberOfPages    The number of pages in the memory region.
  @param InBin            TRUE if the memory region is within the memory type bin, FALSE if outside the bin.
**/
VOID
EFIAPI
UpdateMemoryStatistics (
  IN EFI_MEMORY_TYPE       OldType,
  IN EFI_MEMORY_TYPE       NewType,
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINTN                 NumberOfPages
  )
{
  if (!mMemoryTypeInformationInitialized) {
    return;
  }

  //
  // Update counters for the number of pages allocated to each memory type
  //
  if ((UINT32)OldType < EfiMaxMemoryType) {
    if (((Start >= mMemoryTypeStatistics[OldType].BaseAddress) && (Start <= mMemoryTypeStatistics[OldType].MaximumAddress)) ||
        ((Start >= mDefaultBaseAddress) && (Start <= mDefaultMaximumAddress)))
    {
      if (NumberOfPages > mMemoryTypeStatistics[OldType].CurrentNumberOfPages) {
        mMemoryTypeStatistics[OldType].CurrentNumberOfPages = 0;
      } else {
        mMemoryTypeStatistics[OldType].CurrentNumberOfPages -= NumberOfPages;
      }
    }
  }

  if ((UINT32)NewType < EfiMaxMemoryType) {
    if (((Start >= mMemoryTypeStatistics[NewType].BaseAddress) && (Start <= mMemoryTypeStatistics[NewType].MaximumAddress)) ||
        ((Start >= mDefaultBaseAddress) && (Start <= mDefaultMaximumAddress)))
    {
      mMemoryTypeStatistics[NewType].CurrentNumberOfPages += NumberOfPages;
      if (mMemoryTypeStatistics[NewType].CurrentNumberOfPages > gMemoryTypeInformation[mMemoryTypeStatistics[NewType].InformationIndex].NumberOfPages) {
        gMemoryTypeInformation[mMemoryTypeStatistics[NewType].InformationIndex].NumberOfPages = (UINT32)mMemoryTypeStatistics[NewType].CurrentNumberOfPages;
      }
    }
  }
}
