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

VOID  *mHobList;

/**
  Returns the pointer to the HOB list.

  This function returns the pointer to first HOB in the list.

  @return The pointer to the HOB list.

**/
VOID *
MmGetHobList (
  VOID
  )
{
  ASSERT (mHobList != NULL);
  return mHobList;
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
EFI_HOB_HANDOFF_INFO_TABLE *
MmHobConstructor (
  IN VOID  *EfiMemoryBottom,
  IN VOID  *EfiMemoryTop,
  IN VOID  *EfiFreeMemoryBottom,
  IN VOID  *EfiFreeMemoryTop
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *Hob;
  EFI_HOB_GENERIC_HEADER      *HobEnd;

  Hob    = EfiFreeMemoryBottom;
  HobEnd = (EFI_HOB_GENERIC_HEADER *)(Hob+1);

  Hob->Header.HobType   = EFI_HOB_TYPE_HANDOFF;
  Hob->Header.HobLength = sizeof (EFI_HOB_HANDOFF_INFO_TABLE);
  Hob->Header.Reserved  = 0;

  HobEnd->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength = sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved  = 0;

  Hob->Version  = EFI_HOB_HANDOFF_TABLE_VERSION;
  Hob->BootMode = BOOT_WITH_FULL_CONFIGURATION;

  Hob->EfiMemoryTop        = (EFI_PHYSICAL_ADDRESS)(UINTN)EfiMemoryTop;
  Hob->EfiMemoryBottom     = (EFI_PHYSICAL_ADDRESS)(UINTN)EfiMemoryBottom;
  Hob->EfiFreeMemoryTop    = (EFI_PHYSICAL_ADDRESS)(UINTN)EfiFreeMemoryTop;
  Hob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS)(UINTN)(HobEnd+1);
  Hob->EfiEndOfHobList     = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

  mHobList = Hob;
  return Hob;
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
  EFI_HOB_HANDOFF_INFO_TABLE  *HandOffHob;
  EFI_HOB_GENERIC_HEADER      *HobEnd;
  EFI_PHYSICAL_ADDRESS        FreeMemory;
  VOID                        *Hob;

  HandOffHob = MmGetHobList ();

  //
  // Check Length to avoid data overflow.
  //
  if (HobLength > MAX_UINT16 - 0x7) {
    return NULL;
  }

  HobLength = (UINT16)((HobLength + 0x7) & (~0x7));

  FreeMemory = HandOffHob->EfiFreeMemoryTop - HandOffHob->EfiFreeMemoryBottom;

  if (FreeMemory < HobLength) {
    return NULL;
  }

  Hob                                        = (VOID *)(UINTN)HandOffHob->EfiEndOfHobList;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobType   = HobType;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->HobLength = HobLength;
  ((EFI_HOB_GENERIC_HEADER *)Hob)->Reserved  = 0;

  HobEnd                      = (EFI_HOB_GENERIC_HEADER *)((UINTN)Hob + HobLength);
  HandOffHob->EfiEndOfHobList = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

  HobEnd->HobType   = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength = sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved  = 0;
  HobEnd++;
  HandOffHob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

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
  if ((*(EFI_MEMORY_DESCRIPTOR **)Buffer1)->PhysicalStart > (*(EFI_MEMORY_DESCRIPTOR **)Buffer2)->PhysicalStart) {
    return 1;
  } else if ((*(EFI_MEMORY_DESCRIPTOR **)Buffer1)->PhysicalStart < (*(EFI_MEMORY_DESCRIPTOR **)Buffer2)->PhysicalStart) {
    return -1;
  }

  return 0;
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
  Create the HOB list which StandaloneMm Core needed.

  This function searches the HOBs needed by StandaloneMm Core among the whole
  HOB list. If the input pointer to the HOB list is NULL, then ASSERT().

  @return The pointer of the whole HOB list which StandaloneMm Core needed.

**/
EFI_HOB_HANDOFF_INFO_TABLE *
EFIAPI
CreateMmCoreHobList (
  VOID
  )
{
  VOID                         *HobList;
  UINTN                        EfiMemoryBottom;
  UINTN                        EfiMemoryTop;
  UINTN                        NumberOfPages;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_PEI_HOB_POINTERS         FirstResHob;
  UINTN                        Count;
  UINTN                        Index;
  UINT64                       PreviousAddress;
  UINT64                       Base;
  UINT64                       Limit;
  EFI_MEMORY_DESCRIPTOR        *MemoryMap;
  EFI_MEMORY_DESCRIPTOR        SortBuffer;
  UINT8                        PhysicalAddressBits;
  UINT64                       MaxHobLength;

  Index   = 0;
  Count   = 0;
  HobList = GetHobList ();
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

  MaxHobLength = (Count + 1) * sizeof (EFI_HOB_RESOURCE_DESCRIPTOR) +
                 sizeof (EFI_HOB_HANDOFF_INFO_TABLE) + sizeof (EFI_HOB_GENERIC_HEADER);
  NumberOfPages   = EFI_SIZE_TO_PAGES (MaxHobLength);
  EfiMemoryBottom = (UINTN)AllocatePages (NumberOfPages);
  EfiMemoryTop    = EfiMemoryBottom + EFI_PAGES_TO_SIZE (NumberOfPages);

  //
  // 1.New HOB database initialization
  //
  MmHobConstructor ((VOID *)EfiMemoryBottom, (VOID *)EfiMemoryTop, (VOID *)EfiMemoryBottom, (VOID *)EfiMemoryTop);

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED;

  //
  // 2.Build Resource HOB for MMIO range
  //
  //
  // Cache resource HOB
  //
  MemoryMap = AllocatePool (Count * sizeof (EFI_MEMORY_DESCRIPTOR));
  Hob       = FirstResHob;
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
  // Perform QuickSort
  //
  QuickSort (MemoryMap, Index, sizeof (EFI_MEMORY_DESCRIPTOR *), (BASE_SORT_COMPARE)MemoryDescriptorCompare, &SortBuffer);

  PhysicalAddressBits = CalculateMaximumSupportAddress ();
  Limit               = LShiftU64 (1, PhysicalAddressBits);

  //
  // Build Resource HOB for MMIO range
  //
  PreviousAddress = 0;
  for (Index = 0; Index < Count; Index++) {
    Base = MemoryMap[Index].PhysicalStart;
    if (Base > PreviousAddress) {
      MmBuildResourceDescriptorHob (
        EFI_RESOURCE_MEMORY_MAPPED_IO,
        Attribue,
        PreviousAddress,
        Base - PreviousAddress
        );
    }

    PreviousAddress = MemoryMap[Index].PhysicalStart + EFI_PAGES_TO_SIZE (MemoryMap[Index].NumberOfPages);
  }

  if (PreviousAddress < Limit) {
    MmBuildResourceDescriptorHob (
      EFI_RESOURCE_MEMORY_MAPPED_IO,
      Attribue,
      PreviousAddress,
      Limit - PreviousAddress
      );
  }

  return MmGetHobList ();
}
