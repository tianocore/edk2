/**@file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  WinNtAutoscan.c

Abstract:
  This PEIM to abstract memory auto-scan in a Windows NT environment.

Revision History

**/

//
// The package level header files this module uses
//
#include <PiPei.h>
#include <WinNtPeim.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/BaseMemoryTest.h>
#include <Ppi/NtAutoscan.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>

EFI_STATUS
EFIAPI
PeimInitializeWinNtAutoScan (
  IN EFI_FFS_FILE_HEADER       *FfsHeader,
  IN EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:
  Perform a call-back into the SEC simulator to get a memory value

Arguments:
  FfsHeader   - General purpose data available to every PEIM
  PeiServices - General purpose services available to every PEIM.
    
Returns:
  None

--*/
{
  EFI_STATUS                  Status;
  EFI_PEI_PPI_DESCRIPTOR      *PpiDescriptor;
  PEI_NT_AUTOSCAN_PPI         *PeiNtService;
  UINT64                      MemorySize;
  EFI_PHYSICAL_ADDRESS        MemoryBase;
  PEI_BASE_MEMORY_TEST_PPI    *MemoryTestPpi;
  EFI_PHYSICAL_ADDRESS        ErrorAddress;
  UINTN                       Index;
  EFI_RESOURCE_ATTRIBUTE_TYPE Attributes;


  DEBUG ((EFI_D_ERROR, "NT 32 Autoscan PEIM Loaded\n"));

  //
  // Get the PEI NT Autoscan PPI
  //
  Status = (**PeiServices).LocatePpi (
                            PeiServices,
                            &gPeiNtAutoScanPpiGuid, // GUID
                            0,                      // INSTANCE
                            &PpiDescriptor,         // EFI_PEI_PPI_DESCRIPTOR
                            &PeiNtService           // PPI
                            );
  ASSERT_EFI_ERROR (Status);

  //
  // Get the Memory Test PPI
  //
  Status = (**PeiServices).LocatePpi (
                            PeiServices,
                            &gPeiBaseMemoryTestPpiGuid,
                            0,
                            NULL,
                            &MemoryTestPpi
                            );
  ASSERT_EFI_ERROR (Status);

  Index = 0;
  do {
    Status = PeiNtService->NtAutoScan (Index, &MemoryBase, &MemorySize);
    if (!EFI_ERROR (Status)) {
      Attributes =
        (
          EFI_RESOURCE_ATTRIBUTE_PRESENT |
          EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
          EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
          EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
          EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
          EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE
        );

      if (Index == 0) {
        //
        // For the first area register it as PEI tested memory
        //
        Status = MemoryTestPpi->BaseMemoryTest (
                                  PeiServices,
                                  MemoryTestPpi,
                                  MemoryBase,
                                  MemorySize,
                                  Quick,
                                  &ErrorAddress
                                  );
        ASSERT_EFI_ERROR (Status);

        //
        // Register the "tested" memory with the PEI Core
        //
        Status = (**PeiServices).InstallPeiMemory (PeiServices, MemoryBase, MemorySize);
        ASSERT_EFI_ERROR (Status);

        Attributes |= EFI_RESOURCE_ATTRIBUTE_TESTED;
      }
      
      BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        Attributes,
        MemoryBase,
        MemorySize
        );
    }
    Index++;
  } while (!EFI_ERROR (Status));

  //
  // Build the CPU hob with 36-bit addressing and 16-bits of IO space.
  //
  BuildCpuHob (36, 16);
  
  return Status;
}
