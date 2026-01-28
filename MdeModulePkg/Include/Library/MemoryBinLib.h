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
  @param MemoryTypeInformation The memory type information array.

  @return The total memory bin size needed.

**/
UINT64
CalculateTotalMemoryBinSizeNeeded (
  IN UINTN                        BinTop,
  IN EFI_MEMORY_TYPE_INFORMATION  *MemoryTypeInformation
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif // MEMORY_BIN_LIB_
