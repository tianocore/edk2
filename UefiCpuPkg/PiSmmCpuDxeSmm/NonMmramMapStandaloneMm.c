/** @file

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuCommon.h"

/**
  Get SmmProfileData.

  @param[in, out]     Size     Return Size of SmmProfileData.
                               0 means the gMmProfileDataHobGuid does not exist.

  @return Address of SmmProfileData

**/
EFI_PHYSICAL_ADDRESS
GetSmmProfileData (
  IN OUT  UINTN  *Size
  )
{
  EFI_PEI_HOB_POINTERS  SmmProfileDataHob;

  ASSERT (Size != NULL);

  //
  // Get Smm Profile Base from Memory Allocation HOB
  //
  SmmProfileDataHob.Raw = GetFirstHob (EFI_HOB_TYPE_MEMORY_ALLOCATION);
  while (SmmProfileDataHob.Raw != NULL) {
    //
    // Find gMmProfileDataHobGuid
    //
    if (CompareGuid (&SmmProfileDataHob.MemoryAllocation->AllocDescriptor.Name, &gMmProfileDataHobGuid)) {
      break;
    }

    SmmProfileDataHob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, GET_NEXT_HOB (SmmProfileDataHob));
  }

  if (SmmProfileDataHob.Raw == NULL) {
    *Size = 0;
    return 0;
  }

  *Size = (UINTN)SmmProfileDataHob.MemoryAllocation->AllocDescriptor.MemoryLength;

  return SmmProfileDataHob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
}

/**
  Return if the Address is the NonMmram logging Address.

  @param[in] Address the address to be checked

  @return TRUE  The address is the NonMmram logging Address.
  @return FALSE The address is not the NonMmram logging Address.
**/
BOOLEAN
IsNonMmramLoggingAddress (
  IN UINT64  Address
  )
{
  EFI_PEI_HOB_POINTERS  Hob;

  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (!IS_RESOURCE_DESCRIPTOR_HOB (Hob)) {
      continue;
    }

    if ((Address >= Hob.ResourceDescriptor->PhysicalStart) && (Address < Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength)) {
      if ((GET_RESOURCE_HOB_ATTRIBUTE (Hob) & MM_RESOURCE_ATTRIBUTE_LOGGING) != 0) {
        return TRUE;
      }

      return FALSE;
    }
  }

  return FALSE;
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
  EFI_PEI_HOB_POINTERS  Hob;

  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (!IS_RESOURCE_DESCRIPTOR_HOB (Hob)) {
      continue;
    }

    if ((Address >= Hob.ResourceDescriptor->PhysicalStart) && (Address < Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Build Memory Region from ResourceDescriptor HOBs by excluding Logging attribute range.

  @param[out]     MemoryRegion            Returned Non-Mmram Memory regions.
  @param[out]     MemoryRegionCount       A pointer to the number of Memory regions.
**/
VOID
BuildMemoryMapFromResDescHobs (
  OUT MM_CPU_MEMORY_REGION  **MemoryRegion,
  OUT UINTN                 *MemoryRegionCount
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  UINTN                 Count;
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  MaxPhysicalAddress;
  EFI_PHYSICAL_ADDRESS  ResourceHobEnd;

  ASSERT (MemoryRegion != NULL && MemoryRegionCount != NULL);

  *MemoryRegion      = NULL;
  *MemoryRegionCount = 0;
  MaxPhysicalAddress = LShiftU64 (1, mPhysicalAddressBits);

  //
  // Get the count.
  //
  Count = 0;
  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (!IS_RESOURCE_DESCRIPTOR_HOB (Hob)) {
      continue;
    }

    if ((GET_RESOURCE_HOB_ATTRIBUTE (Hob) & MM_RESOURCE_ATTRIBUTE_LOGGING) == 0) {
      ResourceHobEnd = Hob.ResourceDescriptor->PhysicalStart + Hob.ResourceDescriptor->ResourceLength;

      ASSERT (ResourceHobEnd <= MaxPhysicalAddress);
      if (ResourceHobEnd > MaxPhysicalAddress) {
        CpuDeadLoop ();
      }

      //
      // Resource HOBs describe all accessible non-smram regions.
      // Logging attribute range is treated as not present. Not-present ranges are not included in this memory map.
      //
      Count++;
    }
  }

  *MemoryRegionCount = Count;

  *MemoryRegion = (MM_CPU_MEMORY_REGION *)AllocateZeroPool (sizeof (MM_CPU_MEMORY_REGION) * Count);
  ASSERT (*MemoryRegion != NULL);

  Index = 0;
  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (!IS_RESOURCE_DESCRIPTOR_HOB (Hob)) {
      continue;
    }

    if ((GET_RESOURCE_HOB_ATTRIBUTE (Hob) & MM_RESOURCE_ATTRIBUTE_LOGGING) == 0) {
      ASSERT (Index < Count);
      (*MemoryRegion)[Index].Base      = Hob.ResourceDescriptor->PhysicalStart;
      (*MemoryRegion)[Index].Length    = Hob.ResourceDescriptor->ResourceLength;
      (*MemoryRegion)[Index].Attribute = EFI_MEMORY_XP;
      if (GET_RESOURCE_HOB_ATTRIBUTE (Hob) == EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED) {
        (*MemoryRegion)[Index].Attribute |= EFI_MEMORY_RO;
      }

      Index++;
    }
  }

  return;
}

/**
  Build extended protection MemoryRegion.

  The caller is responsible for freeing MemoryRegion via FreePool().

  @param[out]     MemoryRegion         Returned Non-Mmram Memory regions.
  @param[out]     MemoryRegionCount    A pointer to the number of Memory regions.
**/
VOID
CreateExtendedProtectionRange (
  OUT MM_CPU_MEMORY_REGION  **MemoryRegion,
  OUT UINTN                 *MemoryRegionCount
  )
{
  BuildMemoryMapFromResDescHobs (MemoryRegion, MemoryRegionCount);
}

/**
  Create the Non-Mmram Memory Region within the ResourceDescriptor HOBs
  without Logging attribute.

  The caller is responsible for freeing MemoryRegion via FreePool().

  @param[in]      PhysicalAddressBits  The bits of physical address to map.
  @param[out]     MemoryRegion         Returned Non-Mmram Memory regions.
  @param[out]     MemoryRegionCount    A pointer to the number of Memory regions.
**/
VOID
CreateNonMmramMemMap (
  IN  UINT8                 PhysicalAddressBits,
  OUT MM_CPU_MEMORY_REGION  **MemoryRegion,
  OUT UINTN                 *MemoryRegionCount
  )
{
  BuildMemoryMapFromResDescHobs (MemoryRegion, MemoryRegionCount);
}
