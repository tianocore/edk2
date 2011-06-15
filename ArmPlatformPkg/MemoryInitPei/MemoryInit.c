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

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/MasterBootMode.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Guid/MemoryTypeInformation.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ArmLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ArmPlatformLib.h>

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

// May want to put this into a library so you only need the PCD settings if you are using the feature?
VOID
BuildMemoryTypeInformationHob (
  VOID
  )
{
  EFI_MEMORY_TYPE_INFORMATION   Info[10];

  Info[0].Type          = EfiACPIReclaimMemory;
  Info[0].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIReclaimMemory);
  Info[1].Type          = EfiACPIMemoryNVS;
  Info[1].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiACPIMemoryNVS);
  Info[2].Type          = EfiReservedMemoryType;
  Info[2].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiReservedMemoryType);
  Info[3].Type          = EfiRuntimeServicesData;
  Info[3].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesData);
  Info[4].Type          = EfiRuntimeServicesCode;
  Info[4].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiRuntimeServicesCode);
  Info[5].Type          = EfiBootServicesCode;
  Info[5].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesCode);
  Info[6].Type          = EfiBootServicesData;
  Info[6].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiBootServicesData);
  Info[7].Type          = EfiLoaderCode;
  Info[7].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderCode);
  Info[8].Type          = EfiLoaderData;
  Info[8].NumberOfPages = PcdGet32 (PcdMemoryTypeEfiLoaderData);
  
  // Terminator for the list
  Info[9].Type          = EfiMaxMemoryType;
  Info[9].NumberOfPages = 0;

  BuildGuidDataHob (&gEfiMemoryTypeInformationGuid, &Info, sizeof (Info));
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
InitializeMemory (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                            Status;
  EFI_RESOURCE_ATTRIBUTE_TYPE           Attributes;
  ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR*  EfiMemoryMap;
  UINTN                                 Index;
  UINTN                                 SystemMemoryTop;
  UINTN                                 UefiMemoryBase;
  UINTN                                 UefiMemorySize;

  DEBUG ((EFI_D_ERROR, "Memory Init PEIM Loaded\n"));

  // Ensure PcdSystemMemorySize has been set
  ASSERT (FixedPcdGet32 (PcdSystemMemorySize) != 0);

  SystemMemoryTop = (UINTN)FixedPcdGet32 (PcdSystemMemoryBase) + (UINTN)FixedPcdGet32 (PcdSystemMemorySize);

  //
  // Initialize the System Memory (DRAM)
  //
  if (FeaturePcdGet(PcdStandalone)) {
    // In case of a standalone version, the DRAM is already initialized
    ArmPlatformInitializeSystemMemory();
  }

  //
  // Declare the UEFI memory to PEI
  //
  if (FeaturePcdGet(PcdStandalone)) {
    // In case of standalone UEFI, we set the UEFI memory region at the top of the DRAM
    UefiMemoryBase = SystemMemoryTop - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  } else {
    // In case of a non standalone UEFI, we set the UEFI memory below the Firmware Volume
    UefiMemoryBase = FixedPcdGet32 (PcdNormalFdBaseAddress) - FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  }
  UefiMemorySize = FixedPcdGet32 (PcdSystemMemoryUefiRegionSize);
  Status = PeiServicesInstallPeiMemory (UefiMemoryBase,UefiMemorySize);
  ASSERT_EFI_ERROR (Status);

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
  if (!FeaturePcdGet(PcdStandalone)) {
    // Check if firmware volume has not be copied at the top of DRAM then we must reserve the extra space
    // between the firmware and the top
    if (SystemMemoryTop != FixedPcdGet32 (PcdNormalFdBaseAddress) + FixedPcdGet32 (PcdNormalFdSize)) {
      BuildResourceDescriptorHob (
          EFI_RESOURCE_SYSTEM_MEMORY,
          Attributes & (~EFI_RESOURCE_ATTRIBUTE_TESTED),
          FixedPcdGet32 (PcdNormalFdBaseAddress) + FixedPcdGet32 (PcdNormalFdSize),
          SystemMemoryTop - (FixedPcdGet32 (PcdNormalFdBaseAddress) + FixedPcdGet32 (PcdNormalFdSize))
      );
    }

    // Reserved the memory space occupied by the firmware volume
    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        Attributes & (~EFI_RESOURCE_ATTRIBUTE_PRESENT),
        (UINT32)FixedPcdGet32 (PcdNormalFdBaseAddress),
        (UINT32)FixedPcdGet32 (PcdNormalFdSize)
    );
  }

  // Check there is no overlap between UEFI and Fix Address Regions
  ASSERT (FixedPcdGet32 (PcdSystemMemoryBase) + FixedPcdGet32 (PcdSystemMemoryFixRegionSize) <= UefiMemoryBase);

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
      FixedPcdGet32 (PcdSystemMemoryBase),
      FixedPcdGet32 (PcdSystemMemoryFixRegionSize)
  );

  // Reserved the memory between UEFI and Fix Address regions
  if (FixedPcdGet32 (PcdSystemMemoryBase) + FixedPcdGet32 (PcdSystemMemoryFixRegionSize) != UefiMemoryBase) {
    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        Attributes & (~EFI_RESOURCE_ATTRIBUTE_TESTED),
        FixedPcdGet32 (PcdSystemMemoryBase) + FixedPcdGet32 (PcdSystemMemoryFixRegionSize),
        UefiMemoryBase - (FixedPcdGet32 (PcdSystemMemoryBase) + FixedPcdGet32 (PcdSystemMemoryFixRegionSize))
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
