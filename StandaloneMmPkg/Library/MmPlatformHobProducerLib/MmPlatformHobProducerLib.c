/** @file
  HOB Producer Library implementation for Standalone MM Core.

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

UINTN  mHobListEnd;

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
UINTN *
MmGetHobListEndPointer (
  VOID
  )
{
  return &mHobListEnd;
}

/**
  Build a Handoff Information Table HOB

  This function initialize a HOB region from EfiMemoryBegin to
  EfiMemoryTop. And EfiFreeMemoryBottom and EfiFreeMemoryTop should
  be inside the HOB region.

  @param[in] EfiMemoryBottom       Total memory start address
  @param[in] EfiMemoryTop          Total memory end address.
  @param[in] EfiFreeMemoryBottom   Free memory start address
  @param[in] EfiFreeMemoryTop      Free memory end address.

  @return   The pointer to the handoff HOB table.

**/
VOID
MmHobConstructor (
  IN VOID  *Buffer
  )
{
  UINTN  *HobListEnd;

  HobListEnd  = MmGetHobListEndPointer ();
  *HobListEnd = (UINTN)Buffer;
}

/**
  Add a new HOB to the HOB List.

  @param HobType            Type of the new HOB.
  @param HobLength          Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
MmCreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  )
{
  VOID   *Hob;
  UINTN  *HobListEnd;

  HobListEnd = MmGetHobListEndPointer ();

  Hob                                        = (VOID *)(*HobListEnd);
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobType   = HobType;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobLength = HobLength;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->Reserved  = 0;

  *HobListEnd = (UINTN)Hob + HobLength;

  return Hob;
}

/**
  Builds a HOB that describes a chunk of system memory.

  This function builds a HOB that describes a chunk of system memory.
  If there is no additional space for HOB creation, then ASSERT().

  @param  ResourceType        The type of resource described by this HOB.
  @param  ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param  PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param  NumberOfBytes       The length of the memory described by this HOB in bytes.

**/
VOID
MmBuildResourceDescriptorHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  )
{
  EFI_HOB_RESOURCE_DESCRIPTOR  *Hob;

  Hob = MmCreateHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, sizeof (EFI_HOB_RESOURCE_DESCRIPTOR));
  ASSERT (Hob != NULL);
  if (Hob == NULL) {
    return;
  }

  Hob->ResourceType      = ResourceType;
  Hob->ResourceAttribute = ResourceAttribute;
  Hob->PhysicalStart     = PhysicalStart;
  Hob->ResourceLength    = NumberOfBytes;
}

/**
  Calculate the maximum support address.

  @return the maximum support address.
**/
UINT8
CalculateMaximumSupportAddress (
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

  This function treats all all ranges outside the system memory range as mmio
  and builds resource HOB list for all MMIO range.

  @param  Create         The type of resource described by this HOB.
  @param  MemoryMap      EFI_MEMORY_DESCRIPTOR that describes all system memory range.
  @param  Count          Number of EFI_MEMORY_DESCRIPTOR.
  @param  MmioMemoryMap  The memory map buffer for MMIO.

**/
UINTN
MmBuildHobForMmio (
  IN  BOOLEAN                Create,
  IN  EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN  UINTN                  Count,
  OUT EFI_MEMORY_DESCRIPTOR  *MmioMemoryMap
  )
{
  UINTN                        MmioRangeCount;
  UINT64                       PreviousAddress;
  UINT64                       Base;
  UINT64                       Limit;
  UINT8                        PhysicalAddressBits;
  UINTN                        Index;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribute;

  Index               = 0;
  PreviousAddress     = 0;
  MmioRangeCount      = 0;
  PhysicalAddressBits = CalculateMaximumSupportAddress ();
  Limit               = LShiftU64 (1, PhysicalAddressBits);
  Attribute           = EFI_RESOURCE_ATTRIBUTE_PRESENT |
                        EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                        EFI_RESOURCE_ATTRIBUTE_TESTED;

  if (FeaturePcdGet (PcdCpuSmmProfileEnable) == TRUE) {
    Attribute |= EDKII_MM_RESOURCE_ATTRIBUTE_LOGGING;
  }

  for (Index = 0; Index <= Count; Index++) {
    Base = (Index == Count) ? Limit : MemoryMap[Index].PhysicalStart;
    if (Base > PreviousAddress) {
      if (MmioMemoryMap != NULL) {
        MmioMemoryMap[MmioRangeCount].PhysicalStart = PreviousAddress;
        MmioMemoryMap[MmioRangeCount].NumberOfPages = EFI_SIZE_TO_PAGES (Base - PreviousAddress);
      }
      MmioRangeCount++;
      if (Create) {
        MmBuildResourceDescriptorHob (
          EFI_RESOURCE_MEMORY_MAPPED_IO,
          Attribute,
          PreviousAddress,
          Base - PreviousAddress
          );
      }
    }

    if (Index < Count) {
      PreviousAddress = MemoryMap[Index].PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap[Index].NumberOfPages);
    }
  }

  return MmioRangeCount * sizeof (EFI_HOB_RESOURCE_DESCRIPTOR);
}

/**
  Function to compare 2 EFI_MEMORY_DESCRIPTOR pointer based on PhysicalStart.

  @param[in] Buffer1            pointer to MP_INFORMATION2_HOB_DATA poiner to compare
  @param[in] Buffer2            pointer to second MP_INFORMATION2_HOB_DATA pointer to compare

  @retval 0                     Buffer1 equal to Buffer2
  @retval <0                    Buffer1 is less than Buffer2
  @retval >0                    Buffer1 is greater than Buffer2
**/
INTN
EFIAPI
MemoryDescriptorCompare (
  IN  CONST VOID  *Buffer1,
  IN  CONST VOID  *Buffer2
  )
{
  if (((EFI_MEMORY_DESCRIPTOR *)Buffer1)->PhysicalStart > ((EFI_MEMORY_DESCRIPTOR *)Buffer2)->PhysicalStart) {
    return 1;
  } else if (((EFI_MEMORY_DESCRIPTOR *)Buffer1)->PhysicalStart < ((EFI_MEMORY_DESCRIPTOR *)Buffer2)->PhysicalStart) {
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
  @param[out]     MmioMemoryMap     The memory map buffer for MMIO.

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
  IN      VOID                   *Buffer,
  IN  OUT UINTN                  *BufferSize,
  OUT     EFI_MEMORY_DESCRIPTOR  *MmioMemoryMap
  )
{
  VOID                   *HobList;
  EFI_PEI_HOB_POINTERS   Hob;
  EFI_PEI_HOB_POINTERS   FirstResHob;
  UINTN                  Count;
  UINTN                  Index;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_MEMORY_DESCRIPTOR  SortBuffer;
  UINTN                  RequiredSize;

  if (BufferSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((*BufferSize != 0) && (Buffer == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  Index        = 0;
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
  // Cache resource HOB
  //
  MemoryMap = AllocatePool (Count * sizeof (EFI_MEMORY_DESCRIPTOR));
  ASSERT (MemoryMap != NULL);
  Hob = FirstResHob;
  while (Hob.Raw != NULL) {
    if (Hob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      MemoryMap[Index].PhysicalStart = Hob.ResourceDescriptor->PhysicalStart;
      MemoryMap[Index].NumberOfPages = EFI_SIZE_TO_PAGES (Hob.ResourceDescriptor->ResourceLength);
      Index++;
    }

    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, GET_NEXT_HOB (Hob));
  }

  ASSERT (Index == Count);

  //
  // Perform QuickSort for all EFI_RESOURCE_SYSTEM_MEMORY range to calculating the MMIO
  //
  QuickSort (MemoryMap, Count, sizeof (EFI_MEMORY_DESCRIPTOR), (BASE_SORT_COMPARE)MemoryDescriptorCompare, &SortBuffer);

  //
  // Calculate needed buffer size.
  //
  RequiredSize = MmBuildHobForMmio (FALSE, MemoryMap, Count, MmioMemoryMap);

  if (*BufferSize < RequiredSize) {
    *BufferSize = RequiredSize;
    FreePool (MemoryMap);
    return EFI_BUFFER_TOO_SMALL;
  }

  ASSERT (Buffer != NULL);
  MmHobConstructor (Buffer);

  //
  // Build resource HOB for MMIO range.
  //
  *BufferSize = MmBuildHobForMmio (TRUE, MemoryMap, Count, MmioMemoryMap);
  FreePool (MemoryMap);

  return EFI_SUCCESS;
}
