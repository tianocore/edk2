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
#include <Library/ArmPlatformLib.h>

//
// Module globals
//

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

// May want to put this into a library so you only need the PCD setings if you are using the feature?
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

EFI_STATUS
EFIAPI
InitializeMemory (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
/*++

Routine Description:

  

Arguments:

  FileHandle  - Handle of the file being invoked.
  PeiServices - Describes the list of possible PEI Services.
    
Returns:

  Status -  EFI_SUCCESS if the boot mode could be set

--*/
{
  EFI_STATUS                    Status;
  ARM_SYSTEM_MEMORY_REGION_DESCRIPTOR* EfiMemoryMap;
  UINTN                         PeiMemoryBase;
  UINTN                         PeiMemorySize;
  UINTN                         Index;

  DEBUG ((EFI_D_ERROR, "Memory Init PEIM Loaded\n"));

  // If it is not a standalone version, then we need to initialize the System Memory
  // In case of a standalone version, the DRAM is already initialized
  if (FeaturePcdGet(PcdStandalone)) {
    // Initialize the System Memory controller (DRAM)
    ArmPlatformInitializeSystemMemory();
  }

  // Install the Memory to PEI
  ArmPlatformGetPeiMemory (&PeiMemoryBase,&PeiMemorySize);
  Status = PeiServicesInstallPeiMemory (PeiMemoryBase,PeiMemorySize);
  ASSERT_EFI_ERROR (Status);

  //
  // Now, the permanent memory has been installed, we can call AllocatePages()
  //

  ArmPlatformGetEfiMemoryMap (&EfiMemoryMap);

  // Install the EFI Memory Map
  for (Index = 0; EfiMemoryMap[Index].ResourceAttribute != 0; Index++) {
    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        EfiMemoryMap[Index].ResourceAttribute,
        EfiMemoryMap[Index].PhysicalStart,
        EfiMemoryMap[Index].NumberOfBytes
    );
  }

  // Build Memory Allocation Hob
  InitMmu ();

  if (FeaturePcdGet (PcdPrePiProduceMemoryTypeInformationHob)) {
    // Optional feature that helps prevent EFI memory map fragmentation. 
    BuildMemoryTypeInformationHob ();
  }

  return Status;
}
