/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LoadFile.h
    
Abstract:

  Load image file from fv to memory. 

--*/

#ifndef _PEI_FV_FILE_LOADER_PPI_H
#define _PEI_FV_FILE_LOADER_PPI_H

#define EFI_PEI_FV_FILE_LOADER_GUID \
  { \
    0x7e1f0d85, 0x4ff, 0x4bb2, {0x86, 0x6a, 0x31, 0xa2, 0x99, 0x6a, 0x48, 0xa8} \
  }

EFI_FORWARD_DECLARATION (EFI_PEI_FV_FILE_LOADER_PPI);

typedef
EFI_STATUS
(EFIAPI *EFI_PEI_FV_LOAD_FILE) (
  IN EFI_PEI_FV_FILE_LOADER_PPI                 * This,
  IN  EFI_FFS_FILE_HEADER                       * FfsHeader,
  OUT EFI_PHYSICAL_ADDRESS                      * ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      * EntryPoint
  );

struct _EFI_PEI_FV_FILE_LOADER_PPI {
  EFI_PEI_FV_LOAD_FILE  FvLoadFile;
};

extern EFI_GUID gPeiFvFileLoaderPpiGuid;

#endif
