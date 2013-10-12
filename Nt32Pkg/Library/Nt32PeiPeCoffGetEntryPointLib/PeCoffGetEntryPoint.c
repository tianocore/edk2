/**@file

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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

**/

#include <PiPei.h>
#include <IndustryStandard/PeImage.h>
#include <WinNtPeim.h>
#include <Ppi/NtPeiLoadFile.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>


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

  ASSERT (Pe32Data   != NULL);
  ASSERT (EntryPoint != NULL);

  Status = PeiServicesLocatePpi (
             &gNtPeiLoadFilePpiGuid,
             0,
             &PpiDescriptor,
             (VOID**)&PeiNtService
             );
  ASSERT_EFI_ERROR (Status);

  Status = PeiNtService->PeiLoadFileService (
                           Pe32Data,
                           &ImageAddress,
                           &ImageSize,
                           &ImageEntryPoint
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *EntryPoint = (VOID*)(UINTN)ImageEntryPoint;
  return Status;
}

/**
  Returns the machine type of PE/COFF image. 
  This is copied from MDE BasePeCoffGetEntryPointLib, the code should be sync with it.
  The reason is NT32 package needs to load the image to memory to support source
  level debug.
   

  @param  Pe32Data   Pointer to a PE/COFF header

  @return            Machine type or zero if not a valid iamge

**/
UINT16
EFIAPI
PeCoffLoaderGetMachineType (
  IN  VOID  *Pe32Data
  )
{  
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;

  ASSERT (Pe32Data   != NULL);

  DosHdr = (EFI_IMAGE_DOS_HEADER  *)Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
  }

  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    return Hdr.Te->Machine;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE)  {
    return Hdr.Pe32->FileHeader.Machine;
  }

  return 0x0000;
}

/**
  Returns a pointer to the PDB file name for a PE/COFF image that has been
  loaded into system memory with the PE/COFF Loader Library functions.

  Returns the PDB file name for the PE/COFF image specified by Pe32Data.  If
  the PE/COFF image specified by Pe32Data is not a valid, then NULL is
  returned.  If the PE/COFF image specified by Pe32Data does not contain a
  debug directory entry, then NULL is returned.  If the debug directory entry
  in the PE/COFF image specified by Pe32Data does not contain a PDB file name,
  then NULL is returned.
  If Pe32Data is NULL, then ASSERT().

  @param  Pe32Data   Pointer to the PE/COFF image that is loaded in system
                     memory.

  @return The PDB file name for the PE/COFF image specified by Pe32Data or NULL
          if it cannot be retrieved.

**/
VOID *
EFIAPI
PeCoffLoaderGetPdbPointer (
  IN VOID  *Pe32Data
  )
{
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  EFI_IMAGE_DATA_DIRECTORY              *DirectoryEntry;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY       *DebugEntry;
  UINTN                                 DirCount;
  VOID                                  *CodeViewEntryPointer;
  INTN                                  TEImageAdjust;
  UINT32                                NumberOfRvaAndSizes;
  UINT16                                Magic;

  ASSERT (Pe32Data   != NULL);

  TEImageAdjust       = 0;
  DirectoryEntry      = NULL;
  DebugEntry          = NULL;
  NumberOfRvaAndSizes = 0;

  DosHdr = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
  }

  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    if (Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress != 0) {
      DirectoryEntry  = &Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG];
      TEImageAdjust   = sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize;
      DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *)((UINTN) Hdr.Te +
                    Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress +
                    TEImageAdjust);
    }
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
    //
    // NOTE: We use Machine field to identify PE32/PE32+, instead of Magic.
    //       It is due to backward-compatibility, for some system might
    //       generate PE32+ image with PE32 Magic.
    //
    switch (Hdr.Pe32->FileHeader.Machine) {
    case IMAGE_FILE_MACHINE_I386:
      //
      // Assume PE32 image with IA32 Machine field.
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
      break;
    case IMAGE_FILE_MACHINE_X64:
    case IMAGE_FILE_MACHINE_IA64:
      //
      // Assume PE32+ image with X64 or IA64 Machine field
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
      break;
    default:
      //
      // For unknow Machine field, use Magic in optional Header
      //
      Magic = Hdr.Pe32->OptionalHeader.Magic;
    }

    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset get Debug Directory Entry
      //
      NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
      DebugEntry     = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) ((UINTN) Pe32Data + DirectoryEntry->VirtualAddress);
    } else if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      //
      // Use PE32+ offset get Debug Directory Entry
      //
      NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
      DebugEntry     = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) ((UINTN) Pe32Data + DirectoryEntry->VirtualAddress);
    }

    if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
      DirectoryEntry = NULL;
      DebugEntry = NULL;
    }
  } else {
    return NULL;
  }

  if (DebugEntry == NULL || DirectoryEntry == NULL) {
    return NULL;
  }

  for (DirCount = 0; DirCount < DirectoryEntry->Size; DirCount += sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY), DebugEntry++) {
    if (DebugEntry->Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
      if (DebugEntry->SizeOfData > 0) {
        CodeViewEntryPointer = (VOID *) ((UINTN) DebugEntry->RVA + ((UINTN)Pe32Data) + (UINTN)TEImageAdjust);
        switch (* (UINT32 *) CodeViewEntryPointer) {
        case CODEVIEW_SIGNATURE_NB10:
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY));
        case CODEVIEW_SIGNATURE_RSDS:
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY));
        case CODEVIEW_SIGNATURE_MTOC:              
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY));
          break;
        default:
          break;
        }
      }
    }
  }

  return NULL;
}

/**
  Returns the size of the PE/COFF headers

  Returns the size of the PE/COFF header specified by Pe32Data.
  If Pe32Data is NULL, then ASSERT().

  @param  Pe32Data   Pointer to the PE/COFF image that is loaded in system
                     memory.

  @return Size of PE/COFF header in bytes or zero if not a valid image.

**/
UINT32
EFIAPI
PeCoffGetSizeOfHeaders (
  IN VOID     *Pe32Data
  )
{
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  UINTN                                 SizeOfHeaders;

  ASSERT (Pe32Data   != NULL);
 
  DosHdr = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
  }

  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    SizeOfHeaders = sizeof (EFI_TE_IMAGE_HEADER) + (UINTN)Hdr.Te->BaseOfCode - (UINTN)Hdr.Te->StrippedSize;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
    SizeOfHeaders = Hdr.Pe32->OptionalHeader.SizeOfHeaders;
  } else {
    SizeOfHeaders = 0;
  }

  return (UINT32) SizeOfHeaders;
}

