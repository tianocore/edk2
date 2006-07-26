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
  Retrieves and returns a pointer to the entry point to a PE/COFF image that has been loaded
  into system memory with the PE/COFF Loader Library functions.

  Retrieves the entry point to the PE/COFF image specified by Pe32Data and returns this entry
  point in EntryPoint.  If the entry point could not be retrieved from the PE/COFF image, then
  return RETURN_INVALID_PARAMETER.  Otherwise return RETURN_SUCCESS.
  If Pe32Data is NULL, then ASSERT().
  If EntryPoint is NULL, then ASSERT().

  @param  Pe32Data                  Pointer to the PE/COFF image that is loaded in system memory.
  @param  EntryPoint                Pointer to entry point to the PE/COFF image to return.

  @retval RETURN_SUCCESS            EntryPoint was returned.
  @retval RETURN_INVALID_PARAMETER  The entry point could not be found in the PE/COFF image.

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN  VOID  *Pe32Data,
  OUT VOID  **EntryPoint
  )
{
  EFI_IMAGE_DOS_HEADER                  *DosHeader;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Header;

  ASSERT (Pe32Data   != NULL);
  ASSERT (EntryPoint != NULL);

  DosHeader = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
  if (DosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Header.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN) Pe32Data + (UINTN) ((DosHeader->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Header.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
  }

  //
  // Calculate the entry point relative to the start of the image. 
  // AddressOfEntryPoint is common for PE32 & PE32+
  //
  *EntryPoint = (VOID *)((UINTN)Pe32Data + (UINTN)(Header.Pe32->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff));
  return RETURN_SUCCESS;
}


/**
  Returns the machine type of PE/COFF image. 

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


