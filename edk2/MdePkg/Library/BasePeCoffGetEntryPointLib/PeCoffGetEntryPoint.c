/** @file
  Tiano PE/COFF loader.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PeCoffGetEntryPoint.c

**/



/**
  Loads a PE/COFF image into memory.

  @param  Pe32Data Pointer to a PE/COFF Image
  
  @param  EntryPoint Pointer to the entry point of the PE/COFF image

  @retval EFI_SUCCESS            if the EntryPoint was returned
  @retval EFI_INVALID_PARAMETER  if the EntryPoint could not be found from Pe32Data

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  )
{
  EFI_IMAGE_DOS_HEADER  *DosHeader;
  EFI_IMAGE_NT_HEADERS  *PeHeader;

  DosHeader = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
  if (DosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header
    //
    PeHeader = (EFI_IMAGE_NT_HEADERS *) ((UINTN) Pe32Data + (UINTN) ((DosHeader->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base
    //
    PeHeader = (EFI_IMAGE_NT_HEADERS *) Pe32Data;
  }
  *EntryPoint = (VOID *) ((UINTN) Pe32Data + (UINTN) (PeHeader->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff));
  return RETURN_SUCCESS;
}
