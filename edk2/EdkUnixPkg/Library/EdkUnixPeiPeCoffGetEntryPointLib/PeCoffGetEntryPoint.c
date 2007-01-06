/*++

Copyright (c) 2006, Intel Corporation
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


RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  )
/*++

Routine Description:

  Loads a PE/COFF image into memory

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
  UNIX_PEI_LOAD_FILE_PPI *PeiUnixService;
  EFI_PHYSICAL_ADDRESS    ImageAddress;
  UINT64                  ImageSize;
  EFI_PHYSICAL_ADDRESS    ImageEntryPoint;

  Status = PeiServicesLocatePpi (
             &gUnixPeiLoadFilePpiGuid,
             0,
             &PpiDescriptor,
             (void **)&PeiUnixService
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PeiUnixService->PeiLoadFileService (
                           Pe32Data,
                           &ImageAddress,
                           &ImageSize,
                           &ImageEntryPoint
                           );
  *EntryPoint = (VOID*)(UINTN)ImageEntryPoint;
  return Status;
}
