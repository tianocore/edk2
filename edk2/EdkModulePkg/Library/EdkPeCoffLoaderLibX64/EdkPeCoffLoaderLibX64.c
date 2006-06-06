/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdkPeCoffLoaderX64.c

Abstract:

  Wrap the Base PE/COFF loader with the PE COFF Protocol


--*/

#define IMAGE_64_MACHINE_TYPE_SUPPORTED(Machine) \
  ((Machine) == EFI_IMAGE_MACHINE_IA32 || \
   (Machine) == EFI_IMAGE_MACHINE_X64 || \
   (Machine) == EFI_IMAGE_MACHINE_EBC)

STATIC
EFI_STATUS
PeCoffLoader64GetPeHeader (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  OUT    EFI_IMAGE_NT_HEADERS64                  *PeHdr
  );

STATIC
EFI_STATUS
PeCoffLoader64CheckImageType (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN     EFI_IMAGE_NT_HEADERS64                  *PeHdr
  );

STATIC
VOID *
PeCoffLoader64ImageAddress (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN     UINTN                                 Address
  );

EFI_STATUS
EFIAPI
PeCoffLoader64GetImageInfo (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

EFI_STATUS
EFIAPI
PeCoffLoader64RelocateImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

EFI_STATUS
EFIAPI
PeCoffLoader64LoadImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

EFI_STATUS
EFIAPI
PeCoffLoader64UnloadImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL      *This,
  IN PE_COFF_LOADER_IMAGE_CONTEXT   *ImageContext
  );

EFI_STATUS
PeCoffLoader64RelocateImageEx (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup, 
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  );

EFI_PEI_PE_COFF_LOADER_PROTOCOL mPeCoffLoaderX64 = {
  PeCoffLoader64GetImageInfo,
  PeCoffLoader64LoadImage,
  PeCoffLoader64RelocateImage,
  PeCoffLoader64UnloadImage
};

STATIC
EFI_STATUS
PeCoffLoader64GetPeHeader (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  OUT    EFI_IMAGE_NT_HEADERS64                *PeHdr
  )
/*++

Routine Description:
  Retrieves the PE Header from a PE/COFF image

Arguments:
  ImageContext  - The context of the image being loaded
  PeHdr         - The buffer in which to return the PE header

Returns:
  EFI_SUCCESS if the PE Header is read, 
  Otherwise, the error status from reading the PE/COFF image using the ImageRead function.

--*/
{
  EFI_STATUS            Status;
  EFI_IMAGE_DOS_HEADER  DosHdr;
  UINTN                 Size;
  
  //
  // Read the DOS image headers
  //
  Size = sizeof (EFI_IMAGE_DOS_HEADER);
  Status = ImageContext->ImageRead (
                           ImageContext->Handle, 
                           0,
                           &Size, 
                           &DosHdr
                           );
  if (EFI_ERROR (Status)) {
    ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
    return Status;
  }

  ImageContext->PeCoffHeaderOffset = 0;
  if (DosHdr.e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header
    //
    ImageContext->PeCoffHeaderOffset = DosHdr.e_lfanew;
  } 

  //
  // Read the PE/COFF Header
  //
  Size = sizeof (EFI_IMAGE_NT_HEADERS64);
  Status = ImageContext->ImageRead (
                           ImageContext->Handle, 
                           ImageContext->PeCoffHeaderOffset, 
                           &Size, 
                           PeHdr
                           );
  if (EFI_ERROR (Status)) {
      ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
      return Status;
  }

  return EFI_SUCCESS;
}

static
EFI_STATUS
PeCoffLoader64CheckImageType (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext,
  IN     EFI_IMAGE_NT_HEADERS64                    *PeHdr
  )
/*++

Routine Description:
  Checks the PE header of a PE/COFF image to determine if it supported

Arguments:
  ImageContext  - The context of the image being loaded
  PeHdr         - The buffer in which to return the PE header

Returns:
  EFI_SUCCESS if the PE/COFF image is supported
  EFI_UNSUPPORTED of the PE/COFF image is not supported.

--*/
{
  //
  // Check the PE/COFF Header SIgnature
  //
  if (PeHdr->Signature != EFI_IMAGE_NT_SIGNATURE) {
    ImageContext->ImageError = IMAGE_ERROR_INVALID_PE_HEADER_SIGNATURE;
    return EFI_UNSUPPORTED;
  }

  //
  // See if the machine type is supported.  We support a native machine type (IA-32/Itanium-based)
  // and the machine type for the Virtual Machine.
  //
  ImageContext->Machine = PeHdr->FileHeader.Machine;
  if (!(IMAGE_64_MACHINE_TYPE_SUPPORTED (ImageContext->Machine))) {
    ImageContext->ImageError = IMAGE_ERROR_INVALID_MACHINE_TYPE;
    return EFI_UNSUPPORTED;
  }

  //
  // See if the image type is supported.  We support EFI Applications, 
  // EFI Boot Service Drivers, and EFI Runtime Drivers.
  //
  ImageContext->ImageType = PeHdr->OptionalHeader.Subsystem;
  switch (ImageContext->ImageType) {

  case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
    ImageContext->ImageCodeMemoryType = EfiLoaderCode;
    ImageContext->ImageDataMemoryType = EfiLoaderData;
    break;
  
  case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
    ImageContext->ImageCodeMemoryType = EfiBootServicesCode;
    ImageContext->ImageDataMemoryType = EfiBootServicesData;
    break;
  
  case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
  case EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER:
    ImageContext->ImageCodeMemoryType = EfiRuntimeServicesCode;
    ImageContext->ImageDataMemoryType = EfiRuntimeServicesData;
    break;
  
  default:
    ImageContext->ImageError = IMAGE_ERROR_INVALID_SUBSYSTEM;
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeCoffLoader64GetImageInfo (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL           *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext
  )
/*++

Routine Description:
  Retrieves information on a PE/COFF image

Arguments:
  ImageContext  - The context of the image being loaded
  PeHdr         - The buffer in which to return the PE header

Returns:
  EFI_SUCCESS if the information on the PE/COFF image was collected.
  EFI_UNSUPPORTED of the PE/COFF image is not supported.
  Otherwise, the error status from reading the PE/COFF image using the 
    ImageContext->ImageRead() function

--*/
{
  EFI_STATUS                      Status;
  EFI_IMAGE_NT_HEADERS64          PeHdr;
  EFI_IMAGE_DATA_DIRECTORY        *DebugDirectoryEntry;
  UINTN                           Size;
  UINTN                           Index;
  UINTN                           DebugDirectoryEntryRva;
  UINTN                           DebugDirectoryEntryFileOffset;
  UINTN                           SectionHeaderOffset;
  EFI_IMAGE_SECTION_HEADER        SectionHeader;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY DebugEntry;

  if (NULL == ImageContext) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Assume success
  //
  ImageContext->ImageError = IMAGE_ERROR_SUCCESS;
  
  Status = PeCoffLoader64GetPeHeader (ImageContext, &PeHdr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Verify machine type
  //
  Status = PeCoffLoader64CheckImageType (ImageContext, &PeHdr);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // Retrieve the base address of the image
  //
  ImageContext->ImageAddress = PeHdr.OptionalHeader.ImageBase;
  
  //
  // Initialize the alternate destination address to 0 indicating that it 
  // should not be used.
  //
  ImageContext->DestinationAddress = 0;

  //
  // Initialize the codeview pointer.
  //
  ImageContext->CodeView   = NULL;
  ImageContext->PdbPointer = NULL;
  
  //
  // Three cases with regards to relocations:
  // - Image has base relocs, RELOCS_STRIPPED==0    => image is relocatable
  // - Image has no base relocs, RELOCS_STRIPPED==1 => Image is not relocatable
  // - Image has no base relocs, RELOCS_STRIPPED==0 => Image is relocatable but
  //   has no base relocs to apply
  // Obviously having base relocations with RELOCS_STRIPPED==1 is invalid.
  //
  // Look at the file header to determine if relocations have been stripped, and
  // save this info in the image context for later use.
  //
  if (PeHdr.FileHeader.Characteristics &  EFI_IMAGE_FILE_RELOCS_STRIPPED) {
    ImageContext->RelocationsStripped = TRUE;
  } else {
    ImageContext->RelocationsStripped = FALSE;
  }

  ImageContext->ImageSize        = (UINT64)PeHdr.OptionalHeader.SizeOfImage;
  ImageContext->SectionAlignment = PeHdr.OptionalHeader.SectionAlignment;
  ImageContext->SizeOfHeaders    = PeHdr.OptionalHeader.SizeOfHeaders; 

  //
  // Modify ImageSize to contain .PDB file name if required and initialize
  // PdbRVA field...
  //

  if (PeHdr.OptionalHeader.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
    DebugDirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)
      &(PeHdr.OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);

    DebugDirectoryEntryRva = DebugDirectoryEntry->VirtualAddress;

    //
    // Determine the file offset of the debug directory...  This means we walk
    // the sections to find which section contains the RVA of the debug
    // directory
    //
    
    DebugDirectoryEntryFileOffset = 0;
    
    SectionHeaderOffset = (UINTN) (
                      ImageContext->PeCoffHeaderOffset +
                      sizeof (UINT32) + 
                      sizeof (EFI_IMAGE_FILE_HEADER) + 
                      PeHdr.FileHeader.SizeOfOptionalHeader
                      );
    
    for (Index = 0; Index < PeHdr.FileHeader.NumberOfSections; Index += 1) {
      //
      // Read section header from file
      //
      Size = sizeof (EFI_IMAGE_SECTION_HEADER);
      Status = ImageContext->ImageRead (
                               ImageContext->Handle, 
                               SectionHeaderOffset, 
                               &Size, 
                               &SectionHeader
                               );
      if (EFI_ERROR (Status)) {
        ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
        return Status;
      }
      
      if (DebugDirectoryEntryRva >= SectionHeader.VirtualAddress &&
          DebugDirectoryEntryRva < SectionHeader.VirtualAddress + SectionHeader.Misc.VirtualSize) {
        DebugDirectoryEntryFileOffset = DebugDirectoryEntryRva - SectionHeader.VirtualAddress + SectionHeader.PointerToRawData;
        break;
      }
      
      SectionHeaderOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
    }
    
    if (DebugDirectoryEntryFileOffset != 0) {
      for (Index = 0; Index < DebugDirectoryEntry->Size; Index++) {
        //
        // Read next debug directory entry
        //
        Size = sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY);
        Status = ImageContext->ImageRead (
                                 ImageContext->Handle, 
                                 DebugDirectoryEntryFileOffset, 
                                 &Size, 
                                 &DebugEntry
                                 );
        if (EFI_ERROR (Status)) {
          ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
          return Status;
        }
        if (DebugEntry.Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
          ImageContext->DebugDirectoryEntryRva = (UINT32) (DebugDirectoryEntryRva + Index * sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY));
          if (DebugEntry.RVA == 0 && DebugEntry.FileOffset != 0) {
            ImageContext->ImageSize += DebugEntry.SizeOfData;
          }
          return EFI_SUCCESS;
        }
      }
    }
  }
  return EFI_SUCCESS;
}

static
VOID *
PeCoffLoader64ImageAddress (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext,
  IN     UINTN                                     Address
  )
/*++

Routine Description:
  Converts an image address to the loaded address

Arguments:
  ImageContext  - The context of the image being loaded
  Address       - The address to be converted to the loaded address

Returns:
  NULL if the address can not be converted, otherwise, the converted address

--*/
{
  if (Address >= ImageContext->ImageSize) {
    ImageContext->ImageError = IMAGE_ERROR_INVALID_IMAGE_ADDRESS;
    return NULL;
  }
  return (CHAR8 *)((UINTN)ImageContext->ImageAddress + Address);
}

EFI_STATUS
EFIAPI
PeCoffLoader64RelocateImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL           *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext
  )
/*++

Routine Description:
  Relocates a PE/COFF image in memory

Arguments:
  ImageContext - Contains information on the loaded image to relocate

Returns:
  EFI_SUCCESS    if the PE/COFF image was relocated
  EFI_LOAD_ERROR if the image is not a valid PE/COFF image

--*/
{
  EFI_STATUS                 Status;
  EFI_IMAGE_NT_HEADERS64     *PeHdr;
  EFI_IMAGE_DATA_DIRECTORY   *RelocDir;
  IN UINT64                  Adjust;
  EFI_IMAGE_BASE_RELOCATION  *RelocBase;
  EFI_IMAGE_BASE_RELOCATION  *RelocBaseEnd;
  UINT16                     *Reloc;
  UINT16                     *RelocEnd;
  CHAR8                      *Fixup;
  CHAR8                      *FixupBase;
  UINT16                     *F16;
  UINT32                     *F32;
  CHAR8                      *FixupData;
  EFI_PHYSICAL_ADDRESS       BaseAddress;


  //
  // Assume success
  //
  ImageContext->ImageError = IMAGE_ERROR_SUCCESS;

  //
  // If there are no relocation entries, then we are done
  //
  if (ImageContext->RelocationsStripped) {
    return EFI_SUCCESS;
  }

  //
  // If the destination address is not 0, use that rather than the
  // image address as the relocation target.
  //
  if (ImageContext->DestinationAddress) {
    BaseAddress = ImageContext->DestinationAddress;
  } else {
    BaseAddress = ImageContext->ImageAddress;
  }
  PeHdr    = (EFI_IMAGE_NT_HEADERS64 *)((UINTN)ImageContext->ImageAddress + 
                                            ImageContext->PeCoffHeaderOffset);
  Adjust   = (UINT64) BaseAddress - PeHdr->OptionalHeader.ImageBase;

  PeHdr->OptionalHeader.ImageBase = (UINTN) BaseAddress;

  //
  // Find the relocation block
  //
  // Per the PE/COFF spec, you can't assume that a given data directory 
  // is present in the image. You have to check the NumberOfRvaAndSizes in 
  // the optional header to verify a desired directory entry is there.
  //
  if (PeHdr->OptionalHeader.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
    RelocDir = &PeHdr->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    RelocBase = PeCoffLoader64ImageAddress (ImageContext, RelocDir->VirtualAddress);
    RelocBaseEnd = PeCoffLoader64ImageAddress ( 
                    ImageContext, 
                    RelocDir->VirtualAddress + RelocDir->Size - 1
                    );
} else {
    //
    // Set base and end to bypass processing below.
    //
    RelocBase = RelocBaseEnd = 0;
  }
  //
  // Run the relocation information and apply the fixups
  //
  FixupData = ImageContext->FixupData;
  while (RelocBase < RelocBaseEnd) {
           
    Reloc     = (UINT16 *) ((CHAR8 *) RelocBase + sizeof(EFI_IMAGE_BASE_RELOCATION));
    RelocEnd  = (UINT16 *) ((CHAR8 *) RelocBase + RelocBase->SizeOfBlock);
    FixupBase = PeCoffLoader64ImageAddress (ImageContext, RelocBase->VirtualAddress);
    if ((CHAR8 *) RelocEnd < (CHAR8 *)((UINTN)ImageContext->ImageAddress) || 
        (CHAR8 *) RelocEnd > (CHAR8 *)((UINTN)ImageContext->ImageAddress + 
          (UINTN)ImageContext->ImageSize)) {
      ImageContext->ImageError = IMAGE_ERROR_FAILED_RELOCATION;
      return EFI_LOAD_ERROR;
    }

    //
    // Run this relocation record
    //
    while (Reloc < RelocEnd) {

      Fixup = FixupBase + (*Reloc & 0xFFF);
      switch ((*Reloc) >> 12) {
      case EFI_IMAGE_REL_BASED_ABSOLUTE:
        break;
      case EFI_IMAGE_REL_BASED_HIGH:
        F16 = (UINT16 *) Fixup;
        *F16 = (UINT16)((*F16 << 16) + (UINT16) Adjust);
        if (FixupData != NULL) {
          *(UINT16 *) FixupData = *F16;
          FixupData = FixupData + sizeof(UINT16);
        }
        break;
      case EFI_IMAGE_REL_BASED_LOW:
        F16 = (UINT16 *) Fixup;
        *F16 = (UINT16)(*F16 + (UINT16) Adjust);
        if (FixupData != NULL) {
          *(UINT16 *) FixupData = *F16;
          FixupData = FixupData + sizeof(UINT16);
        }
        break;
      case EFI_IMAGE_REL_BASED_HIGHLOW:
        F32 = (UINT32 *) Fixup;
        *F32 = *F32 + (UINT32) Adjust;
        if (FixupData != NULL) {
          FixupData = ALIGN_POINTER(FixupData, sizeof(UINT32));
          *(UINT32 *) FixupData = *F32;
          FixupData = FixupData + sizeof(UINT32);
        }
        break;
      case EFI_IMAGE_REL_BASED_HIGHADJ:
        // Return the same EFI_UNSUPPORTED return code as 
        // PeCoffLoader64RelocateImageEx() returns if it does not recognize
        // the relocation type.
        //
        ImageContext->ImageError = IMAGE_ERROR_FAILED_RELOCATION;
        return EFI_UNSUPPORTED;
      default:
        Status = PeCoffLoader64RelocateImageEx (Reloc, Fixup, &FixupData, Adjust);
        if (EFI_ERROR (Status)) {
          ImageContext->ImageError = IMAGE_ERROR_FAILED_RELOCATION;
          return Status;
        }
      }

      //
      // Next relocation record
      //
      Reloc += 1;
    }

    //
    // Next reloc block
    //
    RelocBase = (EFI_IMAGE_BASE_RELOCATION *) RelocEnd;
  }

  return EFI_SUCCESS;
}


EFI_STATUS
PeCoffLoader64RelocateImageEx (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup, 
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  )
/*++

Routine Description:
  Performs an IA-32 specific relocation fixup

Arguments:
  Reloc      - Pointer to the relocation record
  Fixup      - Pointer to the address to fix up
  FixupData  - Pointer to a buffer to log the fixups
  Adjust     - The offset to adjust the fixup

Returns:
  None

--*/
{
  UINT64      *F64;

  switch ((*Reloc) >> 12) {

    case EFI_IMAGE_REL_BASED_DIR64:
      F64 = (UINT64 *) Fixup;
      *F64 = *F64 + (UINT64) Adjust;
      if (*FixupData != NULL) {
        *FixupData = ALIGN_POINTER(*FixupData, sizeof(UINT64));
        *(UINT64 *)(*FixupData) = *F64;
        *FixupData = *FixupData + sizeof(UINT64);
      }
      break;

    default:
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
PeCoffLoader64LoadImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL           *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext
  )
/*++

Routine Description:
  Loads a PE/COFF image into memory

Arguments:
  ImageContext - Contains information on image to load into memory

Returns:
  EFI_SUCCESS            if the PE/COFF image was loaded
  EFI_BUFFER_TOO_SMALL   if the caller did not provide a large enough buffer
  EFI_LOAD_ERROR         if the image is a runtime driver with no relocations
  EFI_INVALID_PARAMETER  if the image address is invalid

--*/
{
  EFI_STATUS                            Status;
  EFI_IMAGE_NT_HEADERS64                *PeHdr;
  PE_COFF_LOADER_IMAGE_CONTEXT  CheckContext;
  EFI_IMAGE_SECTION_HEADER              *FirstSection;
  EFI_IMAGE_SECTION_HEADER              *Section;
  UINTN                                 Index;
  CHAR8                                 *Base;
  CHAR8                                 *End;
  CHAR8                                 *MaxEnd;
  EFI_IMAGE_DATA_DIRECTORY              *DirectoryEntry;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY       *DebugEntry;
  UINTN                                 Size;
  UINT32                                TempDebugEntryRva;

  //
  // Assume success
  //
  ImageContext->ImageError = IMAGE_ERROR_SUCCESS;

  //
  // Copy the provided context info into our local version, get what we
  // can from the original image, and then use that to make sure everything
  // is legit.
  //
  CopyMem (
    &CheckContext, 
    ImageContext, 
    sizeof (PE_COFF_LOADER_IMAGE_CONTEXT)
    );
            
  Status = PeCoffLoader64GetImageInfo (
             This,
             &CheckContext 
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure there is enough allocated space for the image being loaded
  //
  if (ImageContext->ImageSize < CheckContext.ImageSize) {
    ImageContext->ImageError = IMAGE_ERROR_INVALID_IMAGE_SIZE;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // If there's no relocations, then make sure it's not a runtime driver,
  // and that it's being loaded at the linked address.
  //
  if (CheckContext.RelocationsStripped == TRUE) {
    //
    // If the image does not contain relocations and it is a runtime driver 
    // then return an error.
    //
    if (CheckContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
      ImageContext->ImageError = IMAGE_ERROR_INVALID_SUBSYSTEM;
      return EFI_LOAD_ERROR;
    }
    //
    // If the image does not contain relocations, and the requested load address 
    // is not the linked address, then return an error.
    //
    if (CheckContext.ImageAddress != ImageContext->ImageAddress) {
      ImageContext->ImageError = IMAGE_ERROR_INVALID_IMAGE_ADDRESS;
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Make sure the allocated space has the proper section alignment
  //
  if ((ImageContext->ImageAddress & (CheckContext.SectionAlignment - 1)) != 0) {
    ImageContext->ImageError = IMAGE_ERROR_INVALID_SECTION_ALIGNMENT;
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the entire PE/COFF header into memory
  //
  Status = ImageContext->ImageRead (
                ImageContext->Handle, 
                0, 
                &ImageContext->SizeOfHeaders, 
                (VOID *)(UINTN)ImageContext->ImageAddress
                );
  if (EFI_ERROR(Status)) {
    ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
    return EFI_LOAD_ERROR;
  }

  PeHdr = (EFI_IMAGE_NT_HEADERS64 *)
      ((UINTN)ImageContext->ImageAddress + ImageContext->PeCoffHeaderOffset);

  //
  // Load each section of the image
  //
  FirstSection = (EFI_IMAGE_SECTION_HEADER *) (
                      (UINTN)ImageContext->ImageAddress +
                      ImageContext->PeCoffHeaderOffset +
                      sizeof(UINT32) + 
                      sizeof(EFI_IMAGE_FILE_HEADER) + 
                      PeHdr->FileHeader.SizeOfOptionalHeader
                      );

  Section = FirstSection;
  for ( Index=0, MaxEnd = NULL; 
        Index < PeHdr->FileHeader.NumberOfSections; 
        Index += 1) {

    //
    // Compute sections address
    //
    Base = PeCoffLoader64ImageAddress (ImageContext, Section->VirtualAddress);
    End  = PeCoffLoader64ImageAddress (
                  ImageContext, 
                  Section->VirtualAddress + Section->Misc.VirtualSize - 1);
    if (End > MaxEnd) {
      MaxEnd = End;
    }
    //
    // If the base start or end address resolved to 0, then fail.
    //
    if (!Base  ||  !End) {
      ImageContext->ImageError = IMAGE_ERROR_SECTION_NOT_LOADED;
      return EFI_LOAD_ERROR;
    }

    //
    // Read the section, we can resume the length of PE image can't 
    // exceed the max 32bit integer
    //
    Size = (UINTN) Section->Misc.VirtualSize;
    if ((Size == 0) || (Size > Section->SizeOfRawData)) {
      Size = (UINTN) Section->SizeOfRawData;
    }
    if (Section->SizeOfRawData) {
      Status = ImageContext->ImageRead (
                               ImageContext->Handle, 
                               Section->PointerToRawData, 
                               &Size, 
                               Base);
      if (EFI_ERROR(Status)) {
        ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
        return Status;
      }
    }

    //
    // If raw size is less then virt size, zero fill the remaining
    //

    if (Size < Section->Misc.VirtualSize) {
      ZeroMem (Base + Size, Section->Misc.VirtualSize - (UINTN)Size);
    }

    //
    // Next Section
    //
    Section += 1;
  }

  //
  // Get image's entry point
  //
  ImageContext->EntryPoint = 
      (EFI_PHYSICAL_ADDRESS) (UINTN) PeCoffLoader64ImageAddress (
                                      ImageContext, 
                                      PeHdr->OptionalHeader.AddressOfEntryPoint
                                      );

  //
  // Determine the size of the fixup data
  //
  // Per the PE/COFF spec, you can't assume that a given data directory 
  // is present in the image. You have to check the NumberOfRvaAndSizes in 
  // the optional header to verify a desired directory entry is there.
  //
  if (PeHdr->OptionalHeader.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
    DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)
      &PeHdr->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    ImageContext->FixupDataSize = 
      DirectoryEntry->Size / sizeof(UINT16) * sizeof(UINTN);
  } else {
    ImageContext->FixupDataSize = 0;
  }
  //
  // Consumer must allocate a buffer for the relocation fixup log.
  // Only used for runtime drivers.
  //
  ImageContext->FixupData     = NULL;

  //
  // Load the Codeview info if present
  //
  if (ImageContext->DebugDirectoryEntryRva != 0) {
    DebugEntry = PeCoffLoader64ImageAddress (
                  ImageContext, 
                  ImageContext->DebugDirectoryEntryRva
                  );
    if (DebugEntry != NULL) {
      TempDebugEntryRva = DebugEntry->RVA;
      if (DebugEntry->RVA == 0 && DebugEntry->FileOffset != 0) {
        Section--;
        if ((UINTN) Section->SizeOfRawData < Section->Misc.VirtualSize) {
          TempDebugEntryRva = Section->VirtualAddress + Section->Misc.VirtualSize;
        } else {
          TempDebugEntryRva = Section->VirtualAddress + Section->SizeOfRawData;
        }
      }
      if (TempDebugEntryRva != 0) {
        ImageContext->CodeView = PeCoffLoader64ImageAddress (ImageContext, TempDebugEntryRva);
        if (ImageContext->CodeView == NULL) {
          ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
          return EFI_LOAD_ERROR;
        }

        if (DebugEntry->RVA == 0) {
          Size = (UINTN) DebugEntry->SizeOfData;
          Status = ImageContext->ImageRead (
                        ImageContext->Handle, 
                        DebugEntry->FileOffset,
                        &Size, 
                        ImageContext->CodeView
                        );
          if (EFI_ERROR(Status)) {
            ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
            return EFI_LOAD_ERROR;
          }
          DebugEntry->RVA = TempDebugEntryRva;
        }

        switch (* (UINT32 *) ImageContext->CodeView) {
          case CODEVIEW_SIGNATURE_NB10:
            ImageContext->PdbPointer = (CHAR8 *) ImageContext->CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
            break;
          case CODEVIEW_SIGNATURE_RSDS:
            ImageContext->PdbPointer = (CHAR8 *) ImageContext->CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
            break;
          default:
            break;
        }
      }
    }
  }
  
  return Status;
}

EFI_STATUS
EFIAPI
PeCoffLoader64UnloadImage (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL      *This,
  IN PE_COFF_LOADER_IMAGE_CONTEXT   *ImageContext
  )
/*++

Routine Description:
  Unload of images is not supported

Arguments:
  ImageContext - The image to unload

Returns:
  EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}

EFI_PEI_PE_COFF_LOADER_PROTOCOL *
EFIAPI
GetPeCoffLoaderProtocol (
  )
{
  return &mPeCoffLoaderX64;
}


