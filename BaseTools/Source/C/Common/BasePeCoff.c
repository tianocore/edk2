/** @file

  Functions to get info and load PE/COFF image.

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
Portions Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <Common/UefiBaseTypes.h>
#include <CommonLib.h>
#include <IndustryStandard/PeImage.h>
#include "PeCoffLib.h"

typedef union {
  VOID                         *Header; 
  EFI_IMAGE_OPTIONAL_HEADER32  *Optional32;
  EFI_IMAGE_OPTIONAL_HEADER64  *Optional64;
} EFI_IMAGE_OPTIONAL_HEADER_POINTER;

STATIC
RETURN_STATUS
PeCoffLoaderGetPeHeader (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT    *ImageContext,
  OUT    EFI_IMAGE_OPTIONAL_HEADER_UNION **PeHdr,
  OUT    EFI_TE_IMAGE_HEADER             **TeHdr
  );

STATIC
RETURN_STATUS
PeCoffLoaderCheckImageType (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT    *ImageContext,
  IN     EFI_IMAGE_OPTIONAL_HEADER_UNION *PeHdr,
  IN     EFI_TE_IMAGE_HEADER             *TeHdr
  );

STATIC
VOID *
PeCoffLoaderImageAddress (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN     UINTN                         Address
  );

RETURN_STATUS
PeCoffLoaderRelocateIa32Image (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  );

RETURN_STATUS
PeCoffLoaderRelocateX64Image (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  );

RETURN_STATUS
PeCoffLoaderRelocateIpfImage (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  );

RETURN_STATUS
PeCoffLoaderRelocateArmImage (
  IN UINT16      **Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  );

RETURN_STATUS
PeCoffLoaderRelocateAArch64Image (
  IN UINT16      *Reloc,
  IN OUT CHAR8   *Fixup,
  IN OUT CHAR8   **FixupData,
  IN UINT64      Adjust
  );

STATIC
RETURN_STATUS
PeCoffLoaderGetPeHeader (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT    *ImageContext,
  OUT    EFI_IMAGE_OPTIONAL_HEADER_UNION **PeHdr,
  OUT    EFI_TE_IMAGE_HEADER             **TeHdr
  )
/*++

Routine Description:

  Retrieves the PE or TE Header from a PE/COFF or TE image

Arguments:

  ImageContext  - The context of the image being loaded

  PeHdr         - The buffer in which to return the PE header
  
  TeHdr         - The buffer in which to return the TE header

Returns:

  RETURN_SUCCESS if the PE or TE Header is read, 
  Otherwise, the error status from reading the PE/COFF or TE image using the ImageRead function.

--*/
{
  RETURN_STATUS         Status;
  EFI_IMAGE_DOS_HEADER  DosHdr;
  UINTN                 Size;

  ImageContext->IsTeImage = FALSE;
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
  if (RETURN_ERROR (Status)) {
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
  // Get the PE/COFF Header pointer
  //
  *PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) ((UINTN)ImageContext->Handle + ImageContext->PeCoffHeaderOffset);
  if ((*PeHdr)->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
    //
    // Check the PE/COFF Header Signature. If not, then try to get a TE header
    //
    *TeHdr = (EFI_TE_IMAGE_HEADER *)*PeHdr; 
    if ((*TeHdr)->Signature != EFI_TE_IMAGE_HEADER_SIGNATURE) {
      return RETURN_UNSUPPORTED;
    }
    ImageContext->IsTeImage = TRUE;
  }

  return RETURN_SUCCESS;
}

STATIC
RETURN_STATUS
PeCoffLoaderCheckImageType (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT          *ImageContext,
  IN     EFI_IMAGE_OPTIONAL_HEADER_UNION       *PeHdr,
  IN     EFI_TE_IMAGE_HEADER                   *TeHdr
  )
/*++

Routine Description:

  Checks the PE or TE header of a PE/COFF or TE image to determine if it supported

Arguments:

  ImageContext  - The context of the image being loaded

  PeHdr         - The buffer in which to return the PE header
  
  TeHdr         - The buffer in which to return the TE header

Returns:

  RETURN_SUCCESS if the PE/COFF or TE image is supported
  RETURN_UNSUPPORTED of the PE/COFF or TE image is not supported.

--*/
{
  //
  // See if the machine type is supported. 
  // We support a native machine type (IA-32/Itanium-based)
  //
  if (ImageContext->IsTeImage == FALSE) {
    ImageContext->Machine = PeHdr->Pe32.FileHeader.Machine;
  } else {
    ImageContext->Machine = TeHdr->Machine;
  }
  
  if (ImageContext->Machine != EFI_IMAGE_MACHINE_IA32 && \
      ImageContext->Machine != EFI_IMAGE_MACHINE_IA64 && \
      ImageContext->Machine != EFI_IMAGE_MACHINE_X64  && \
      ImageContext->Machine != EFI_IMAGE_MACHINE_ARMT && \
      ImageContext->Machine != EFI_IMAGE_MACHINE_EBC  && \
      ImageContext->Machine != EFI_IMAGE_MACHINE_AARCH64) {
    if (ImageContext->Machine == IMAGE_FILE_MACHINE_ARM) {
      //
      // There are two types of ARM images. Pure ARM and ARM/Thumb. 
      // If we see the ARM say it is the ARM/Thumb so there is only
      // a single machine type we need to check for ARM.
      //
      ImageContext->Machine = EFI_IMAGE_MACHINE_ARMT;
      if (ImageContext->IsTeImage == FALSE) {
        PeHdr->Pe32.FileHeader.Machine = ImageContext->Machine;
      } else {
        TeHdr->Machine = ImageContext->Machine;
      }

    } else {
      //
      // unsupported PeImage machine type 
      // 
      return RETURN_UNSUPPORTED;
    }
  }

  //
  // See if the image type is supported.  We support EFI Applications,
  // EFI Boot Service Drivers, EFI Runtime Drivers and EFI SAL Drivers.
  //
  if (ImageContext->IsTeImage == FALSE) {
    ImageContext->ImageType = PeHdr->Pe32.OptionalHeader.Subsystem;
  } else {
    ImageContext->ImageType = (UINT16) (TeHdr->Subsystem);
  }

  if (ImageContext->ImageType != EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION && \
      ImageContext->ImageType != EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER && \
      ImageContext->ImageType != EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER && \
      ImageContext->ImageType != EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER) {
    //
    // upsupported PeImage subsystem type 
    // 
    return RETURN_UNSUPPORTED;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
PeCoffLoaderGetImageInfo (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT           *ImageContext
  )
/*++

Routine Description:

  Retrieves information on a PE/COFF image

Arguments:

  This         - Calling context
  ImageContext - The context of the image being loaded

Returns:

  RETURN_SUCCESS           - The information on the PE/COFF image was collected.
  RETURN_INVALID_PARAMETER - ImageContext is NULL.
  RETURN_UNSUPPORTED       - The PE/COFF image is not supported.
  Otherwise             - The error status from reading the PE/COFF image using the
                          ImageContext->ImageRead() function

--*/
{
  RETURN_STATUS                   Status;
  EFI_IMAGE_OPTIONAL_HEADER_UNION *PeHdr;
  EFI_TE_IMAGE_HEADER             *TeHdr;
  EFI_IMAGE_DATA_DIRECTORY        *DebugDirectoryEntry;
  UINTN                           Size;
  UINTN                           Index;
  UINTN                           DebugDirectoryEntryRva;
  UINTN                           DebugDirectoryEntryFileOffset;
  UINTN                           SectionHeaderOffset;
  EFI_IMAGE_SECTION_HEADER        SectionHeader;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY DebugEntry;
  EFI_IMAGE_OPTIONAL_HEADER_POINTER OptionHeader;

  PeHdr = NULL;
  TeHdr = NULL;
  DebugDirectoryEntry    = NULL;
  DebugDirectoryEntryRva = 0;

  if (NULL == ImageContext) {
    return RETURN_INVALID_PARAMETER;
  }
  //
  // Assume success
  //
  ImageContext->ImageError  = IMAGE_ERROR_SUCCESS;

  Status                    = PeCoffLoaderGetPeHeader (ImageContext, &PeHdr, &TeHdr);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Verify machine type
  //
  Status = PeCoffLoaderCheckImageType (ImageContext, PeHdr, TeHdr);
  if (RETURN_ERROR (Status)) {
    return Status;
  }
  OptionHeader.Header = (VOID *) &(PeHdr->Pe32.OptionalHeader);

  //
  // Retrieve the base address of the image
  //
  if (!(ImageContext->IsTeImage)) {
    if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      ImageContext->ImageAddress = (PHYSICAL_ADDRESS) OptionHeader.Optional32->ImageBase;
    } else {
      ImageContext->ImageAddress = (PHYSICAL_ADDRESS) OptionHeader.Optional64->ImageBase;
    }
  } else {
    ImageContext->ImageAddress = (PHYSICAL_ADDRESS) (TeHdr->ImageBase + TeHdr->StrippedSize - sizeof (EFI_TE_IMAGE_HEADER));
  }
  //
  // Initialize the alternate destination address to 0 indicating that it
  // should not be used.
  //
  ImageContext->DestinationAddress = 0;

  //
  // Initialize the codeview pointer.
  //
  ImageContext->CodeView    = NULL;
  ImageContext->PdbPointer  = NULL;

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
  if ((!(ImageContext->IsTeImage)) && ((PeHdr->Pe32.FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) != 0)) {
    ImageContext->RelocationsStripped = TRUE;
  } else if ((ImageContext->IsTeImage) && (TeHdr->DataDirectory[0].Size == 0)) {
    ImageContext->RelocationsStripped = TRUE;
  } else {
    ImageContext->RelocationsStripped = FALSE;
  }

  if (!(ImageContext->IsTeImage)) {

    if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      ImageContext->ImageSize         = (UINT64) OptionHeader.Optional32->SizeOfImage;
      ImageContext->SectionAlignment  = OptionHeader.Optional32->SectionAlignment;
      ImageContext->SizeOfHeaders     = OptionHeader.Optional32->SizeOfHeaders;
  
      //
      // Modify ImageSize to contain .PDB file name if required and initialize
      // PdbRVA field...
      //
      if (OptionHeader.Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
        DebugDirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *) &(OptionHeader.Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
        DebugDirectoryEntryRva = DebugDirectoryEntry->VirtualAddress;
      }
    } else {
      ImageContext->ImageSize         = (UINT64) OptionHeader.Optional64->SizeOfImage;
      ImageContext->SectionAlignment  = OptionHeader.Optional64->SectionAlignment;
      ImageContext->SizeOfHeaders     = OptionHeader.Optional64->SizeOfHeaders;
  
      //
      // Modify ImageSize to contain .PDB file name if required and initialize
      // PdbRVA field...
      //
      if (OptionHeader.Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
        DebugDirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *) &(OptionHeader.Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
        DebugDirectoryEntryRva = DebugDirectoryEntry->VirtualAddress;
      }
    }
    
    if (DebugDirectoryEntryRva != 0) {
      //
      // Determine the file offset of the debug directory...  This means we walk
      // the sections to find which section contains the RVA of the debug
      // directory
      //
      DebugDirectoryEntryFileOffset = 0;

      SectionHeaderOffset = (UINTN)(
                               ImageContext->PeCoffHeaderOffset +
                               sizeof (UINT32) + 
                               sizeof (EFI_IMAGE_FILE_HEADER) + 
                               PeHdr->Pe32.FileHeader.SizeOfOptionalHeader
                               );

      for (Index = 0; Index < PeHdr->Pe32.FileHeader.NumberOfSections; Index++) {
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
        if (RETURN_ERROR (Status)) {
          ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
          return Status;
        }

        if (DebugDirectoryEntryRva >= SectionHeader.VirtualAddress &&
            DebugDirectoryEntryRva < SectionHeader.VirtualAddress + SectionHeader.Misc.VirtualSize) {
            DebugDirectoryEntryFileOffset =
            DebugDirectoryEntryRva - SectionHeader.VirtualAddress + SectionHeader.PointerToRawData;
          break;
        }

        SectionHeaderOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
      }

      if (DebugDirectoryEntryFileOffset != 0) {    
        for (Index = 0; Index < DebugDirectoryEntry->Size; Index += sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY)) {
          //
          // Read next debug directory entry
          //
          Size = sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY);    
          Status = ImageContext->ImageRead (
                                   ImageContext->Handle,
                                   DebugDirectoryEntryFileOffset + Index,
                                   &Size,
                                   &DebugEntry
                                   );
          if (RETURN_ERROR (Status)) {
            ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
            return Status;
          }

          if (DebugEntry.Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
            ImageContext->DebugDirectoryEntryRva = (UINT32) (DebugDirectoryEntryRva + Index);
            if (DebugEntry.RVA == 0 && DebugEntry.FileOffset != 0) {
              ImageContext->ImageSize += DebugEntry.SizeOfData;
            }

            return RETURN_SUCCESS;
          }
        }
      }
    }
  } else {
    ImageContext->ImageSize         = 0;
    ImageContext->SectionAlignment  = 4096;
    ImageContext->SizeOfHeaders     = sizeof (EFI_TE_IMAGE_HEADER) + (UINTN) TeHdr->BaseOfCode - (UINTN) TeHdr->StrippedSize;

    DebugDirectoryEntry             = &TeHdr->DataDirectory[1];
    DebugDirectoryEntryRva          = DebugDirectoryEntry->VirtualAddress;
    SectionHeaderOffset             = (UINTN) (sizeof (EFI_TE_IMAGE_HEADER));

    DebugDirectoryEntryFileOffset   = 0;

    for (Index = 0; Index < TeHdr->NumberOfSections;) {
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
      if (RETURN_ERROR (Status)) {
        ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
        return Status;
      }

      if (DebugDirectoryEntryRva >= SectionHeader.VirtualAddress &&
          DebugDirectoryEntryRva < SectionHeader.VirtualAddress + SectionHeader.Misc.VirtualSize) {
        DebugDirectoryEntryFileOffset = DebugDirectoryEntryRva -
          SectionHeader.VirtualAddress +
          SectionHeader.PointerToRawData +
          sizeof (EFI_TE_IMAGE_HEADER) -
          TeHdr->StrippedSize;

        //
        // File offset of the debug directory was found, if this is not the last
        // section, then skip to the last section for calculating the image size.
        //
        if (Index < (UINTN) TeHdr->NumberOfSections - 1) {
          SectionHeaderOffset += (TeHdr->NumberOfSections - 1 - Index) * sizeof (EFI_IMAGE_SECTION_HEADER);
          Index = TeHdr->NumberOfSections - 1;
          continue;
        }
      }

      //
      // In Te image header there is not a field to describe the ImageSize.
      // Actually, the ImageSize equals the RVA plus the VirtualSize of 
      // the last section mapped into memory (Must be rounded up to 
      // a mulitple of Section Alignment). Per the PE/COFF specification, the
      // section headers in the Section Table must appear in order of the RVA
      // values for the corresponding sections. So the ImageSize can be determined
      // by the RVA and the VirtualSize of the last section header in the
      // Section Table.
      //
      if ((++Index) == (UINTN) TeHdr->NumberOfSections) {
        ImageContext->ImageSize = (SectionHeader.VirtualAddress + SectionHeader.Misc.VirtualSize +
                                   ImageContext->SectionAlignment - 1) & ~(ImageContext->SectionAlignment - 1);
      }

      SectionHeaderOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
    }

    if (DebugDirectoryEntryFileOffset != 0) {
      for (Index = 0; Index < DebugDirectoryEntry->Size; Index += sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY)) {
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
        if (RETURN_ERROR (Status)) {
          ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
          return Status;
        }

        if (DebugEntry.Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
          ImageContext->DebugDirectoryEntryRva = (UINT32) (DebugDirectoryEntryRva + Index);
          return RETURN_SUCCESS;
        }
      }
    }
  }

  return RETURN_SUCCESS;
}

STATIC
VOID *
PeCoffLoaderImageAddress (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT          *ImageContext,
  IN     UINTN                                 Address
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

  return (UINT8 *) ((UINTN) ImageContext->ImageAddress + Address);
}

RETURN_STATUS
EFIAPI
PeCoffLoaderRelocateImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:

  Relocates a PE/COFF image in memory

Arguments:

  This         - Calling context

  ImageContext - Contains information on the loaded image to relocate

Returns:

  RETURN_SUCCESS      if the PE/COFF image was relocated
  RETURN_LOAD_ERROR   if the image is not a valid PE/COFF image
  RETURN_UNSUPPORTED  not support

--*/
{
  RETURN_STATUS                         Status;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *PeHdr;
  EFI_TE_IMAGE_HEADER                   *TeHdr;
  EFI_IMAGE_DATA_DIRECTORY              *RelocDir;
  UINT64                                Adjust;
  EFI_IMAGE_BASE_RELOCATION             *RelocBase;
  EFI_IMAGE_BASE_RELOCATION             *RelocBaseEnd;
  UINT16                                *Reloc;
  UINT16                                *RelocEnd;
  CHAR8                                 *Fixup;
  CHAR8                                 *FixupBase;
  UINT16                                *F16;
  UINT32                                *F32;
  CHAR8                                 *FixupData;
  PHYSICAL_ADDRESS                      BaseAddress;
  UINT16                                MachineType;
  EFI_IMAGE_OPTIONAL_HEADER_POINTER     OptionHeader;

  PeHdr = NULL;
  TeHdr = NULL;
  //
  // Assume success
  //
  ImageContext->ImageError = IMAGE_ERROR_SUCCESS;

  //
  // If there are no relocation entries, then we are done
  //
  if (ImageContext->RelocationsStripped) {
    return RETURN_SUCCESS;
  }

  //
  // Use DestinationAddress field of ImageContext as the relocation address even if it is 0.
  //
  BaseAddress = ImageContext->DestinationAddress;
  
  if (!(ImageContext->IsTeImage)) {
    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((UINTN)ImageContext->ImageAddress + 
                                            ImageContext->PeCoffHeaderOffset);
    OptionHeader.Header = (VOID *) &(PeHdr->Pe32.OptionalHeader);
    if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      Adjust = (UINT64) BaseAddress - OptionHeader.Optional32->ImageBase;
      OptionHeader.Optional32->ImageBase = (UINT32) BaseAddress;
      MachineType = ImageContext->Machine;
      //
      // Find the relocation block
      //
      // Per the PE/COFF spec, you can't assume that a given data directory
      // is present in the image. You have to check the NumberOfRvaAndSizes in
      // the optional header to verify a desired directory entry is there.
      //
      if (OptionHeader.Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
        RelocDir  = &OptionHeader.Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
        RelocBase = PeCoffLoaderImageAddress (ImageContext, RelocDir->VirtualAddress);
        RelocBaseEnd = PeCoffLoaderImageAddress (
                        ImageContext,
                        RelocDir->VirtualAddress + RelocDir->Size - 1
                        );
      } else {
        //
        // Set base and end to bypass processing below.
        //
        RelocBase = RelocBaseEnd = 0;
      }
    } else {
      Adjust = (UINT64) BaseAddress - OptionHeader.Optional64->ImageBase;
      OptionHeader.Optional64->ImageBase = BaseAddress;
      MachineType = ImageContext->Machine;
      //
      // Find the relocation block
      //
      // Per the PE/COFF spec, you can't assume that a given data directory
      // is present in the image. You have to check the NumberOfRvaAndSizes in
      // the optional header to verify a desired directory entry is there.
      //
      if (OptionHeader.Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
        RelocDir  = &OptionHeader.Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
        RelocBase = PeCoffLoaderImageAddress (ImageContext, RelocDir->VirtualAddress);
        RelocBaseEnd = PeCoffLoaderImageAddress (
                        ImageContext,
                        RelocDir->VirtualAddress + RelocDir->Size - 1
                        );
      } else {
        //
        // Set base and end to bypass processing below.
        //
        RelocBase = RelocBaseEnd = 0;
      }
    }
  } else {
    TeHdr             = (EFI_TE_IMAGE_HEADER *) (UINTN) (ImageContext->ImageAddress);
    Adjust            = (UINT64) (BaseAddress - TeHdr->ImageBase);
    TeHdr->ImageBase  = (UINT64) (BaseAddress);
    MachineType = TeHdr->Machine;
    
    //
    // Find the relocation block
    //
    RelocDir = &TeHdr->DataDirectory[0];
    RelocBase = (EFI_IMAGE_BASE_RELOCATION *)(UINTN)(
                                    ImageContext->ImageAddress + 
                                    RelocDir->VirtualAddress +
                                    sizeof(EFI_TE_IMAGE_HEADER) - 
                                    TeHdr->StrippedSize
                                    );
    RelocBaseEnd = (EFI_IMAGE_BASE_RELOCATION *) ((UINTN) RelocBase + (UINTN) RelocDir->Size - 1);
  }
  
  //
  // Run the relocation information and apply the fixups
  //
  FixupData = ImageContext->FixupData;
  while (RelocBase < RelocBaseEnd) {

    Reloc     = (UINT16 *) ((CHAR8 *) RelocBase + sizeof (EFI_IMAGE_BASE_RELOCATION));
    RelocEnd  = (UINT16 *) ((CHAR8 *) RelocBase + RelocBase->SizeOfBlock);
    if (!(ImageContext->IsTeImage)) {
      FixupBase = PeCoffLoaderImageAddress (ImageContext, RelocBase->VirtualAddress);
    } else {
      FixupBase = (CHAR8 *)(UINTN)(ImageContext->ImageAddress +
                    RelocBase->VirtualAddress +
                    sizeof(EFI_TE_IMAGE_HEADER) - 
                    TeHdr->StrippedSize
                    );
    }

    if ((CHAR8 *) RelocEnd < (CHAR8 *) ((UINTN) ImageContext->ImageAddress) ||
        (CHAR8 *) RelocEnd > (CHAR8 *)((UINTN)ImageContext->ImageAddress + 
          (UINTN)ImageContext->ImageSize)) {
      ImageContext->ImageError = IMAGE_ERROR_FAILED_RELOCATION;
      return RETURN_LOAD_ERROR;
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
        F16   = (UINT16 *) Fixup;
        *F16 = (UINT16) (*F16 + ((UINT16) ((UINT32) Adjust >> 16)));
        if (FixupData != NULL) {
          *(UINT16 *) FixupData = *F16;
          FixupData             = FixupData + sizeof (UINT16);
        }
        break;

      case EFI_IMAGE_REL_BASED_LOW:
        F16   = (UINT16 *) Fixup;
        *F16  = (UINT16) (*F16 + (UINT16) Adjust);
        if (FixupData != NULL) {
          *(UINT16 *) FixupData = *F16;
          FixupData             = FixupData + sizeof (UINT16);
        }
        break;

      case EFI_IMAGE_REL_BASED_HIGHLOW:
        F32   = (UINT32 *) Fixup;
        *F32  = *F32 + (UINT32) Adjust;
        if (FixupData != NULL) {
          FixupData             = ALIGN_POINTER (FixupData, sizeof (UINT32));
          *(UINT32 *) FixupData = *F32;
          FixupData             = FixupData + sizeof (UINT32);
        }
        break;

      case EFI_IMAGE_REL_BASED_HIGHADJ:
        //
        // Return the same EFI_UNSUPPORTED return code as
        // PeCoffLoaderRelocateImageEx() returns if it does not recognize
        // the relocation type.
        //
        ImageContext->ImageError = IMAGE_ERROR_FAILED_RELOCATION;
        return RETURN_UNSUPPORTED;

      default:
        switch (MachineType) {
        case EFI_IMAGE_MACHINE_IA32:
          Status = PeCoffLoaderRelocateIa32Image (Reloc, Fixup, &FixupData, Adjust);
          break;
        case EFI_IMAGE_MACHINE_ARMT:
          Status = PeCoffLoaderRelocateArmImage (&Reloc, Fixup, &FixupData, Adjust);
          break;
        case EFI_IMAGE_MACHINE_X64:
          Status = PeCoffLoaderRelocateX64Image (Reloc, Fixup, &FixupData, Adjust);
          break;
        case EFI_IMAGE_MACHINE_IA64:
          Status = PeCoffLoaderRelocateIpfImage (Reloc, Fixup, &FixupData, Adjust);
          break;
        case EFI_IMAGE_MACHINE_AARCH64:
          Status = PeCoffLoaderRelocateAArch64Image (Reloc, Fixup, &FixupData, Adjust);
          break;
        default:
          Status = RETURN_UNSUPPORTED;
          break;
        }
        if (RETURN_ERROR (Status)) {
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

  return RETURN_SUCCESS;
}

RETURN_STATUS
EFIAPI
PeCoffLoaderLoadImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:

  Loads a PE/COFF image into memory

Arguments:

  This         - Calling context

  ImageContext - Contains information on image to load into memory

Returns:

  RETURN_SUCCESS            if the PE/COFF image was loaded
  RETURN_BUFFER_TOO_SMALL   if the caller did not provide a large enough buffer
  RETURN_LOAD_ERROR         if the image is a runtime driver with no relocations
  RETURN_INVALID_PARAMETER  if the image address is invalid

--*/
{
  RETURN_STATUS                         Status;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *PeHdr;
  EFI_TE_IMAGE_HEADER                   *TeHdr;
  PE_COFF_LOADER_IMAGE_CONTEXT          CheckContext;
  EFI_IMAGE_SECTION_HEADER              *FirstSection;
  EFI_IMAGE_SECTION_HEADER              *Section;
  UINTN                                 NumberOfSections;
  UINTN                                 Index;
  CHAR8                                 *Base;
  CHAR8                                 *End;
  CHAR8                                 *MaxEnd;
  EFI_IMAGE_DATA_DIRECTORY              *DirectoryEntry;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY       *DebugEntry;
  UINTN                                 Size;
  UINT32                                TempDebugEntryRva;
  EFI_IMAGE_OPTIONAL_HEADER_POINTER     OptionHeader;

  PeHdr = NULL;
  TeHdr = NULL;
  OptionHeader.Header = NULL;
  //
  // Assume success
  //
  ImageContext->ImageError = IMAGE_ERROR_SUCCESS;

  //
  // Copy the provided context info into our local version, get what we
  // can from the original image, and then use that to make sure everything
  // is legit.
  //
  CopyMem (&CheckContext, ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));

  Status = PeCoffLoaderGetImageInfo (&CheckContext);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure there is enough allocated space for the image being loaded
  //
  if (ImageContext->ImageSize < CheckContext.ImageSize) {
    ImageContext->ImageError = IMAGE_ERROR_INVALID_IMAGE_SIZE;
    return RETURN_BUFFER_TOO_SMALL;
  }

  //
  // If there's no relocations, then make sure it's not a runtime driver,
  // and that it's being loaded at the linked address.
  //
  if (CheckContext.RelocationsStripped) {
    //
    // If the image does not contain relocations and it is a runtime driver
    // then return an error.
    //
    if (CheckContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER) {
      ImageContext->ImageError = IMAGE_ERROR_INVALID_SUBSYSTEM;
      return RETURN_LOAD_ERROR;
    }
    //
    // If the image does not contain relocations, and the requested load address
    // is not the linked address, then return an error.
    //
    if (CheckContext.ImageAddress != ImageContext->ImageAddress) {
      ImageContext->ImageError = IMAGE_ERROR_INVALID_IMAGE_ADDRESS;
      return RETURN_INVALID_PARAMETER;
    }
  }
  //
  // Make sure the allocated space has the proper section alignment
  //
  if (!(ImageContext->IsTeImage)) {
    if ((ImageContext->ImageAddress & (CheckContext.SectionAlignment - 1)) != 0) {
      ImageContext->ImageError = IMAGE_ERROR_INVALID_SECTION_ALIGNMENT;
      return RETURN_INVALID_PARAMETER;
    }
  }
  //
  // Read the entire PE/COFF or TE header into memory
  //
  if (!(ImageContext->IsTeImage)) {
    Status = ImageContext->ImageRead (
                            ImageContext->Handle,
                            0,
                            &ImageContext->SizeOfHeaders,
                            (VOID *) (UINTN) ImageContext->ImageAddress
                            );

    PeHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)
      ((UINTN)ImageContext->ImageAddress + ImageContext->PeCoffHeaderOffset);

    OptionHeader.Header = (VOID *) &(PeHdr->Pe32.OptionalHeader);
    
    FirstSection = (EFI_IMAGE_SECTION_HEADER *) (
                      (UINTN)ImageContext->ImageAddress +
                      ImageContext->PeCoffHeaderOffset +
                      sizeof(UINT32) + 
                      sizeof(EFI_IMAGE_FILE_HEADER) + 
                      PeHdr->Pe32.FileHeader.SizeOfOptionalHeader
      );
    NumberOfSections = (UINTN) (PeHdr->Pe32.FileHeader.NumberOfSections);
  } else {
    Status = ImageContext->ImageRead (
                            ImageContext->Handle,
                            0,
                            &ImageContext->SizeOfHeaders,
                            (VOID *) (UINTN) ImageContext->ImageAddress
                            );

    TeHdr             = (EFI_TE_IMAGE_HEADER *) (UINTN) (ImageContext->ImageAddress);

    FirstSection = (EFI_IMAGE_SECTION_HEADER *) (
          (UINTN)ImageContext->ImageAddress +
          sizeof(EFI_TE_IMAGE_HEADER)
          );
    NumberOfSections  = (UINTN) (TeHdr->NumberOfSections);

  }

  if (RETURN_ERROR (Status)) {
    ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
    return RETURN_LOAD_ERROR;
  }

  //
  // Load each section of the image
  //
  Section = FirstSection;
  for (Index = 0, MaxEnd = NULL; Index < NumberOfSections; Index++) {

    //
    // Compute sections address
    //
    Base = PeCoffLoaderImageAddress (ImageContext, Section->VirtualAddress);
    End = PeCoffLoaderImageAddress (
            ImageContext,
            Section->VirtualAddress + Section->Misc.VirtualSize - 1
            );

    //
    // If the base start or end address resolved to 0, then fail.
    //
    if ((Base == NULL) || (End == NULL)) {
      ImageContext->ImageError = IMAGE_ERROR_SECTION_NOT_LOADED;
      return RETURN_LOAD_ERROR;
    }

	
    if (ImageContext->IsTeImage) {
      Base  = (CHAR8 *) ((UINTN) Base + sizeof (EFI_TE_IMAGE_HEADER) - (UINTN) TeHdr->StrippedSize);
      End   = (CHAR8 *) ((UINTN) End + sizeof (EFI_TE_IMAGE_HEADER) - (UINTN) TeHdr->StrippedSize);
    }

    if (End > MaxEnd) {
      MaxEnd = End;
    }

    //
    // Read the section
    //
    Size = (UINTN) Section->Misc.VirtualSize;
    if ((Size == 0) || (Size > Section->SizeOfRawData)) {
      Size = (UINTN) Section->SizeOfRawData;
    }

    if (Section->SizeOfRawData) {
      if (!(ImageContext->IsTeImage)) {
        Status = ImageContext->ImageRead (
                                ImageContext->Handle,
                                Section->PointerToRawData,
                                &Size,
                                Base
                                );
      } else {
        Status = ImageContext->ImageRead (
                                ImageContext->Handle,
                                Section->PointerToRawData + sizeof (EFI_TE_IMAGE_HEADER) - (UINTN) TeHdr->StrippedSize,
                                &Size,
                                Base
                                );
      }

      if (RETURN_ERROR (Status)) {
        ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
        return Status;
      }
    }

    //
    // If raw size is less then virt size, zero fill the remaining
    //

    if (Size < Section->Misc.VirtualSize) {
      ZeroMem (Base + Size, Section->Misc.VirtualSize - Size);
    }

    //
    // Next Section
    //
    Section += 1;
  }

  //
  // Get image's entry point
  //
  if (!(ImageContext->IsTeImage)) {
    ImageContext->EntryPoint = (PHYSICAL_ADDRESS) (UINTN) PeCoffLoaderImageAddress (
                                                                ImageContext,
                                                                PeHdr->Pe32.OptionalHeader.AddressOfEntryPoint
                                                                );
  } else {
    ImageContext->EntryPoint =  (PHYSICAL_ADDRESS) (
                       (UINTN)ImageContext->ImageAddress +
                       (UINTN)TeHdr->AddressOfEntryPoint +
                       (UINTN)sizeof(EFI_TE_IMAGE_HEADER) -
          (UINTN) TeHdr->StrippedSize
      );
  }

  //
  // Determine the size of the fixup data
  //
  // Per the PE/COFF spec, you can't assume that a given data directory
  // is present in the image. You have to check the NumberOfRvaAndSizes in
  // the optional header to verify a desired directory entry is there.
  //
  if (!(ImageContext->IsTeImage)) {
    if (PeHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      if (OptionHeader.Optional32->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
        DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)
          &OptionHeader.Optional32->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
        ImageContext->FixupDataSize = DirectoryEntry->Size / sizeof (UINT16) * sizeof (UINTN);
      } else {
        ImageContext->FixupDataSize = 0;
      }
    } else {
      if (OptionHeader.Optional64->NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
        DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)
          &OptionHeader.Optional64->DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
        ImageContext->FixupDataSize = DirectoryEntry->Size / sizeof (UINT16) * sizeof (UINTN);
      } else {
        ImageContext->FixupDataSize = 0;
      }
    }
  } else {
    DirectoryEntry              = &TeHdr->DataDirectory[0];
    ImageContext->FixupDataSize = DirectoryEntry->Size / sizeof (UINT16) * sizeof (UINTN);
  }
  //
  // Consumer must allocate a buffer for the relocation fixup log.
  // Only used for runtime drivers.
  //
  ImageContext->FixupData = NULL;

  //
  // Load the Codeview info if present
  //
  if (ImageContext->DebugDirectoryEntryRva != 0) {
    if (!(ImageContext->IsTeImage)) {
      DebugEntry = PeCoffLoaderImageAddress (
                    ImageContext,
                    ImageContext->DebugDirectoryEntryRva
                    );
    } else {
      DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *)(UINTN)(
                                               ImageContext->ImageAddress +
                                               ImageContext->DebugDirectoryEntryRva +
                                               sizeof(EFI_TE_IMAGE_HEADER) -
                                               TeHdr->StrippedSize
                                               );
    }

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
        if (!(ImageContext->IsTeImage)) {
          ImageContext->CodeView = PeCoffLoaderImageAddress (ImageContext, TempDebugEntryRva);
        } else {
          ImageContext->CodeView = (VOID *)(
                      (UINTN)ImageContext->ImageAddress +
                      (UINTN)TempDebugEntryRva +
                      (UINTN)sizeof(EFI_TE_IMAGE_HEADER) -
                (UINTN) TeHdr->StrippedSize
            );
        }

        if (ImageContext->CodeView == NULL) {
          ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
          return RETURN_LOAD_ERROR;
        }

        if (DebugEntry->RVA == 0) {
          Size = DebugEntry->SizeOfData;
          if (!(ImageContext->IsTeImage)) {
            Status = ImageContext->ImageRead (
                                    ImageContext->Handle,
                                    DebugEntry->FileOffset,
                                    &Size,
                                    ImageContext->CodeView
                                    );
          } else {
            Status = ImageContext->ImageRead (
                                    ImageContext->Handle,
                                    DebugEntry->FileOffset + sizeof (EFI_TE_IMAGE_HEADER) - TeHdr->StrippedSize,
                                    &Size,
                                    ImageContext->CodeView
                                    );
            //
            // Should we apply fix up to this field according to the size difference between PE and TE?
            // Because now we maintain TE header fields unfixed, this field will also remain as they are
            // in original PE image.
            //
          }

          if (RETURN_ERROR (Status)) {
            ImageContext->ImageError = IMAGE_ERROR_IMAGE_READ;
            return RETURN_LOAD_ERROR;
          }

          DebugEntry->RVA = TempDebugEntryRva;
        }

        switch (*(UINT32 *) ImageContext->CodeView) {
        case CODEVIEW_SIGNATURE_NB10:
          ImageContext->PdbPointer = (CHAR8 *) ImageContext->CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
          break;

        case CODEVIEW_SIGNATURE_RSDS:
          ImageContext->PdbPointer = (CHAR8 *) ImageContext->CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
          break;

        case CODEVIEW_SIGNATURE_MTOC:
          ImageContext->PdbPointer = (CHAR8 *) ImageContext->CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY);

        default:
          break;
        }
      }
    }
  }

  return Status;
}

/**
  Returns a pointer to the PDB file name for a raw PE/COFF image that is not
  loaded into system memory with the PE/COFF Loader Library functions.

  Returns the PDB file name for the PE/COFF image specified by Pe32Data.  If
  the PE/COFF image specified by Pe32Data is not a valid, then NULL is
  returned.  If the PE/COFF image specified by Pe32Data does not contain a
  debug directory entry, then NULL is returned.  If the debug directory entry
  in the PE/COFF image specified by Pe32Data does not contain a PDB file name,
  then NULL is returned.
  If Pe32Data is NULL, then return NULL.

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
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;
  UINT32                                Index, Index1;

  if (Pe32Data == NULL) {
    return NULL;
  }

  TEImageAdjust       = 0;
  DirectoryEntry      = NULL;
  DebugEntry          = NULL;
  NumberOfRvaAndSizes = 0;
  Index               = 0;
  Index1              = 0;
  SectionHeader       = NULL;

  DosHdr = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
  if (EFI_IMAGE_DOS_SIGNATURE == DosHdr->e_magic) {
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

  if (EFI_TE_IMAGE_HEADER_SIGNATURE == Hdr.Te->Signature) {
    if (Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress != 0) {
      DirectoryEntry  = &Hdr.Te->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG];
      TEImageAdjust   = sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize;
      
      //
      // Get the DebugEntry offset in the raw data image.
      //
      SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (Hdr.Te + 1);
      Index = Hdr.Te->NumberOfSections;
      for (Index1 = 0; Index1 < Index; Index1 ++) {
        if ((DirectoryEntry->VirtualAddress >= SectionHeader[Index1].VirtualAddress) && 
           (DirectoryEntry->VirtualAddress < (SectionHeader[Index1].VirtualAddress + SectionHeader[Index1].Misc.VirtualSize))) {
          DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *)((UINTN) Hdr.Te +
                        DirectoryEntry->VirtualAddress - 
                        SectionHeader [Index1].VirtualAddress + 
                        SectionHeader [Index1].PointerToRawData + 
                        TEImageAdjust);
          break;
        }
      }
    }
  } else if (EFI_IMAGE_NT_SIGNATURE == Hdr.Pe32->Signature) {
    //
    // NOTE: We use Machine field to identify PE32/PE32+, instead of Magic.
    //       It is due to backward-compatibility, for some system might
    //       generate PE32+ image with PE32 Magic.
    //
    switch (Hdr.Pe32->FileHeader.Machine) {
    case EFI_IMAGE_MACHINE_IA32:
    case EFI_IMAGE_MACHINE_ARMT:
      //
      // Assume PE32 image with IA32 Machine field.
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
      break;
    case EFI_IMAGE_MACHINE_X64:
    case EFI_IMAGE_MACHINE_IPF:
      //
      // Assume PE32+ image with X64 or IPF Machine field
      //
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
      break;
    default:
      //
      // For unknow Machine field, use Magic in optional Header
      //
      Magic = Hdr.Pe32->OptionalHeader.Magic;
    }

    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
                       (UINT8 *) Hdr.Pe32 +
                       sizeof (UINT32) + 
                       sizeof (EFI_IMAGE_FILE_HEADER) +  
                       Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                       );
    Index = Hdr.Pe32->FileHeader.NumberOfSections;

    if (EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC == Magic) {
      //
      // Use PE32 offset get Debug Directory Entry
      //
      NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
    } else if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      //
      // Use PE32+ offset get Debug Directory Entry
      //
      NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
    }

    if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_DEBUG || DirectoryEntry->VirtualAddress == 0) {
      DirectoryEntry = NULL;
      DebugEntry = NULL;
    } else {
      //
      // Get the DebugEntry offset in the raw data image.
      //
      for (Index1 = 0; Index1 < Index; Index1 ++) {
        if ((DirectoryEntry->VirtualAddress >= SectionHeader[Index1].VirtualAddress) && 
           (DirectoryEntry->VirtualAddress < (SectionHeader[Index1].VirtualAddress + SectionHeader[Index1].Misc.VirtualSize))) {
          DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) (
                       (UINTN) Pe32Data + 
                       DirectoryEntry->VirtualAddress - 
                       SectionHeader[Index1].VirtualAddress + 
                       SectionHeader[Index1].PointerToRawData);
          break;
        }
      }
    }
  } else {
    return NULL;
  }

  if (NULL == DebugEntry || NULL == DirectoryEntry) {
    return NULL;
  }

  //
  // Scan the directory to find the debug entry.
  // 
  for (DirCount = 0; DirCount < DirectoryEntry->Size; DirCount += sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY), DebugEntry++) {
    if (EFI_IMAGE_DEBUG_TYPE_CODEVIEW == DebugEntry->Type) {
      if (DebugEntry->SizeOfData > 0) {
        //
        // Get the DebugEntry offset in the raw data image.
        //
        CodeViewEntryPointer = NULL;
        for (Index1 = 0; Index1 < Index; Index1 ++) {
          if ((DebugEntry->RVA >= SectionHeader[Index1].VirtualAddress) && 
             (DebugEntry->RVA < (SectionHeader[Index1].VirtualAddress + SectionHeader[Index1].Misc.VirtualSize))) {
            CodeViewEntryPointer = (VOID *) (
                                   ((UINTN)Pe32Data) + 
                                   (UINTN) DebugEntry->RVA - 
                                   SectionHeader[Index1].VirtualAddress + 
                                   SectionHeader[Index1].PointerToRawData + 
                                   (UINTN)TEImageAdjust);
            break;
          }
        }
        if (Index1 >= Index) {
          //
          // Can't find CodeViewEntryPointer in raw PE/COFF image.
          //
          continue;
        }
        switch (* (UINT32 *) CodeViewEntryPointer) {
        case CODEVIEW_SIGNATURE_NB10:
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY));
        case CODEVIEW_SIGNATURE_RSDS:
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY));
        case CODEVIEW_SIGNATURE_MTOC:
          return (VOID *) ((CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_MTOC_ENTRY));
        default:
          break;
        }
      }
    }
  }

  return NULL;
}


RETURN_STATUS
EFIAPI
PeCoffLoaderGetEntryPoint (
  IN  VOID  *Pe32Data,
  OUT VOID  **EntryPoint,
  OUT VOID  **BaseOfImage
  )
{
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;

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

  //
  // Calculate the entry point relative to the start of the image.
  // AddressOfEntryPoint is common for PE32 & PE32+
  //
  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    *BaseOfImage = (VOID *)(UINTN)(Hdr.Te->ImageBase + Hdr.Te->StrippedSize - sizeof (EFI_TE_IMAGE_HEADER));
    *EntryPoint = (VOID *)((UINTN)*BaseOfImage + (Hdr.Te->AddressOfEntryPoint & 0x0ffffffff) + sizeof(EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize);
    return RETURN_SUCCESS;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
    *EntryPoint = (VOID *)(UINTN)Hdr.Pe32->OptionalHeader.AddressOfEntryPoint;
    if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      *BaseOfImage = (VOID *)(UINTN)Hdr.Pe32->OptionalHeader.ImageBase;
    } else {
      *BaseOfImage = (VOID *)(UINTN)Hdr.Pe32Plus->OptionalHeader.ImageBase;
    }
    *EntryPoint = (VOID *)(UINTN)((UINTN)*EntryPoint + (UINTN)*BaseOfImage);
    return RETURN_SUCCESS;
  }

  return RETURN_UNSUPPORTED;
}
