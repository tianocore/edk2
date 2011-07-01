/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiPei.h>

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <Chipset/ArmV7.h>

VOID
BuildMemoryTypeInformationHob (
  VOID
  );

VOID
InitMmu (
  VOID
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *MemoryTable;
  VOID                          *TranslationTableBase;
  UINTN                         TranslationTableSize;

  // Get Virtual Memory Map from the Platform Library
  ArmPlatformGetVirtualMemoryMap(&MemoryTable);

  //Note: Because we called PeiServicesInstallPeiMemory() before to call InitMmu() the MMU Page Table resides in
  //      DRAM (even at the top of DRAM as it is the first permanent memory allocation)
  ArmConfigureMmu (MemoryTable, &TranslationTableBase, &TranslationTableSize);
}

/*++

Routine Description:



Arguments:

  FileHandle  - Handle of the file being invoked.
  PeiServices - Describes the list of possible PEI Services.

Returns:

  Status -  EFI_SUCCESS if the boot mode could be set

--*/
EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS               UefiMemoryBase,
  IN UINT64                             UefiMemorySize
  )
{
  EFI_STATUS                            Status;
  EFI_RESOURCE_ATTRIBUTE_TYPE           Attributes;
  ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR*  EfiMemoryMap;
  UINTN                                 Index;
  EFI_PHYSICAL_ADDRESS                  SystemMemoryTop;

  // Ensure PcdSystemMemorySize has been set
  ASSERT (PcdGet32 (PcdSystemMemorySize) != 0);

  SystemMemoryTop = (EFI_PHYSICAL_ADDRESS)((UINT32)PcdGet32 (PcdSystemMemoryBase) + (UINT32)PcdGet32 (PcdSystemMemorySize));

  //
  // Now, the permanent memory has been installed, we can call AllocatePages()
  //
  Attributes = (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED
  );

  // If it is not a standalone build we must reserved the space above the base address of the firmware volume
  if (!PcdGet32(PcdStandalone)) {
    // Check if firmware volume has not be copied at the top of DRAM then we must reserve the extra space
    // between the firmware and the top
    if (SystemMemoryTop != PcdGet32 (PcdNormalFdBaseAddress) + PcdGet32 (PcdNormalFdSize)) {
      BuildResourceDescriptorHob (
          EFI_RESOURCE_SYSTEM_MEMORY,
          Attributes & (~EFI_RESOURCE_ATTRIBUTE_TESTED),
          PcdGet32 (PcdNormalFdBaseAddress) + PcdGet32 (PcdNormalFdSize),
          SystemMemoryTop - (PcdGet32 (PcdNormalFdBaseAddress) + PcdGet32 (PcdNormalFdSize))
      );
    }

    // Reserved the memory space occupied by the firmware volume
    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        Attributes & (~EFI_RESOURCE_ATTRIBUTE_PRESENT),
        (UINT32)PcdGet32 (PcdNormalFdBaseAddress),
        (UINT32)PcdGet32 (PcdNormalFdSize)
    );
  }

  // Check there is no overlap between UEFI and Fix Address Regions
  ASSERT (PcdGet32 (PcdSystemMemoryBase) + PcdGet32 (PcdSystemMemoryFixRegionSize) <= UefiMemoryBase);

  // Reserved the UEFI Memory Region
  BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      Attributes,
      UefiMemoryBase,
      UefiMemorySize
  );

  // Reserved the Fix Address Region
  BuildResourceDescriptorHob (
      EFI_RESOURCE_SYSTEM_MEMORY,
      Attributes,
      PcdGet32 (PcdSystemMemoryBase),
      PcdGet32 (PcdSystemMemoryFixRegionSize)
  );

  // Reserved the memory between UEFI and Fix Address regions
  if (PcdGet32 (PcdSystemMemoryBase) + PcdGet32 (PcdSystemMemoryFixRegionSize) != UefiMemoryBase) {
    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        Attributes & (~EFI_RESOURCE_ATTRIBUTE_TESTED),
        PcdGet32 (PcdSystemMemoryBase) + PcdGet32 (PcdSystemMemoryFixRegionSize),
        UefiMemoryBase - (PcdGet32 (PcdSystemMemoryBase) + PcdGet32 (PcdSystemMemoryFixRegionSize))
    );
  }

  // If a platform has system memory extensions, it can declare those in this function
  Status = ArmPlatformGetAdditionalSystemMemory (&EfiMemoryMap);
  if (!EFI_ERROR(Status)) {
    // Install the EFI Memory Map
    for (Index = 0; EfiMemoryMap[Index].ResourceAttribute != 0; Index++) {
      BuildResourceDescriptorHob (
          EFI_RESOURCE_SYSTEM_MEMORY,
          EfiMemoryMap[Index].ResourceAttribute,
          EfiMemoryMap[Index].PhysicalStart,
          EfiMemoryMap[Index].NumberOfBytes
      );
    }
    FreePool (EfiMemoryMap);
  }

  // Build Memory Allocation Hob
  InitMmu ();

  if (FeaturePcdGet (PcdPrePiProduceMemoryTypeInformationHob)) {
    // Optional feature that helps prevent EFI memory map fragmentation.
    BuildMemoryTypeInformationHob ();
  }

  return EFI_SUCCESS;
}
