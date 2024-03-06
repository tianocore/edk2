/** @file

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

/**
  This function sets UEFI memory attribute according to UEFI memory map.
**/
VOID
SetUefiMemMapAttributes (
  UINT64  StartAddress,
  UINT64  EndAddress
  )
{
  RETURN_STATUS                Status;
  UINTN                        Index;
  EFI_PEI_HOB_POINTERS         Hob;
  UINTN                        Count;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor;
  EFI_MEMORY_DESCRIPTOR        *MemoryMap;
  UINTN                        MemoryMapSize;
  UINTN                        DescriptorSize;
  UINTN                        PageTable;
  UINT64                       PreviousAddress;
  UINT64                       Base;
  UINT64                       Length;
  UINT64                       MemoryAttr;

  //
  // Get the count of resource hob & SmramRangeCount.
  //
  Count   = mSmmCpuSmramRangeCount;
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    ResourceDescriptor = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;

    Count++;
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  //
  // Allocate the MemoryMap
  //
  DescriptorSize = sizeof (EFI_MEMORY_DESCRIPTOR);
  MemoryMapSize  =  DescriptorSize * Count;

  MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool (MemoryMapSize);
  ASSERT (MemoryMap != NULL);

  //
  // Move SmramRange and Resource HOBs to MemoryMap for ordering.
  //
  Index = 0;
  for (Index = 0; Index < mSmmCpuSmramRangeCount; Index++) {
    ASSERT (Index < mSmmCpuSmramRangeCount);
    MemoryMap[Index].PhysicalStart = mSmmCpuSmramRanges[Index].CpuStart;
    MemoryMap[Index].NumberOfPages = EFI_SIZE_TO_PAGES (mSmmCpuSmramRanges[Index].PhysicalSize);
    MemoryMap[Index].Attribute     = 0;
  }

  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    ResourceDescriptor = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
    ASSERT (Index < Count);
    MemoryMap[Index].PhysicalStart = ResourceDescriptor->PhysicalStart;
    MemoryMap[Index].NumberOfPages = EFI_SIZE_TO_PAGES (ResourceDescriptor->ResourceLength);
    MemoryMap[Index].Attribute     = EFI_MEMORY_XP;
    if (ResourceDescriptor->ResourceAttribute == EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED) {
      MemoryMap[Index].Attribute |= EFI_MEMORY_RO;
    }

    Index++;
    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  //
  // Sort memory map entries based upon PhysicalStart, from low to high
  //
  SortMemoryMap (MemoryMap, MemoryMapSize, DescriptorSize);

  //
  // Set Uefi MemoryMap Attributes
  //
  PageTable       = AsmReadCr3 ();
  PreviousAddress = StartAddress;
  for (Index = 0; Index < MemoryMapSize / DescriptorSize; Index++) {
    Base   = MemoryMap[Index].PhysicalStart;
    Length = EFI_PAGES_TO_SIZE ((UINTN)MemoryMap[Index].NumberOfPages);

    //
    // Only set [StartAddress, EndAddress] linear-address range memory attributes.
    // Note: Consider the case that 5-Level paging is disabled with more than 47
    //       physical-address bits. The range in memory map entries might go beyond
    //       the EndAddress.
    //
    if (Base >= EndAddress) {
      break;
    }

    if (Base + Length > EndAddress) {
      Length = EndAddress - Base;
    }

    if (MemoryMap[Index].Attribute != 0) {
      Status = ConvertMemoryPageAttributes (PageTable, mPagingMode, Base, Length, MemoryMap[Index].Attribute, TRUE, NULL);
      ASSERT_RETURN_ERROR (Status);

      if (Base > PreviousAddress) {
        //
        // Mark the ranges not in MemoryMap as non-present if non-SMRAM is restricted to access.
        //
        if (IsRestrictedMemoryAccess ()) {
          MemoryAttr = EFI_MEMORY_RP;
        } else {
          MemoryAttr = EFI_MEMORY_XP;
        }

        Status = ConvertMemoryPageAttributes (PageTable, mPagingMode, PreviousAddress, Base - PreviousAddress, MemoryAttr, TRUE, NULL);
        ASSERT_RETURN_ERROR (Status);
      }
    }

    PreviousAddress = Base + Length;
  }

  //
  // Set the last remaining range
  //
  if (PreviousAddress < EndAddress) {
    Status = ConvertMemoryPageAttributes (PageTable, mPagingMode, PreviousAddress, EndAddress - PreviousAddress, MemoryAttr, TRUE, NULL);
    ASSERT_RETURN_ERROR (Status);
  }

  //
  // Flush TLB
  //
  CpuFlushTlb ();
}

/**
  Return if the Address is forbidden as SMM communication buffer.

  @param[in] Address the address to be checked

  @return TRUE  The address is forbidden as SMM communication buffer.
  @return FALSE The address is allowed as SMM communication buffer.
**/
BOOLEAN
IsSmmCommBufferForbiddenAddress (
  IN UINT64  Address
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor;

  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    ResourceDescriptor = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
    if ((Address >= ResourceDescriptor->PhysicalStart) && (Address < ResourceDescriptor->PhysicalStart + ResourceDescriptor->ResourceLength)) {
      return FALSE;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  return TRUE;
}

VOID
SmmGetMmioRanges (
  OUT EFI_MEMORY_DESCRIPTOR  **MemoryMap,
  OUT UINTN                  *MemoryMapSize,
  OUT UINTN                  *DescriptorSize
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  UINTN                        Count;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor;
  UINTN                        Index;

  ASSERT (MemoryMap != NULL && MemoryMapSize != NULL && DescriptorSize != NULL);

  *MemoryMap      = NULL;
  *MemoryMapSize  = 0;
  *DescriptorSize = 0;

  //
  // Get the count of MMIO resource descriptor.
  //
  Count   = 0;
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    ResourceDescriptor = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
    if (ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_MAPPED_IO) {
      Count++;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  *DescriptorSize = sizeof (EFI_MEMORY_DESCRIPTOR);
  *MemoryMapSize  =  *DescriptorSize * Count;

  *MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocateZeroPool (*MemoryMapSize);
  ASSERT (*MemoryMap != NULL);

  Index   = 0;
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    ResourceDescriptor = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
    if (ResourceDescriptor->ResourceType == EFI_RESOURCE_MEMORY_MAPPED_IO) {
      ASSERT (Index < Count);
      (*MemoryMap)[Index].PhysicalStart = ResourceDescriptor->PhysicalStart;
      (*MemoryMap)[Index].NumberOfPages = EFI_SIZE_TO_PAGES (ResourceDescriptor->ResourceLength);
      (*MemoryMap)[Index].Type          = EfiMemoryMappedIO;
      Index++;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  return;
}
