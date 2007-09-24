/** @file
  MDE PI library functions and macros for PEI phase

  Copyright (c) 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PiPei.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Guid/FirmwareFileSystem2.h>


#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiPiLib.h>


STATIC CONST EFI_PEI_FIRMWARE_VOLUME_INFO_PPI mFvInfoPpiTemplate = {
  EFI_FIRMWARE_FILE_SYSTEM2_GUID,
  NULL,
  0,    //FvInfoSize
  NULL, //ParentFvName
  NULL //ParentFileName;
};

VOID
EFIAPI
PeiPiLibBuildPiFvInfoPpi (
  IN EFI_PHYSICAL_ADDRESS    FvStart,
  IN UINT64                  FvLength,
  IN EFI_GUID                *ParentFvName,
  IN EFI_GUID                *ParentFileName
  ) {
  
  EFI_STATUS                       Status;   
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI *FvInfoPpi;
  EFI_PEI_PPI_DESCRIPTOR           *FvInfoPpiDescriptor;

  FvInfoPpi = AllocateCopyPool (sizeof (*FvInfoPpi), &mFvInfoPpiTemplate);
  ASSERT( FvInfoPpi != NULL);

  FvInfoPpi->FvInfo = (VOID *) (UINTN) FvStart;
  FvInfoPpi->FvInfoSize = (UINT32) FvLength;
  FvInfoPpi->ParentFvName = ParentFvName;
  FvInfoPpi->ParentFileName = ParentFileName;


  FvInfoPpiDescriptor = AllocatePool (sizeof(EFI_PEI_PPI_DESCRIPTOR));
  ASSERT (FvInfoPpiDescriptor != NULL);

  FvInfoPpiDescriptor->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  FvInfoPpiDescriptor->Guid  = &gEfiPeiFirmwareVolumeInfoPpiGuid;
  FvInfoPpiDescriptor->Ppi   = (VOID *) FvInfoPpi;
  Status = PeiServicesInstallPpi (FvInfoPpiDescriptor);
  ASSERT_EFI_ERROR (Status);

   
}

