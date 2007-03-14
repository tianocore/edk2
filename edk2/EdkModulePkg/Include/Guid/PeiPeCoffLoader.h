/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
    PeiPeCoffLoader.h
    
Abstract:
  GUID for the PE/COFF Loader APIs shared between PEI and DXE

--*/

#ifndef __PEI_PE_COFF_LOADER_H__
#define __PEI_PE_COFF_LOADER_H__

//
// MdePkg/Include/Common/PeCoffLoaderImageContext.h
//
#include <Common/PeCoffLoaderImageContext.h>

#define EFI_PEI_PE_COFF_LOADER_GUID  \
  { 0xd8117cff, 0x94a6, 0x11d4, {0x9a, 0x3a, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

typedef struct _EFI_PEI_PE_COFF_LOADER_PROTOCOL   EFI_PEI_PE_COFF_LOADER_PROTOCOL;


typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_GET_IMAGE_INFO) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_LOAD_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_RELOCATE_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_UNLOAD_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

struct _EFI_PEI_PE_COFF_LOADER_PROTOCOL {
  EFI_PEI_PE_COFF_LOADER_GET_IMAGE_INFO  GetImageInfo;
  EFI_PEI_PE_COFF_LOADER_LOAD_IMAGE      LoadImage;
  EFI_PEI_PE_COFF_LOADER_RELOCATE_IMAGE  RelocateImage;
  EFI_PEI_PE_COFF_LOADER_UNLOAD_IMAGE    UnloadImage;
};

extern EFI_GUID gEfiPeiPeCoffLoaderGuid;

#endif
