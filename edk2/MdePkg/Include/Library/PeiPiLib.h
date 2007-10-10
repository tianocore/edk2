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

#ifndef __PEI_PI_LIB_H__
#define __PEI_PI_LIB_H__

#include <Pi/PiFirmwareFile.h>

/**
    Install a EFI_PEI_FIRMWARE_VOLUME_INFO PPI to inform PEI core about the existence of a new Firmware Volume.
  
    The function allocate the EFI_PEI_PPI_DESCRIPTOR structure and update the fields accordingly to parameter passed
    in and install the PPI.
    
    @param  FvStart              Unique identifier of the format of the memory-mapped firmware volume. If NULL is specified,
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
  
    @retval  VOID
  
  **/

VOID
EFIAPI
PiLibInstallFvInfoPpi (
  IN EFI_GUID                *FvFormat, OPTIONAL
  IN VOID                    *FvInfo,
  IN UINT32                  FvInfoSize,
  IN EFI_GUID                *ParentFvName, OPTIONAL
  IN EFI_GUID                *PraentFileName OPTIONAL
);

#endif

