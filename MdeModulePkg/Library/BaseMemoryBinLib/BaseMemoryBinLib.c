/** @file

  Shared logic between cores to work with memory bins for S4 resume stability.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiMultiPhase.h>
#include <Uefi/UefiSpec.h>
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

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address

  @return Status. On success, Memory is filled in with the base address allocated
  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in
                                 spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
CoreAllocatePages (
  IN EFI_ALLOCATE_TYPE         Type,
  IN EFI_MEMORY_TYPE           MemoryType,
  IN UINTN                     NumberOfPages,
  IN OUT EFI_PHYSICAL_ADDRESS  *Memory
  );

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed
  @param  NumberOfPages          The number of pages to free

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range
  @retval EFI_INVALID_PARAMETER  Address not aligned
  @return EFI_SUCCESS         -Pages successfully freed.

**/
EFI_STATUS
EFIAPI
CoreFreePages (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages
  );

/**
  Calculate total memory bin size needed.

  @param BinTop The top address of the memory bins. This is an optional parameter.
                If non-zero, alignment requirements will be considered in the calculation.
  @param MemoryTypeInformation The memory type information array.

  @return The total memory bin size needed.

**/
UINT64
CalculateTotalMemoryBinSizeNeeded (
  IN UINTN                        BinTop,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation
  )
{
  UINTN   Index;
  UINT64  TotalSize;
  UINTN   Granularity;

  Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;

  if (MemoryTypeInformation == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter(s)\n", __func__));
    ASSERT (FALSE);
    return 0;
  }

  //
  // Loop through each memory type in the order specified by the MemoryTypeInformation[] array
  //
  TotalSize = 0;
  for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    if ((MemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
        (MemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
        (MemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
        (MemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
    {
      Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
    }

    // MemoryTypeInformation[Index].NumberOfPages is already aligned to the allocation granularity
    TotalSize += LShiftU64 (MemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);

    // BinTop is optional
    if (BinTop == 0) {
      continue;
    }

    BinTop    -= (UINTN)LShiftU64 (MemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT);
    TotalSize += (BinTop & (Granularity - 1));
    BinTop    &= ~(Granularity - 1);
  }

  return TotalSize;
}

/**
  Get the Memory Type Information HOB if it exists and populate gMemoryTypeInformation.

  @param  MemoryTypeInformation           The pointer to the memory type information array to be populated.

  @return EFI_STATUS                      On EFI_SUCCESS, gMemoryTypeInformation points to the
                                          Memory Type Information.
  @return EFI_NOT_FOUND                   No valid Memory Type Information HOB found.
**/
EFI_STATUS
EFIAPI
PopulateMemoryTypeInformation (
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation
  )
{
  UINTN                        DataSize;
  EFI_MEMORY_TYPE_INFORMATION  *EfiMemoryTypeInformation;
  EFI_HOB_GUID_TYPE            *GuidHob;
  UINTN                        Index;
  UINT32                       Granularity;

  if (MemoryTypeInformation == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  GuidHob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);
  if (GuidHob != NULL) {
    EfiMemoryTypeInformation = GET_GUID_HOB_DATA (GuidHob);
    DataSize                 = GET_GUID_HOB_DATA_SIZE (GuidHob);
    if ((EfiMemoryTypeInformation != NULL) && (DataSize > 0) && (DataSize <= (EfiMaxMemoryType + 1) * sizeof (EFI_MEMORY_TYPE_INFORMATION))) {
      CopyMem (MemoryTypeInformation, EfiMemoryTypeInformation, DataSize);

      for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
        //
        // Make sure the memory type in the MemoryTypeInformation[] array is valid
        //
        if (MemoryTypeInformation[Index].Type > EfiMaxMemoryType) {
          continue;
        }

        if (MemoryTypeInformation[Index].NumberOfPages != 0) {
          if ((MemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
              (MemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
              (MemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
              (MemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
          {
            Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
          } else {
            Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
          }

          // Align the number of pages to the allocation granularity
          MemoryTypeInformation[Index].NumberOfPages = (UINT32)RShiftU64 (ALIGN_VALUE (LShiftU64 (MemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT), Granularity), EFI_PAGE_SHIFT);
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
  @param MemoryTypeInformation            The memory type information array to be used to determine
                                          the size of the memory bins.

  @return Non-NULL                        The pointer to the singular MemoryTypeInformation Resource Descriptor HOB.
  @return NULL                            No valid MemoryTypeInformation Resource Descriptor HOB found.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
EFIAPI
GetMemoryTypeInformationResourceHob (
  IN  VOID                        **HobStart,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation
  )
{
  UINTN                        Count;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *MemoryTypeInformationResourceHob;

  if ((HobStart == NULL) || (MemoryTypeInformation == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter(s)\n", __func__));
    ASSERT (FALSE);
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

    if (ResourceHob->ResourceLength >= CalculateTotalMemoryBinSizeNeeded ((UINTN)(ResourceHob->PhysicalStart + ResourceHob->ResourceLength), MemoryTypeInformation)) {
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

  @param  Start                             The start address of the Memory Type Information range.
  @param  Length                            The size, in bytes, of the Memory Type Information range.
  @param  MemoryTypeInformation             The memory type information array to be used to determine
                                            the size of the memory bins.
  @param  MemoryTypeInformationInitialized  A pointer to a boolean that indicates whether the memory type
                                            information bins have been initialized.
  @param  MemoryTypeStatistics              The memory type statistics array to be updated with the memory bin
                                            information if the provided range is used.
  @param  DefaultMaximumAddress             A pointer to the default maximum address to be updated if the
                                            provided range is used.
**/
VOID
EFIAPI
CoreSetMemoryTypeInformationRange (
  IN EFI_PHYSICAL_ADDRESS         Start,
  IN UINT64                       Length,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation,
  IN BOOLEAN                      *MemoryTypeInformationInitialized,
  IN EFI_MEMORY_TYPE_STATISTICS   *MemoryTypeStatistics,
  IN EFI_PHYSICAL_ADDRESS         *DefaultMaximumAddress
  )
{
  EFI_PHYSICAL_ADDRESS  Top;
  EFI_MEMORY_TYPE       Type;
  UINTN                 Index;
  UINT64                Size;
  UINT64                Alignment;
  UINT64                BinSize;

  if ((MemoryTypeInformation == NULL) ||
      (MemoryTypeInformationInitialized == NULL) ||
      (MemoryTypeStatistics == NULL) ||
      (DefaultMaximumAddress == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter(s)\n", __func__));
    ASSERT (FALSE);
    return;
  }

  //
  // Return if Memory Type Information bin locations have already been set
  //
  if (*MemoryTypeInformationInitialized) {
    DEBUG ((DEBUG_ERROR, "%a: Ignored. Bins already set.\n", __func__));
    return;
  }

  //
  // Return if size of the Memory Type Information bins is greater than Length
  //
  Top  = Start + Length;
  Size = 0;
  for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(MemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (MemoryTypeInformation[Index].NumberOfPages != 0) {
      Alignment = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
      if ((MemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
          (MemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
          (MemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
          (MemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
      {
        Alignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      }

      BinSize = EFI_PAGES_TO_SIZE ((UINTN)MemoryTypeInformation[Index].NumberOfPages);
      BinSize = ALIGN_VALUE (BinSize, Alignment);

      Size += BinSize;
      if (Size > Length) {
        return;
      }

      Top -= BinSize;

      Size += (Top & (Alignment - 1));
      if (Size > Length) {
        return;
      }

      Top &= ~(Alignment - 1);
    }
  }

  if (Size > Length) {
    return;
  }

  //
  // Loop through each memory type in the order specified by the
  // gMemoryTypeInformation[] array
  //
  Top = Start + Length;
  for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the MemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(MemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (MemoryTypeInformation[Index].NumberOfPages != 0) {
      Alignment = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
      if ((MemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
          (MemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
          (MemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
          (MemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
      {
        Alignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      }

      BinSize = EFI_PAGES_TO_SIZE ((UINTN)MemoryTypeInformation[Index].NumberOfPages);
      BinSize = ALIGN_VALUE (BinSize, Alignment);

      Top = (Top - BinSize) & ~(Alignment - 1);

      MemoryTypeStatistics[Type].BaseAddress    = Top;
      MemoryTypeStatistics[Type].MaximumAddress = Top + BinSize - 1;

      //
      // If the current base address is the lowest address so far, then update
      // the default maximum address
      //
      if (MemoryTypeStatistics[Type].BaseAddress < *DefaultMaximumAddress) {
        *DefaultMaximumAddress = MemoryTypeStatistics[Type].BaseAddress - 1;
      }

      MemoryTypeStatistics[Type].NumberOfPages   = EFI_SIZE_TO_PAGES ((UINTN)BinSize);
      MemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  //
  // If the number of pages reserved for a memory type is 0, then all
  // allocations for that type should be in the default range.
  //
  for (Type = (EFI_MEMORY_TYPE)0; Type < EfiMaxMemoryType; Type++) {
    for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
      if (Type == (EFI_MEMORY_TYPE)MemoryTypeInformation[Index].Type) {
        MemoryTypeStatistics[Type].InformationIndex = Index;
      }
    }

    MemoryTypeStatistics[Type].CurrentNumberOfPages = 0;
    if (MemoryTypeStatistics[Type].MaximumAddress == MAX_ALLOC_ADDRESS) {
      MemoryTypeStatistics[Type].MaximumAddress = *DefaultMaximumAddress;
    }
  }

  *MemoryTypeInformationInitialized = TRUE;
}

/**
  Allocate memory bins for each memory type as specified in gMemoryTypeInformation.

  If all the memory types cannot be allocated, then all previously allocated
  memory types are freed and the function returns. If this function fails, it will log and expect to be called
  again when more memory is added to the system.

  @param  MemoryTypeInformationInitialized  A pointer to a boolean that indicates whether the memory type
                                            information bins have been initialized.
  @param  MemoryTypeInformation             The memory type information array to be used to determine
                                            the size of the memory bins.
  @param  MemoryTypeStatistics              The memory type statistics array to be updated with the memory bin
                                            information if the provided range is used.
  @param  DefaultMaximumAddress             A pointer to the default maximum address to be updated if the
                                            provided range is used.
**/
VOID
EFIAPI
AllocateMemoryTypeInformationBins (
  IN BOOLEAN                      *MemoryTypeInformationInitialized,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation,
  IN EFI_MEMORY_TYPE_STATISTICS   *MemoryTypeStatistics,
  IN EFI_PHYSICAL_ADDRESS         *DefaultMaximumAddress
  )
{
  EFI_STATUS       Status;
  UINTN            Index;
  UINTN            FreeIndex;
  UINT64           Alignment;
  UINT64           BinSize;
  EFI_MEMORY_TYPE  Type;

  if ((MemoryTypeInformationInitialized == NULL) ||
      (MemoryTypeInformation == NULL) ||
      (MemoryTypeStatistics == NULL) ||
      (DefaultMaximumAddress == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter(s)\n", __func__));
    ASSERT (FALSE);
    return;
  }

  //
  // Check to see if the statistics for the different memory types have already been established
  //
  if (*MemoryTypeInformationInitialized) {
    return;
  }

  //
  // Loop through each memory type in the order specified by the gMemoryTypeInformation[] array
  //
  for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the gMemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(MemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (MemoryTypeInformation[Index].NumberOfPages != 0) {
      Alignment = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
      if ((MemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
          (MemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
          (MemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
          (MemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
      {
        Alignment = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
      }

      BinSize = EFI_PAGES_TO_SIZE ((UINTN)MemoryTypeInformation[Index].NumberOfPages);
      BinSize = ALIGN_VALUE (BinSize, Alignment);

      MemoryTypeInformation[Index].NumberOfPages = (UINT32)EFI_SIZE_TO_PAGES ((UINTN)BinSize);

      //
      // Allocate pages for the current memory type from the top of available memory
      //
      Status = CoreAllocatePages (
                 AllocateAnyPages,
                 Type,
                 MemoryTypeInformation[Index].NumberOfPages,
                 &MemoryTypeStatistics[Type].BaseAddress
                 );
      if (EFI_ERROR (Status)) {
        //
        // If an error occurs allocating the pages for the current memory type, then
        // free all the pages allocates for the previous memory types and return.  This
        // operation with be retied when/if more memory is added to the system
        //
        for (FreeIndex = 0; FreeIndex < Index; FreeIndex++) {
          //
          // Make sure the memory type in the gMemoryTypeInformation[] array is valid
          //
          Type = (EFI_MEMORY_TYPE)(MemoryTypeInformation[FreeIndex].Type);
          if ((UINT32)Type > EfiMaxMemoryType) {
            continue;
          }

          if (MemoryTypeInformation[FreeIndex].NumberOfPages != 0) {
            CoreFreePages (
              MemoryTypeStatistics[Type].BaseAddress,
              MemoryTypeInformation[FreeIndex].NumberOfPages
              );
            MemoryTypeStatistics[Type].BaseAddress    = 0;
            MemoryTypeStatistics[Type].MaximumAddress = MAX_ALLOC_ADDRESS;
          }
        }

        return;
      }

      //
      // Compute the address at the top of the current statistics
      //
      MemoryTypeStatistics[Type].MaximumAddress =
        MemoryTypeStatistics[Type].BaseAddress +
        LShiftU64 (MemoryTypeInformation[Index].NumberOfPages, EFI_PAGE_SHIFT) - 1;

      //
      // If the current base address is the lowest address so far, then update the default
      // maximum address
      //
      if (MemoryTypeStatistics[Type].BaseAddress < *DefaultMaximumAddress) {
        *DefaultMaximumAddress = MemoryTypeStatistics[Type].BaseAddress - 1;
      }
    }
  }

  //
  // There was enough system memory for all the the memory types were allocated.  So,
  // those memory areas can be freed for future allocations, and all future memory
  // allocations can occur within their respective bins
  //
  for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the MemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(MemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (MemoryTypeInformation[Index].NumberOfPages != 0) {
      CoreFreePages (
        MemoryTypeStatistics[Type].BaseAddress,
        MemoryTypeInformation[Index].NumberOfPages
        );
      MemoryTypeStatistics[Type].NumberOfPages   = MemoryTypeInformation[Index].NumberOfPages;
      MemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  //
  // If the number of pages reserved for a memory type is 0, then all allocations for that type
  // should be in the default range.
  //
  for (Type = (EFI_MEMORY_TYPE)0; Type < EfiMaxMemoryType; Type++) {
    for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
      if (Type == (EFI_MEMORY_TYPE)MemoryTypeInformation[Index].Type) {
        MemoryTypeStatistics[Type].InformationIndex = Index;
      }
    }

    MemoryTypeStatistics[Type].CurrentNumberOfPages = 0;
    if (MemoryTypeStatistics[Type].MaximumAddress == MAX_ALLOC_ADDRESS) {
      MemoryTypeStatistics[Type].MaximumAddress = *DefaultMaximumAddress;
    }
  }

  *MemoryTypeInformationInitialized = TRUE;
}

/**
  Update memory type statistics upon memory allocation and free.

  @param OldType                          The original memory type of the memory region.
  @param NewType                          The new memory type of the memory region.
  @param Start                            The starting physical address of the memory region.
  @param NumberOfPages                    The number of pages in the memory region.
  @param InBin                            TRUE if the memory region is within the memory type bin, FALSE if
                                          outside the bin.
  @param MemoryTypeInformationInitialized A pointer to a boolean that indicates whether the memory type
                                          information bins have been initialized.
  @param MemoryTypeStatistics             The memory type statistics array to be updated.
  @param MemoryTypeInformation            The memory type information array to be updated.
  @param DefaultBaseAddress               Default bin base address.
  @param DefaultMaximumAddress            Default bin maximum address.
**/
VOID
EFIAPI
UpdateMemoryStatistics (
  IN EFI_MEMORY_TYPE              OldType,
  IN EFI_MEMORY_TYPE              NewType,
  IN EFI_PHYSICAL_ADDRESS         Start,
  IN UINTN                        NumberOfPages,
  IN BOOLEAN                      *MemoryTypeInformationInitialized,
  IN EFI_MEMORY_TYPE_STATISTICS   *MemoryTypeStatistics,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation,
  IN EFI_PHYSICAL_ADDRESS         DefaultBaseAddress,
  IN EFI_PHYSICAL_ADDRESS         DefaultMaximumAddress
  )
{
  if ((MemoryTypeInformationInitialized == NULL) ||
      (MemoryTypeStatistics == NULL) ||
      (MemoryTypeInformation == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameter(s)\n", __func__));
    ASSERT (FALSE);
    return;
  }

  if (!*MemoryTypeInformationInitialized) {
    return;
  }

  //
  // Update counters for the number of pages allocated to each memory type
  //
  if ((UINT32)OldType < EfiMaxMemoryType) {
    if (((Start >= MemoryTypeStatistics[OldType].BaseAddress) && (Start <= MemoryTypeStatistics[OldType].MaximumAddress)) ||
        ((Start >= DefaultBaseAddress) && (Start <= DefaultMaximumAddress)))
    {
      if (NumberOfPages > MemoryTypeStatistics[OldType].CurrentNumberOfPages) {
        MemoryTypeStatistics[OldType].CurrentNumberOfPages = 0;
      } else {
        MemoryTypeStatistics[OldType].CurrentNumberOfPages -= NumberOfPages;
      }
    }
  }

  if ((UINT32)NewType < EfiMaxMemoryType) {
    if (((Start >= MemoryTypeStatistics[NewType].BaseAddress) && (Start <= MemoryTypeStatistics[NewType].MaximumAddress)) ||
        ((Start >= DefaultBaseAddress) && (Start <= DefaultMaximumAddress)))
    {
      MemoryTypeStatistics[NewType].CurrentNumberOfPages += NumberOfPages;
      if ((MemoryTypeStatistics[NewType].InformationIndex < (UINTN)EfiMaxMemoryType) &&
          (MemoryTypeStatistics[NewType].CurrentNumberOfPages > MemoryTypeInformation[MemoryTypeStatistics[NewType].InformationIndex].NumberOfPages))
      {
        MemoryTypeInformation[MemoryTypeStatistics[NewType].InformationIndex].NumberOfPages = (UINT32)MemoryTypeStatistics[NewType].CurrentNumberOfPages;
      }
    }
  }
}
