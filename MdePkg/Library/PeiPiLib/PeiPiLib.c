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
#include <Library/BaseMemoryLib.h>


CONST EFI_PEI_FIRMWARE_VOLUME_INFO_PPI mFvInfoPpiTemplate = {
  EFI_FIRMWARE_FILE_SYSTEM2_GUID,
  NULL,
  0,    //FvInfoSize
  NULL, //ParentFvName
  NULL //ParentFileName;
};

/**
    Install a EFI_PEI_FIRMWARE_VOLUME_INFO PPI to inform PEI core about the existence of a new Firmware Volume.
  
    The function allocate the EFI_PEI_PPI_DESCRIPTOR structure and update the fields accordingly to parameter passed
    in and install the PPI.
    
    @param  FvFormat             Unique identifier of the format of the memory-mapped firmware volume. If NULL is specified,
                                         EFI_FIRMWARE_FILE_SYSTEM2_GUID is used as the Format GUID.
    @param  FvInfo               Points to a buffer which allows the EFI_PEI_FIRMWARE_VOLUME_PPI to
                                         process the volume. The format of this buffer is specific to the FvFormat. For
                                         memory-mapped firmware volumes, this typically points to the first byte of the
                                         firmware volume.
    @param  FvInfoSize          Size of the data provided by FvInfo. For memory-mapped firmware volumes, this is
                                         typically the size of the firmware volume.
    @param  ParentFvName, ParentFileName      If the firmware volume originally came from a firmware file, then these point to the
                                          parent firmware volume name and firmware volume file. If it did not originally come
                                          from a firmware file, these should be NULL
  
**/
VOID
EFIAPI
PiLibInstallFvInfoPpi (
  IN EFI_GUID                *FvFormat, OPTIONAL
  IN VOID                    *FvInfo,
  IN UINT32                  FvInfoSize,
  IN EFI_GUID                *ParentFvName, OPTIONAL
  IN EFI_GUID                *ParentFileName OPTIONAL
  )
{
  
  EFI_STATUS                       Status;   
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI *FvInfoPpi;
  EFI_PEI_PPI_DESCRIPTOR           *FvInfoPpiDescriptor;

  FvInfoPpi = AllocateCopyPool (sizeof (*FvInfoPpi), &mFvInfoPpiTemplate);
  ASSERT( FvInfoPpi != NULL);

  if (FvFormat != NULL) {
    CopyMem (&FvInfoPpi->FvFormat, FvFormat, sizeof (*FvFormat));
  }
  FvInfoPpi->FvInfo = (VOID *) (UINTN) FvInfo;
  FvInfoPpi->FvInfoSize = (UINT32) FvInfoSize;
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

