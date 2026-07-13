/** @file

  Shared logic between cores to work with memory bins for S4 resume stability. This file is duplicated in PEI Core and
  DXE Core until a BaseTools feature comes online to support recommended library instances. Any changes to this file
  must also be made in MdeModulePkg/Core/Pei/Memory/MemoryBin.c.

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

#include <MemoryBin.h>

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
  Calculate total memory bin size needed.

  @param BinTop                The top address of the memory bins. This is an optional parameter.
                               When NULL, the returned size meets the alignment requirements as long as
                               the base address selected also meets the alignment requirements. When
                               non-NULL, then the returned BinTop value and the returned size both meet
                               the alignment requirements. When non-NULL, this will be updated on
                               output to the new top address of the memory bins that must be used to
                               satisfy alignment requirements.
  @param MemoryTypeInformation The memory type information array.

  @return The total memory bin size needed.

**/
UINT64
CalculateTotalMemoryBinSizeNeeded (
  IN OUT OPTIONAL EFI_PHYSICAL_ADDRESS  *BinTop,
  IN EFI_MEMORY_TYPE_INFORMATION        *MemoryTypeInformation
  )
{
  UINTN   Index;
  UINT64  TotalSize;
  UINT64  Granularity;

  ASSERT (MemoryTypeInformation != NULL);
  if (MemoryTypeInformation == NULL) {
    return 0;
  }

  //
  // Loop through each memory type in the order specified by the MemoryTypeInformation[] array
  //
  TotalSize = 0;
  for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    Granularity = DEFAULT_PAGE_ALLOCATION_GRANULARITY;
    if ((MemoryTypeInformation[Index].Type == EfiReservedMemoryType) ||
        (MemoryTypeInformation[Index].Type == EfiACPIMemoryNVS) ||
        (MemoryTypeInformation[Index].Type == EfiRuntimeServicesCode) ||
        (MemoryTypeInformation[Index].Type == EfiRuntimeServicesData))
    {
      Granularity = RUNTIME_PAGE_ALLOCATION_GRANULARITY;
    }

    // MemoryTypeInformation[Index].NumberOfPages is already aligned to the allocation granularity
    TotalSize += EFI_PAGES_TO_SIZE ((UINTN)MemoryTypeInformation[Index].NumberOfPages);

    // BinTop is optional
    if (BinTop == NULL) {
      continue;
    }

    // Lower the bin top to the next aligned address, taking any padding into account in the size
    *BinTop   -= EFI_PAGES_TO_SIZE ((UINTN)MemoryTypeInformation[Index].NumberOfPages);
    TotalSize += (*BinTop & (Granularity - 1));
    *BinTop   &= ~(Granularity - 1);
  }

  if (BinTop != NULL) {
    // Set *BinTop to the new top of the memory bins. It currently points to the base address of
    // the memory bins
    *BinTop += TotalSize;
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
  UINTN                        MaxIndex;

  ASSERT (MemoryTypeInformation != NULL);
  if (MemoryTypeInformation == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  GuidHob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);
  if (GuidHob != NULL) {
    EfiMemoryTypeInformation = GET_GUID_HOB_DATA (GuidHob);
    DataSize                 = GET_GUID_HOB_DATA_SIZE (GuidHob);

    if ((EfiMemoryTypeInformation != NULL) && (DataSize > 0) && (DataSize <= (EfiMaxMemoryType + 1) * sizeof (EFI_MEMORY_TYPE_INFORMATION))) {
      CopyMem (MemoryTypeInformation, EfiMemoryTypeInformation, DataSize);
      MaxIndex = (DataSize / sizeof (EFI_MEMORY_TYPE_INFORMATION)) - 1;

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
          MemoryTypeInformation[Index].NumberOfPages = (UINT32)EFI_SIZE_TO_PAGES (ALIGN_VALUE (EFI_PAGES_TO_SIZE ((UINTN)MemoryTypeInformation[Index].NumberOfPages), Granularity));
        }

        // It is guaranteed that DataSize must be > 0 and <= (EfiMaxMemoryType + 1) * sizeof (EFI_MEMORY_TYPE_INFORMATION)
        // however, we may have a corrupted HOB that does end in the EfiMaxMemoryType, so we need to terminate the loop
        // to not overrun the array. Because we can't trust the HOB data, we will reset it to 0.
        if (Index == MaxIndex) {
          DEBUG ((DEBUG_WARN, "%a: Corrupted Memory Type Information HOB data\n", __func__));
          goto CleanAndError;
        }
      }

      return EFI_SUCCESS;
    }

    DEBUG ((DEBUG_WARN, "%a: Invalid Memory Type Information HOB data\n", __func__));
  }

CleanAndError:
  // We may have gotten here from a corrupted HOB, ensure all data is set back
  // to disabled bins.
  for (Index = 0; Index <= EfiMaxMemoryType; Index++) {
    MemoryTypeInformation[Index].Type          = (UINT32)Index;
    MemoryTypeInformation[Index].NumberOfPages = 0;
  }

  DEBUG ((DEBUG_WARN, "%a: No Memory Type Information HOB found, S4 resume is likely to fail\n", __func__));

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
  EFI_PHYSICAL_ADDRESS         BinTop;

  ASSERT (HobStart != NULL);
  ASSERT (MemoryTypeInformation != NULL);
  if ((HobStart == NULL) || (MemoryTypeInformation == NULL)) {
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

    BinTop = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
    if (ResourceHob->ResourceLength >= CalculateTotalMemoryBinSizeNeeded (&BinTop, MemoryTypeInformation)) {
      MemoryTypeInformationResourceHob = ResourceHob;
    }
  }

  if (Count > 1) {
    return NULL;
  }

  return MemoryTypeInformationResourceHob;
}

/**
  Helper function to set up the bin statistics with the provided bin range

  @param  MemoryTypeInformation             The memory type information array to be used to determine
                                            the size of the memory bins.
  @param  MemoryTypeStatistics              The memory type statistics to be updated with the memory bin
                                            information if the provided range is used.
  @param  DefaultMaximumAddress             A pointer to the default maximum address to be updated if the
                                            provided range is used.
**/
STATIC
VOID
InitializeBinStatisticsFromRange (
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation,
  IN EFI_MEMORY_TYPE_STATISTICS   *MemoryTypeStatistics,
  IN EFI_PHYSICAL_ADDRESS         *DefaultMaximumAddress
  )
{
  EFI_MEMORY_TYPE  Type;
  UINTN            Index;

  ASSERT (MemoryTypeInformation != NULL);
  ASSERT (MemoryTypeStatistics != NULL);
  ASSERT (DefaultMaximumAddress != NULL);

  if ((MemoryTypeInformation == NULL) || (MemoryTypeStatistics == NULL) || (DefaultMaximumAddress == NULL)) {
    return;
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
}

/**
  Sets the preferred memory range to use for the Memory Type Information bins.
  This service must be called before first call to CoreAddMemoryDescriptor().

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

  ASSERT (MemoryTypeInformation != NULL);
  ASSERT (MemoryTypeInformationInitialized != NULL);
  ASSERT (MemoryTypeStatistics != NULL);
  ASSERT (DefaultMaximumAddress != NULL);

  if ((MemoryTypeInformation == NULL) ||
      (MemoryTypeInformationInitialized == NULL) ||
      (MemoryTypeStatistics == NULL) ||
      (DefaultMaximumAddress == NULL))
  {
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
  Size = CalculateTotalMemoryBinSizeNeeded (&Top, MemoryTypeInformation);

  if (Size > Length) {
    return;
  }

  //
  // Loop through each memory type in the order specified by the
  // gMemoryTypeInformation[] array
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
      MemoryTypeStatistics[Type].MaximumAddress = Top - 1;
      Top                                      -= EFI_PAGES_TO_SIZE ((UINTN)MemoryTypeInformation[Index].NumberOfPages);
      MemoryTypeStatistics[Type].BaseAddress    = Top;

      //
      // If the current base address is the lowest address so far, then update
      // the default maximum address
      //
      if (MemoryTypeStatistics[Type].BaseAddress < *DefaultMaximumAddress) {
        *DefaultMaximumAddress = MemoryTypeStatistics[Type].BaseAddress - 1;
      }

      MemoryTypeStatistics[Type].NumberOfPages   = MemoryTypeInformation[Index].NumberOfPages;
      MemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  InitializeBinStatisticsFromRange (MemoryTypeInformation, MemoryTypeStatistics, DefaultMaximumAddress);

  DEBUG ((
    DEBUG_INFO,
    "%a: Inherited range 0x%llx - 0x%llx for memory bins\n",
    __func__,
    Start,
    Start + Length -1
    ));
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
  @param  CreateHob                         TRUE to create Memory Type Information Resource HOB after successful
                                            allocation. This is used for PEI Core to report the bins to DXE Core.
                                            DXE Core must set this to FALSE because HOB creation is not supported in
                                            DXE (nor is the information required to be passed to another entity).
**/
VOID
EFIAPI
AllocateMemoryTypeInformationBins (
  IN BOOLEAN                      *MemoryTypeInformationInitialized,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation,
  IN EFI_MEMORY_TYPE_STATISTICS   *MemoryTypeStatistics,
  IN EFI_PHYSICAL_ADDRESS         *DefaultMaximumAddress,
  IN BOOLEAN                      CreateHob
  )
{
  UINTN                 Index;
  EFI_MEMORY_TYPE       Type;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  EFI_PHYSICAL_ADDRESS  LastBinAddress;
  UINT64                RequiredSize;

  ASSERT (MemoryTypeInformationInitialized != NULL);
  ASSERT (MemoryTypeInformation != NULL);
  ASSERT (MemoryTypeStatistics != NULL);
  ASSERT (DefaultMaximumAddress != NULL);

  if ((MemoryTypeInformationInitialized == NULL) ||
      (MemoryTypeInformation == NULL) ||
      (MemoryTypeStatistics == NULL) ||
      (DefaultMaximumAddress == NULL))
  {
    return;
  }

  //
  // Check to see if the statistics for the different memory types have already been established
  //
  if (*MemoryTypeInformationInitialized) {
    return;
  }

  BaseAddress  = 0;
  RequiredSize = CalculateTotalMemoryBinSizeNeeded (NULL, MemoryTypeInformation);
  if (RequiredSize == 0) {
    *MemoryTypeInformationInitialized = TRUE;
    return;
  }

  // To ensure we get a contiguous range of memory for our bins, we will attempt to allocate
  // all of the memory needed in one go. If that works, we can then carve it up into the individual bins.
  // Our size is already aligned to the correct granularity, allocate aligned pages to ensure the base address is
  // aligned.
  BaseAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocateAlignedPages (
                                               EFI_SIZE_TO_PAGES ((UINTN)RequiredSize),
                                               RUNTIME_PAGE_ALLOCATION_GRANULARITY
                                               );

  if (BaseAddress == 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a: Could not allocate contiguous pages for all memory bins. It will be attempted again when more memory is added.\n",
      __func__
      ));
    return;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a: Allocated 0x%llx - 0x%llx for memory bins\n",
    __func__,
    BaseAddress,
    BaseAddress + RequiredSize - 1
    ));

  LastBinAddress         = BaseAddress + RequiredSize;
  *DefaultMaximumAddress = BaseAddress - 1;

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
      MemoryTypeStatistics[Type].BaseAddress    = LastBinAddress - EFI_PAGES_TO_SIZE (MemoryTypeInformation[Index].NumberOfPages);
      MemoryTypeStatistics[Type].MaximumAddress = LastBinAddress - 1;
      LastBinAddress                            = MemoryTypeStatistics[Type].BaseAddress;
    }
  }

  //
  // There was enough system memory for all the the memory types were allocated.  So,
  // those memory areas can be freed for future allocations, and all future memory
  // allocations can occur within their respective bins
  //
  FreeAlignedPages (
    (VOID *)(UINTN)BaseAddress,
    EFI_SIZE_TO_PAGES ((UINTN)RequiredSize)
    );
  for (Index = 0; MemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {
    //
    // Make sure the memory type in the MemoryTypeInformation[] array is valid
    //
    Type = (EFI_MEMORY_TYPE)(MemoryTypeInformation[Index].Type);
    if ((UINT32)Type > EfiMaxMemoryType) {
      continue;
    }

    if (MemoryTypeInformation[Index].NumberOfPages != 0) {
      MemoryTypeStatistics[Type].NumberOfPages   = MemoryTypeInformation[Index].NumberOfPages;
      MemoryTypeInformation[Index].NumberOfPages = 0;
    }
  }

  InitializeBinStatisticsFromRange (MemoryTypeInformation, MemoryTypeStatistics, DefaultMaximumAddress);

  if (CreateHob) {
    //
    // Create a Resource Descriptor HOB to report the Memory Type Information bins to DXE Core
    //
    BuildResourceDescriptorWithOwnerHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      TESTED_MEMORY_ATTRIBUTES,
      BaseAddress,
      RequiredSize,
      &gEfiMemoryTypeInformationGuid
      );
  }

  *MemoryTypeInformationInitialized = TRUE;
}

/**
  Update memory type statistics upon memory allocation and free.

  @param OldType                          The original memory type of the memory region.
  @param NewType                          The new memory type of the memory region.
  @param Start                            The starting physical address of the memory region.
  @param NumberOfPages                    The number of pages in the memory region.
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
  ASSERT (MemoryTypeInformationInitialized != NULL);
  ASSERT (MemoryTypeStatistics != NULL);
  ASSERT (MemoryTypeInformation != NULL);

  if ((MemoryTypeInformationInitialized == NULL) ||
      (MemoryTypeStatistics == NULL) ||
      (MemoryTypeInformation == NULL))
  {
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
