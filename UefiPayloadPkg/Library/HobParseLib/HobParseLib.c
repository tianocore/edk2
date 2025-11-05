/** @file
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Guid/MemoryAllocationHob.h>
#include <Library/IoLib.h>
#include <Library/CpuLib.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/MemoryMappedConfigurationSpaceAccessTable.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <UniversalPayload/AcpiTable.h>
#include <UniversalPayload/UniversalPayload.h>
#include <UniversalPayload/ExtraData.h>

#define MEMORY_ATTRIBUTE_MASK  (EFI_RESOURCE_ATTRIBUTE_PRESENT             |        \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED         | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED              | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED      | \
                                       EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED     | \
                                       EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_READ_ONLY_PROTECTED | \
                                       EFI_RESOURCE_ATTRIBUTE_16_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_32_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_64_BIT_IO           | \
                                       EFI_RESOURCE_ATTRIBUTE_PERSISTENT          )

#define TESTED_MEMORY_ATTRIBUTES  (EFI_RESOURCE_ATTRIBUTE_PRESENT     |     \
                                       EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                                       EFI_RESOURCE_ATTRIBUTE_TESTED      )

extern VOID  *mHobList;

/**
  Add a new HOB to the HOB List.

  @param HobType            Type of the new HOB.
  @param HobLength          Length of the new HOB to allocate.

  @return  NULL if there is no space to create a hob.
  @return  The address point to the new created hob.

**/
VOID *
EFIAPI
CreateHob (
  IN  UINT16  HobType,
  IN  UINT16  HobLength
  );

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
EFIAPI
HobConstructor (
  IN VOID  *EfiMemoryBottom,
  IN VOID  *EfiMemoryTop,
  IN VOID  *EfiFreeMemoryBottom,
  IN VOID  *EfiFreeMemoryTop
  );

/**
  Build ACPI board info HOB using infomation from ACPI table

  @param  AcpiTableBase      ACPI table start address in memory

  @retval  A pointer to ACPI board HOB ACPI_BOARD_INFO. Null if build HOB failure.
**/
ACPI_BOARD_INFO *
BuildHobFromAcpi (
  IN   UINT64  AcpiTableBase
  );

/**
 *
  Add HOB into HOB list

  @param[in]  Hob    The HOB to be added into the HOB list.
**/
VOID
AddNewHob (
  IN EFI_PEI_HOB_POINTERS  *Hob
  )
{
  EFI_PEI_HOB_POINTERS  NewHob;

  if (Hob->Raw == NULL) {
    return;
  }

  NewHob.Header = CreateHob (Hob->Header->HobType, Hob->Header->HobLength);

  if (NewHob.Header != NULL) {
    CopyMem (NewHob.Header + 1, Hob->Header + 1, Hob->Header->HobLength - sizeof (EFI_HOB_GENERIC_HEADER));
  }
}

/**
  Found the Resource Descriptor HOB that contains a range (Base, Top)

  @param[in] HobList    Hob start address
  @param[in] Base       Memory start address
  @param[in] Top        Memory end address.

  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindResourceDescriptorByRange (
  IN VOID                  *HobList,
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN EFI_PHYSICAL_ADDRESS  Top
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }

    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not contain the PHIT range EfiFreeMemoryBottom..EfiFreeMemoryTop
    //
    if (Base < ResourceHob->PhysicalStart) {
      continue;
    }

    if (Top > (ResourceHob->PhysicalStart + ResourceHob->ResourceLength)) {
      continue;
    }

    return ResourceHob;
  }

  return NULL;
}

/**
  Find the highest below 4G memory resource descriptor, except the input Resource Descriptor.

  @param[in] HobList                 Hob start address
  @param[in] MinimalNeededSize       Minimal needed size.
  @param[in] ExceptResourceHob       Ignore this Resource Descriptor.

  @retval     The pointer to the Resource Descriptor HOB.
**/
EFI_HOB_RESOURCE_DESCRIPTOR *
FindAnotherHighestBelow4GResourceDescriptor (
  IN VOID                         *HobList,
  IN UINTN                        MinimalNeededSize,
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *ExceptResourceHob
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceHob;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ReturnResourceHob;

  ReturnResourceHob = NULL;

  for (Hob.Raw = (UINT8 *)HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    //
    // Skip all HOBs except Resource Descriptor HOBs
    //
    if (GET_HOB_TYPE (Hob) != EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that do not describe tested system memory
    //
    ResourceHob = Hob.ResourceDescriptor;
    if (ResourceHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY) {
      continue;
    }

    if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) != TESTED_MEMORY_ATTRIBUTES) {
      continue;
    }

    //
    // Skip if the Resource Descriptor HOB equals to ExceptResourceHob
    //
    if (ResourceHob == ExceptResourceHob) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that are beyond 4G
    //
    if ((ResourceHob->PhysicalStart + ResourceHob->ResourceLength) > BASE_4GB) {
      continue;
    }

    //
    // Skip Resource Descriptor HOBs that are too small
    //
    if (ResourceHob->ResourceLength < MinimalNeededSize) {
      continue;
    }

    //
    // Return the topest Resource Descriptor
    //
    if (ReturnResourceHob == NULL) {
      ReturnResourceHob = ResourceHob;
    } else {
      if (ReturnResourceHob->PhysicalStart < ResourceHob->PhysicalStart) {
        ReturnResourceHob = ResourceHob;
      }
    }
  }

  return ReturnResourceHob;
}

/**
  Check the HOB and decide if it is need inside Payload

  Payload maintainer may make decision which HOB is need or needn't
  Then add the check logic in the function.

  @param[in] Hob The HOB to check

  @retval TRUE  If HOB is need inside Payload
  @retval FALSE If HOB is needn't inside Payload
**/
BOOLEAN
IsHobNeed (
  EFI_PEI_HOB_POINTERS  Hob
  )
{
  if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
    return FALSE;
  }

  if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
    if (CompareGuid (&Hob.MemoryAllocationModule->MemoryAllocationHeader.Name, &gEfiHobMemoryAllocModuleGuid)) {
      return FALSE;
    }
  }

  // Arrive here mean the HOB is need
  return TRUE;
}
