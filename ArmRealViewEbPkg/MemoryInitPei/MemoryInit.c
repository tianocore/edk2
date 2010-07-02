/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  MemoryInit.c
   
Abstract:

  PEIM to provide fake memory init

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
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>


//
// Module globals
//

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
  EFI_RESOURCE_ATTRIBUTE_TYPE   Attributes;
  UINT64                        MemoryBase;
  UINT64                        MemorySize;

  DEBUG ((EFI_D_ERROR, "Memory Init PEIM Loaded\n"));

  // NOTE: this needs to come from your memory controller initization process
  MemoryBase = 0;
  MemorySize = 0x10000000;

  DEBUG ((EFI_D_ERROR, "Installing hardcoded 256MB\n"));
  Status = PeiServicesInstallPeiMemory (MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);
  
  Attributes =
    (
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED
    );
  
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    Attributes,
    MemoryBase,
    MemorySize
    );

  return Status;
}
