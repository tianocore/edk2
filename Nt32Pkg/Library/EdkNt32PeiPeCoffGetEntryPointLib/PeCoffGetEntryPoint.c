/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeCoffGetEntryPoint.c

Abstract:

  Tiano PE/COFF loader

Revision History

--*/


//
// Include common header file for this module.
//
#include "CommonHeader.h"

RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  )
/*++

Routine Description:

  Loads a PE/COFF image into memory, this is not follow the original purpose of 
  PeCoffGetEntryPoint library class.  But it's ok that Unix package not run on a real 
  platform and this is for source level debug.

Arguments:

  Pe32Data   - Pointer to a PE/COFF Image

  EntryPoint - Pointer to the entry point of the PE/COFF image

Returns:

  EFI_SUCCESS            if the EntryPoint was returned
  EFI_INVALID_PARAMETER  if the EntryPoint could not be found from Pe32Data

--*/
{
  EFI_STATUS              Status;
  EFI_PEI_PPI_DESCRIPTOR  *PpiDescriptor;
  NT_PEI_LOAD_FILE_PPI    *PeiNtService;
  EFI_PHYSICAL_ADDRESS    ImageAddress;
  UINT64                  ImageSize;
  EFI_PHYSICAL_ADDRESS    ImageEntryPoint;

  Status = PeiServicesLocatePpi (
             &gNtPeiLoadFilePpiGuid,
             0,
             &PpiDescriptor,
             &PeiNtService
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeiNtService->PeiLoadFileService (
                           Pe32Data,
                           &ImageAddress,
                           &ImageSize,
                           &ImageEntryPoint
                           );
  *EntryPoint = (VOID*)(UINTN)ImageEntryPoint;
  return Status;
}

/**
  Returns the machine type of PE/COFF image. 
  This is copied from MDE BasePeCoffGetEntryPointLib, the code should be sync with it.
  The reason is NT32 package needs to load the image to memory to support source
  level debug.
   

  @param  Image   Pointer to a PE/COFF header

  @return         Machine type or zero if not a valid iamge

**/
UINT16
EFIAPI
PeCoffLoaderGetMachineType (
  IN  VOID  *Pe32Data
  )
{  
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;

  DosHdr = (EFI_IMAGE_DOS_HEADER  *)Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)Pe32Data + DosHdr->e_lfanew);
  } else {
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)Pe32Data);
  }

  if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE)  {
    return Hdr.Pe32->FileHeader.Machine;
  }

  return 0x0000;
}

