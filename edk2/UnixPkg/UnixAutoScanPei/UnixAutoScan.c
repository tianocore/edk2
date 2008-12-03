/*++

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  UnixAutoscan.c

Abstract:
  This PEIM to abstract memory auto-scan in an Unix environment.

Revision History

--*/

#include "PiPei.h"
#include <Ppi/UnixAutoScan.h>
#include <Ppi/BaseMemoryTest.h>
#include <Ppi/MemoryDiscovered.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>

EFI_STATUS
EFIAPI
PeimInitializeUnixAutoScan (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
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
  PEI_UNIX_AUTOSCAN_PPI      *PeiUnixService;
  UINT64                      MemorySize;
  EFI_PHYSICAL_ADDRESS        MemoryBase;
  PEI_BASE_MEMORY_TEST_PPI    *MemoryTestPpi;
  EFI_PHYSICAL_ADDRESS        ErrorAddress;
  UINTN                       Index;
  EFI_RESOURCE_ATTRIBUTE_TYPE Attributes;


  DEBUG ((EFI_D_ERROR, "Unix Autoscan PEIM Loaded\n"));

  //
  // Get the PEI UNIX Autoscan PPI
  //
  Status = (**PeiServices).LocatePpi (
                            PeiServices,
                            &gPeiUnixAutoScanPpiGuid, // GUID
                            0,                      // INSTANCE
                            &PpiDescriptor,         // EFI_PEI_PPI_DESCRIPTOR
                            (VOID **)&PeiUnixService           // PPI
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
                            (VOID **)&MemoryTestPpi
                            );
  ASSERT_EFI_ERROR (Status);

  Index = 0;
  do {
    Status = PeiUnixService->UnixAutoScan (Index, &MemoryBase, &MemorySize);
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
