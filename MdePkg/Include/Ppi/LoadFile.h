/** @file
  Load image file from fv to memory. 

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  LoadFile.h

  @par Revision Reference:
  This PPI is defined in PEI CIS spec Version 0.91.

**/

#ifndef __FV_FILE_LOADER_PPI_H__
#define __FV_FILE_LOADER_PPI_H__

#define EFI_PEI_FV_FILE_LOADER_GUID \
  { \
    0x7e1f0d85, 0x4ff, 0x4bb2, {0x86, 0x6a, 0x31, 0xa2, 0x99, 0x6a, 0x48, 0xa8 } \
  }

typedef struct _EFI_PEI_FV_FILE_LOADER_PPI  EFI_PEI_FV_FILE_LOADER_PPI;

/**
  Loads a PEIM into memory for subsequent execution.

  @param  This           Interface pointer that implements the Load File PPI instance.
  @param  FfsHeader      Pointer to the FFS header of the file to load.
  @param  ImageAddress   Pointer to the address of the loaded Image
  @param  ImageSize      Pointer to the size of the loaded image.
  @param  EntryPoint     Pointer to the entry point of the image.

  @retval EFI_SUCCESS           The image was loaded successfully.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory.
  @retval EFI_INVALID_PARAMETER The contents of the FFS file did not
                                contain a valid PE/COFF image that could be loaded.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FV_LOAD_FILE) (
  IN EFI_PEI_FV_FILE_LOADER_PPI                 *This,
  IN  EFI_FFS_FILE_HEADER                       *FfsHeader,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  );

/**
  @par Ppi Description:
  This PPI is a pointer to the Load File service. This service will be 
  published by a PEIM.The PEI Foundation will use this service to 
  launch the known non-XIP PE/COFF PEIM images.  This service may 
  depend upon the presence of the EFI_PEI_PERMANENT_MEMORY_INSTALLED_PPI.

  @param FvLoadFile
  Loads a PEIM into memory for subsequent execution

**/
struct _EFI_PEI_FV_FILE_LOADER_PPI {
  EFI_PEI_FV_LOAD_FILE  FvLoadFile;
};

extern EFI_GUID gEfiPeiFvFileLoaderPpiGuid;

#endif
