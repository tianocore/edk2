/** @file
  Platform specific HOB producer Library implementation for Standalone MM Core.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>

#include <Guid/SmmProfileDataHob.h>
#include <Guid/SmramMemoryReserve.h>

typedef struct {
  EFI_PHYSICAL_ADDRESS    Base;
  UINT64                  Length;
} MM_PLATFORM_MEMORY_REGION;

/**
  Initialize the EFI_HOB_GENERIC_HEADER based on input Hob.

  @param[in] Hob          Pointer to the new HOB buffer.
  @param[in] HobType      Type of the new HOB.
  @param[in] HobLength    Length of the new HOB to allocate.

**/
VOID
MmCreateHob (
  IN VOID    *Hob,
  IN UINT16  HobType,
  IN UINT16  HobLength
  )
{
  //
  // Check Length to avoid data overflow.
  //
  ASSERT (HobLength < MAX_UINT16 - 0x7);

  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobType   = HobType;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobLength = HobLength;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->Reserved  = 0;
}

/**
  Builds a EFI_HOB_TYPE_RESOURCE_DESCRIPTOR HOB.

  @param[in] Hob                 Pointer to the new HOB buffer.
  @param[in] ResourceType        The type of resource described by this HOB.
  @param[in] ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param[in] PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param[in] NumberOfBytes       The length of the memory described by this HOB in bytes.

**/
VOID
MmBuildResourceDescriptorHob (
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *Hob,
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  )
{
  ASSERT (Hob != NULL);
  MmCreateHob (Hob, EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, sizeof (EFI_HOB_RESOURCE_DESCRIPTOR));

  Hob->ResourceType      = ResourceType;
  Hob->ResourceAttribute = ResourceAttribute;
  Hob->PhysicalStart     = PhysicalStart;
  Hob->ResourceLength    = NumberOfBytes;
}

/**
  Calculate the maximum support address.

  @return the maximum support address.
**/
static
UINT8
MmCalculateMaximumSupportAddress (
  VOID
  )
{
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;
  VOID    *Hob;

  //
  // Get physical address bits supported.
  //
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *)Hob)->SizeOfMemorySpace;
  } else {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      PhysicalAddressBits = (UINT8)RegEax;
    } else {
      PhysicalAddressBits = 36;
    }
  }

  return PhysicalAddressBits;
}

/**
  Builds resource HOB list for all MMIO range.

  This function treats all all ranges outside the system memory range and smram range
  as mmio and builds resource HOB list for all MMIO range.

  @param[in] Create        The type of resource described by this HOB.
  @param[in] MemoryRegion  MM_PLATFORM_MEMORY_REGION that describes all system memory range.
  @param[in] Count         Number of MM_PLATFORM_MEMORY_REGION.
  @param[in] Buffer        The pointer of new HOB buffer.

  @return The expected/used buffer size.
**/
UINTN
MmBuildHobForMmio (
  IN BOOLEAN                    Create,
  IN MM_PLATFORM_MEMORY_REGION  *MemoryRegion,
  IN UINTN                      Count,
  IN UINT8                      *Buffer
  )
{
  UINT64  PreviousAddress;
  UINT64  Base;
  UINT64  Limit;
  UINT8   PhysicalAddressBits;
  UINTN   Index;
  UINTN   BufferSize;

  Index               = 0;
  PreviousAddress     = 0;
  PhysicalAddressBits = MmCalculateMaximumSupportAddress ();
  Limit               = LShiftU64 (1, PhysicalAddressBits);
  BufferSize          = 0;

  for (Index = 0; Index <= Count; Index++) {
    //
    // When Index is equal to Count, Base covers the very last region.
    //
    Base = (Index == Count) ? Limit : MemoryRegion[Index].Base;
    if (Base > PreviousAddress) {
      if (Create) {
        MmBuildResourceDescriptorHob (
          (EFI_HOB_RESOURCE_DESCRIPTOR *)(Buffer + BufferSize),
          EFI_RESOURCE_MEMORY_MAPPED_IO,
          0,
          PreviousAddress,
          Base - PreviousAddress
          );
      }

      BufferSize += sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
    }

    if (Index < Count) {
      PreviousAddress = MemoryRegion[Index].Base + MemoryRegion[Index].Length;
    }
  }

  return BufferSize;
}

/**
  Function to compare 2 MM_PLATFORM_MEMORY_REGION pointer based on Base.

  @param[in] Buffer1  pointer to MP_INFORMATION2_HOB_DATA poiner to compare
  @param[in] Buffer2  pointer to second MP_INFORMATION2_HOB_DATA pointer to compare

  @retval 0   Buffer1 equal to Buffer2
  @retval <0  Buffer1 is less than Buffer2
  @retval >0  Buffer1 is greater than Buffer2
**/
static
INTN
EFIAPI
MmMemoryDescriptorCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  )
{
  if (((MM_PLATFORM_MEMORY_REGION *)Buffer1)->Base > ((MM_PLATFORM_MEMORY_REGION *)Buffer2)->Base) {
    return 1;
  } else if (((MM_PLATFORM_MEMORY_REGION *)Buffer1)->Base < ((MM_PLATFORM_MEMORY_REGION *)Buffer2)->Base) {
    return -1;
  }

  return 0;
}

/**
  Create the platform specific HOB list which StandaloneMm Core needed.

  This function build the platform specific HOB list needed by StandaloneMm Core
  based on the PEI HOB list.

  @param[in]      Buffer            The free buffer to be used for HOB creation.
  @param[in, out] BufferSize        The buffer size.
                                    On return, the expected/used size.

  @retval RETURN_INVALID_PARAMETER  BufferSize is NULL.
  @retval RETURN_BUFFER_TOO_SMALL   The buffer is too small for HOB creation.
                                    BufferSize is updated to indicate the expected buffer size.
                                    When the input BufferSize is bigger than the expected buffer size,
                                    the BufferSize value will be changed the used buffer size.
  @retval RETURN_SUCCESS            The HOB list is created successfully.

**/
EFI_STATUS
EFIAPI
CreateMmPlatformHob (
  IN VOID       *Buffer,
  IN OUT UINTN  *BufferSize
  )
{
  VOID                            *HobList;
  EFI_PEI_HOB_POINTERS            Hob;
  EFI_PEI_HOB_POINTERS            FirstResHob;
  UINTN                           Count;
  UINTN                           Index;
  MM_PLATFORM_MEMORY_REGION       *MemoryRegion;
  MM_PLATFORM_MEMORY_REGION       SortBuffer;
  UINTN                           RequiredSize;
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK  *SmramHob;

  if (BufferSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((*BufferSize != 0) && (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  Count        = 0;
  RequiredSize = 0;
  HobList      = GetHobList ();
  ASSERT (HobList != NULL);

  //
  // Count the Resource HOB number
  //
  Hob.Raw     = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  FirstResHob = Hob;
  while (Hob.Raw != NULL) {
    if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      Count++;
    }

    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, GET_NEXT_HOB (Hob));
  }

  //
  // Count the gEfiSmmSmramMemoryGuid Descriptor number
  //
  Hob.Raw = GetFirstGuidHob (&gEfiSmmSmramMemoryGuid);
  ASSERT (Hob.Raw != NULL);
  SmramHob = GET_GUID_HOB_DATA (Hob.Raw);
  Count   += SmramHob->NumberOfSmmReservedRegions;

  MemoryRegion = AllocatePool (Count * sizeof (MM_PLATFORM_MEMORY_REGION));
  ASSERT (MemoryRegion != NULL);

  //
  // Cache gEfiSmmSmramMemoryGuid Descriptor
  //
  Index = 0;
  while (Index < SmramHob->NumberOfSmmReservedRegions) {
    MemoryRegion[Index].Base   = SmramHob->Descriptor[Index].PhysicalStart;
    MemoryRegion[Index].Length = SmramHob->Descriptor[Index].PhysicalSize;
    Index++;
  }

  //
  // Cache resource HOB
  //
  Hob = FirstResHob;
  while (Hob.Raw != NULL) {
    if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      MemoryRegion[Index].Base   = Hob.ResourceDescriptor->PhysicalStart;
      MemoryRegion[Index].Length = Hob.ResourceDescriptor->ResourceLength;
      Index++;
    }

    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, GET_NEXT_HOB (Hob));
  }

  ASSERT (Index == Count);

  //
  // Perform QuickSort for all MM_PLATFORM_MEMORY_REGION range for calculating the MMIO
  //
  QuickSort (MemoryRegion, Count, sizeof (MM_PLATFORM_MEMORY_REGION), (BASE_SORT_COMPARE)MmMemoryDescriptorCompare, &SortBuffer);

  //
  // Calculate needed buffer size.
  //
  RequiredSize = MmBuildHobForMmio (FALSE, MemoryRegion, Count, NULL);

  if (*BufferSize < RequiredSize) {
    *BufferSize = RequiredSize;
    FreePool (MemoryRegion);
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Build resource HOB for MMIO range.
  //
  *BufferSize = MmBuildHobForMmio (TRUE, MemoryRegion, Count, Buffer);
  FreePool (MemoryRegion);

  return EFI_SUCCESS;
}
