/** @file
  Shared logic between cores to work with memory bins for S4 resume stability.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MEMORY_BIN_LIB_
#define MEMORY_BIN_LIB_

#include <Guid/MemoryTypeInformation.h>

/**
  Calculate total memory bin size needed.

  @param BinTop The top address of the memory bins. This is an optional parameter.
                If non-zero, alignment requirements will be considered in the calculation.

  @return The total memory bin size needed.

**/
UINT64
CalculateTotalMemoryBinSizeNeeded (
  UINTN  BinTop
  );

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
  );

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
  );

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
  );

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
  );

/**
  Update memory type statistics upon memory allocation and free.

  @param OldType          The original memory type of the memory region.
  @param NewType          The new memory type of the memory region.
  @param Start            The starting physical address of the memory region.
  @param NumberOfPages    The number of pages in the memory region.
**/
VOID
EFIAPI
UpdateMemoryStatistics (
  IN EFI_MEMORY_TYPE       OldType,
  IN EFI_MEMORY_TYPE       NewType,
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINTN                 NumberOfPages
  );

extern EFI_MEMORY_TYPE_INFORMATION        gMemoryTypeInformation[EfiMaxMemoryType + 1];
extern EFI_MEMORY_TYPE_STATISTICS_HEADER  mMemoryTypeStatistics;
extern BOOLEAN                            mMemoryTypeInformationInitialized;
extern EFI_PHYSICAL_ADDRESS               mDefaultMaximumAddress;
extern EFI_PHYSICAL_ADDRESS               mDefaultBaseAddress;
#endif // MEMORY_BIN_LIB_
