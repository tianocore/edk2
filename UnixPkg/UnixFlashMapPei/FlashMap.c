/*++

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FlashMap.c
   
Abstract:

  PEIM to build GUIDed HOBs for platform specific flash map

--*/


#include "PiPei.h"

#include <Guid/SystemNvDataGuid.h>
#include <Guid/FirmwareFileSystem.h>
#include <Ppi/UnixFwh.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>

EFI_STATUS
EFIAPI
PeimInitializeFlashMap (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
/*++

Routine Description:
  Build GUIDed HOBs for platform specific flash map
  
Arguments:
  FfsHeader   - A pointer to the EFI_FFS_FILE_HEADER structure.
  PeiServices - General purpose services available to every PEIM.
    
Returns:
  EFI_STATUS

--*/
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_STATUS              Status;
  UNIX_FWH_PPI           *UnixFwhPpi;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  EFI_PHYSICAL_ADDRESS    FdBase;
  UINT64                  FdSize;

  DEBUG ((EFI_D_ERROR, "NT 32 Flash Map PEIM Loaded\n"));

  //
  // Get the Fwh Information PPI
  //
  Status = PeiServicesLocatePpi (
            &gUnixFwhPpiGuid, // GUID
            0,              // INSTANCE
            &PpiDescriptor, // EFI_PEI_PPI_DESCRIPTOR
            (VOID **)&UnixFwhPpi       // PPI
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Assume that FD0 contains the Flash map.
  //
  Status = UnixFwhPpi->UnixFwh (0, &FdBase, &FdSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  PcdSet32 (PcdFlashNvStorageVariableBase, PcdGet32 (PcdUnixFlashNvStorageVariableBase) + (UINT32) FdBase);
  PcdSet32 (PcdFlashNvStorageFtwWorkingBase, PcdGet32 (PcdUnixFlashNvStorageFtwWorkingBase) + (UINT32) FdBase);
  PcdSet32 (PcdFlashNvStorageFtwSpareBase, PcdGet32 (PcdUnixFlashNvStorageFtwSpareBase) + (UINT32) FdBase);

  return EFI_SUCCESS;
}
