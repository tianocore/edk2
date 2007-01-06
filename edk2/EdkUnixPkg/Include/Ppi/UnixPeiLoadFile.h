/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 UnixPeiLoadFile.h

Abstract:

  Unix Load File PPI.

  When the PEI core is done it calls the DXE IPL via PPI

--*/

#ifndef __UNIX_PEI_LOAD_FILE_H__
#define __UNIX_PEI_LOAD_FILE_H__

#include <UnixDxe.h>

#define UNIX_PEI_LOAD_FILE_GUID \
  { \
    0xf2f48768, 0x8985, 0x11db, {0xb8, 0xda, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

typedef
EFI_STATUS
(EFIAPI *UNIX_PEI_LOAD_FILE) (
  VOID                  *Pe32Data,
  EFI_PHYSICAL_ADDRESS  *ImageAddress,
  UINT64                *ImageSize,
  EFI_PHYSICAL_ADDRESS  *EntryPoint
  );

/*++

Routine Description:
  Loads and relocates a PE/COFF image into memory.

Arguments:
  Pe32Data         - The base address of the PE/COFF file that is to be loaded and relocated
  ImageAddress     - The base address of the relocated PE/COFF image
  ImageSize        - The size of the relocated PE/COFF image
  EntryPoint       - The entry point of the relocated PE/COFF image

Returns:
  EFI_SUCCESS   - The file was loaded and relocated
  EFI_OUT_OF_RESOURCES - There was not enough memory to load and relocate the PE/COFF file

--*/
typedef struct {
  UNIX_PEI_LOAD_FILE  PeiLoadFileService;
} UNIX_PEI_LOAD_FILE_PPI;

extern EFI_GUID gUnixPeiLoadFilePpiGuid;

#endif
