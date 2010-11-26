/*++

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeCoffLoader.c

Abstract:

  Tiano PE/COFF loader 

Revision History

--*/

#include "Tiano.h"
#include "Pei.h"
#include "PeiLib.h"
#include "PeCoffLoaderEx.h"

#ifdef EFI_NT_EMULATOR
#include "peilib.h"
#include "EfiHobLib.h"
#include EFI_PPI_DEFINITION (NtLoadAsDll)
#endif

STATIC
EFI_STATUS
PeCoffLoaderGetPeHeader (
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT   *ImageContext,
  OUT    EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION    Hdr
  );

STATIC
VOID*
PeCoffLoaderImageAddress (
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN     UINTN                                 Address
  );

EFI_STATUS
EFIAPI
PeCoffLoaderGetImageInfo (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

EFI_STATUS
EFIAPI
PeCoffLoaderRelocateImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

EFI_STATUS
EFIAPI
PeCoffLoaderLoadImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

EFI_STATUS
EFIAPI
PeCoffLoaderUnloadImage (
  IN EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT   *ImageContext
  );

#if defined (EFI_DEBUG_ITP_BREAK) && !defined (_CONSOLE)
VOID
AsmEfiSetBreakSupport (
  IN UINTN  LoadAddr
  );
#endif

EFI_PEI_PE_COFF_LOADER_PROTOCOL mPeCoffLoader = {
  PeCoffLoaderGetImageInfo,
  PeCoffLoaderLoadImage,
  PeCoffLoaderRelocateImage,
  PeCoffLoaderUnloadImage
};

#ifdef EFI_NT_EMULATOR
EFI_NT_LOAD_AS_DLL_PPI          *mPeCoffLoaderWinNtLoadAsDll = NULL;
#endif

EFI_STATUS
InstallEfiPeiPeCoffLoader (
  IN EFI_PEI_SERVICES                          **PeiServices,
  IN OUT  EFI_PEI_PE_COFF_LOADER_PROTOCOL      **This,
  IN EFI_PEI_PPI_DESCRIPTOR                    *ThisPpi
  )
/*++

Routine Description:

  Install PE/COFF loader PPI
  
Arguments:

  PeiServices - General purpose services available to every PEIM

  This        - Pointer to get Pei PE coff loader protocol as output
  
  ThisPpi     - Passed in as EFI_NT_LOAD_AS_DLL_PPI on NT_EMULATOR platform

Returns:

  EFI_SUCCESS

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

#ifdef EFI_NT_EMULATOR
  //
  // For use by PEI Core and Modules
  //
  if (NULL != PeiServices) {
    Status = (**PeiServices).LocatePpi (
                              PeiServices,
                              &gEfiNtLoadAsDllPpiGuid,
                              0,
                              NULL,
                              &mPeCoffLoaderWinNtLoadAsDll
                              );
  } else {
    //
    // Now in SecMain or ERM usage, bind appropriately
    //
    PEI_ASSERT (PeiServices, (NULL != ThisPpi));

    mPeCoffLoaderWinNtLoadAsDll = (EFI_NT_LOAD_AS_DLL_PPI *) ThisPpi;
    PEI_ASSERT (PeiServices, (NULL != mPeCoffLoaderWinNtLoadAsDll));
  }
#endif

  if (NULL != This) {
    *This = &mPeCoffLoader;
  }

  return Status;
}

STATIC
EFI_STATUS
PeCoffLoaderGetPeHeader (
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT   *ImageContext,
  OUT    EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION    Hdr
  )
/*++

Routine Description:

  Retrieves the PE or TE Header from a PE/COFF or TE image

Arguments:

  ImageContext  - The context of the image being loaded

  PeHdr         - The buffer in which to return the PE32, PE32+, or TE header

Returns:

  EFI_SUCCESS if the PE or TE Header is read, 
  Otherwise, the error status from reading the PE/COFF or TE image using the ImageRead function.

--*/
{
  EFI_STATUS            Status;
  EFI_IMAGE_DOS_HEADER  DosHdr;
  UINTN                 Size;
  UINT16                Magic;

  //
  // Read the DOS image header to check for it's existance
  //
  Size = sizeof (EFI_IMAGE_DOS_HEADER);
  Status = ImageContext->ImageRead (
                           ImageContext->Handle,
                           0,
                           &Size,
                           &DosHdr
                           );
  if (EFI_ERROR (Status)) {
    ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
    return Status;
  }

  ImageContext->PeCoffHeaderOffset = 0;
  if (DosHdr.e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image 
    // header
    //
    ImageContext->PeCoffHeaderOffset = DosHdr.e_lfanew;
  }

  //
  // Read the PE/COFF Header. For PE32 (32-bit) this will read in too much 
  // data, but that should not hurt anythine. Hdr.Pe32->OptionalHeader.Magic
  // determins if this is a PE32 or PE32+ image. The magic is in the same 
  // location in both images.
  //
  Size = sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION);
  Status = ImageContext->ImageRead (
                           ImageContext->Handle,
                           ImageContext->PeCoffHeaderOffset,
                           &Size,
                           Hdr.Pe32
                           );
  if (EFI_ERROR (Status)) {
    ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
    return Status;
  }

  //
  // Use Signature to figure out if we understand the image format
  //
  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    ImageContext->IsTeImage         = TRUE;
    ImageContext->Machine           = Hdr.Te->Machine;
    ImageContext->ImageType         = (UINT16)(Hdr.Te->Subsystem);
    ImageContext->ImageSize         = 0;
    ImageContext->SectionAlignment  = 4096;
    ImageContext->SizeOfHeaders     = sizeof (EFI_TE_IMAGE_HEADER) + (UINTN)Hdr.Te->BaseOfCode - (UINTN)Hdr.Te->StrippedSize;

  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE)  {
    ImageContext->IsTeImage         = FALSE;
    ImageContext->Machine           = Hdr.Pe32->FileHeader.Machine;
    
    //
    // NOTE: We use Machine to identify PE32/PE32+, instead of Magic.
    //       It is for backward-compatibility consideration, because
    //       some system will generate PE32+ image with PE32 Magic.
    //
    if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA32) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_X64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else {
      Magic = Hdr.Pe32->OptionalHeader.Magic;
    }

    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      //
      // PE32 and PE32+ have the same offset for these fields.
      // We use PE32 for both PE32 and PE32+ headers here.
      //
      ImageContext->ImageType         = Hdr.Pe32->OptionalHeader.Subsystem;
      ImageContext->ImageSize         = (UINT64)Hdr.Pe32->OptionalHeader.SizeOfImage;
      ImageContext->SectionAlignment  = Hdr.Pe32->OptionalHeader.SectionAlignment;
      ImageContext->SizeOfHeaders     = Hdr.Pe32->OptionalHeader.SizeOfHeaders;

    } else {
      ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_MACHINE_TYPE;
      return EFI_UNSUPPORTED;    
    }
  } else {
    ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_MACHINE_TYPE;
    return EFI_UNSUPPORTED;
  }

  if (!PeCoffLoaderImageFormatSupported (ImageContext->Machine)) {
    //
    // If the PE/COFF loader does not support the image type return
    // unsupported. This library can suport lots of types of images
    // this does not mean the user of this library can call the entry
    // point of the image. 
    //
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PeCoffLoaderCheckImageType (
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:

  Checks the PE or TE header of a PE/COFF or TE image to determine if it supported

Arguments:

  ImageContext  - The context of the image being loaded

Returns:

  EFI_SUCCESS if the PE/COFF or TE image is supported
  EFI_UNSUPPORTED of the PE/COFF or TE image is not supported.

--*/
{
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
    ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_SUBSYSTEM;
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeCoffLoaderGetImageInfo (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:

  Retrieves information on a PE/COFF image

Arguments:

  This          - Calling context

  ImageContext  - The context of the image being loaded

Returns:

  EFI_SUCCESS if the information on the PE/COFF image was collected.
  EFI_UNSUPPORTED of the PE/COFF image is not supported.
  Otherwise, the error status from reading the PE/COFF image using the 
    ImageContext->ImageRead() function

  EFI_INVALID_PARAMETER   - ImageContext is NULL.

--*/
{
  EFI_STATUS                            Status;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       HdrData;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  EFI_IMAGE_DATA_DIRECTORY              *DebugDirectoryEntry;
  UINTN                                 Size;
  UINTN                                 Index;
  UINTN                                 DebugDirectoryEntryRva;
  UINTN                                 DebugDirectoryEntryFileOffset;
  UINTN                                 SectionHeaderOffset;
  EFI_IMAGE_SECTION_HEADER              SectionHeader;
  EFI_IMAGE_DEBUG_DIRECTORY_ENTRY       DebugEntry;
  UINT32                                NumberOfRvaAndSizes;
  UINT16                                Magic;

  if (NULL == ImageContext) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Assume success
  //
  ImageContext->ImageError  = EFI_IMAGE_ERROR_SUCCESS;

  Hdr.Union = &HdrData;
  Status = PeCoffLoaderGetPeHeader (ImageContext, Hdr);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Verify machine type
  //
  Status = PeCoffLoaderCheckImageType (ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;

  //
  // Retrieve the base address of the image
  //
  if (!(ImageContext->IsTeImage)) {
  	
    //
    // NOTE: We use Machine to identify PE32/PE32+, instead of Magic.
    //       It is for backward-compatibility consideration, because
    //       some system will generate PE32+ image with PE32 Magic.
    //
    if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA32) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_X64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else {
      Magic = Hdr.Pe32->OptionalHeader.Magic;
    }

    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset
      //
      ImageContext->ImageAddress = Hdr.Pe32->OptionalHeader.ImageBase;
    } else {
      //
      // Use PE32+ offset
      //
      ImageContext->ImageAddress = Hdr.Pe32Plus->OptionalHeader.ImageBase;
    }
  } else {
    ImageContext->ImageAddress = (EFI_PHYSICAL_ADDRESS)(Hdr.Te->ImageBase + Hdr.Te->StrippedSize - sizeof (EFI_TE_IMAGE_HEADER));
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
  if ((!(ImageContext->IsTeImage)) && ((Hdr.Pe32->FileHeader.Characteristics & EFI_IMAGE_FILE_RELOCS_STRIPPED) != 0)) {
    ImageContext->RelocationsStripped = TRUE;
  } else {
    ImageContext->RelocationsStripped = FALSE;
  }

  if (!(ImageContext->IsTeImage)) {
    //
    // Use PE32 to access fields that have same offset in PE32 and PE32+
    //
    ImageContext->ImageSize         = (UINT64) Hdr.Pe32->OptionalHeader.SizeOfImage;
    ImageContext->SectionAlignment  = Hdr.Pe32->OptionalHeader.SectionAlignment;
    ImageContext->SizeOfHeaders     = Hdr.Pe32->OptionalHeader.SizeOfHeaders;
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //     
      // Use PE32 offset
      //
      NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
      DebugDirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
    } else {
      //     
      // Use PE32+ offset
      //
      NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
      DebugDirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
    }    
    
    if (NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {

      DebugDirectoryEntryRva = DebugDirectoryEntry->VirtualAddress;

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
                               Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                               );

      for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
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
          ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
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
            ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
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
  } else {
    //
    // Because Te image only extracts base relocations and debug directory entries from
    // Pe image and in Te image header there is not a field to describe the imagesize,
    // we use the largest VirtualAddress plus Size in each directory entry to describe the imagesize
    //
    ImageContext->ImageSize = (UINT64) (Hdr.Te->DataDirectory[0].VirtualAddress + Hdr.Te->DataDirectory[0].Size);
    if(Hdr.Te->DataDirectory[1].VirtualAddress > Hdr.Te->DataDirectory[0].VirtualAddress) {
      ImageContext->ImageSize = (UINT64) (Hdr.Te->DataDirectory[1].VirtualAddress + Hdr.Te->DataDirectory[1].Size);
    }
    ImageContext->SectionAlignment  = 4096;
    ImageContext->SizeOfHeaders     = sizeof (EFI_TE_IMAGE_HEADER) + (UINTN) Hdr.Te->BaseOfCode - (UINTN) Hdr.Te->StrippedSize;

    DebugDirectoryEntry             = &Hdr.Te->DataDirectory[1];
    DebugDirectoryEntryRva          = DebugDirectoryEntry->VirtualAddress;
    SectionHeaderOffset             = (UINTN)(sizeof (EFI_TE_IMAGE_HEADER));

    DebugDirectoryEntryFileOffset   = 0;

    for (Index = 0; Index < Hdr.Te->NumberOfSections; Index++) {
      //
      // Read section header from file
      //
      Size   = sizeof (EFI_IMAGE_SECTION_HEADER);
      Status = ImageContext->ImageRead (
                               ImageContext->Handle,
                               SectionHeaderOffset,
                               &Size,
                               &SectionHeader
                               );
      if (EFI_ERROR (Status)) {
        ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
        return Status;
      }

      if (DebugDirectoryEntryRva >= SectionHeader.VirtualAddress &&
          DebugDirectoryEntryRva < SectionHeader.VirtualAddress + SectionHeader.Misc.VirtualSize) {
        DebugDirectoryEntryFileOffset = DebugDirectoryEntryRva -
                                        SectionHeader.VirtualAddress +
                                        SectionHeader.PointerToRawData +
                                        sizeof (EFI_TE_IMAGE_HEADER) -
                                        Hdr.Te->StrippedSize;
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
          ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
          return Status;
        }

        if (DebugEntry.Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
          ImageContext->DebugDirectoryEntryRva = (UINT32) (DebugDirectoryEntryRva + Index * sizeof (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY));
          return EFI_SUCCESS;
        }
      }
    }
  }

  return EFI_SUCCESS;
}

STATIC
VOID *
PeCoffLoaderImageAddress (
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
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
    ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_IMAGE_ADDRESS;
    return NULL;
  }

  return (CHAR8 *) ((UINTN) ImageContext->ImageAddress + Address);
}

EFI_STATUS
EFIAPI
PeCoffLoaderRelocateImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:

  Relocates a PE/COFF image in memory

Arguments:

  This         - Calling context

  ImageContext - Contains information on the loaded image to relocate

Returns:

  EFI_SUCCESS      if the PE/COFF image was relocated
  EFI_LOAD_ERROR   if the image is not a valid PE/COFF image
  EFI_UNSUPPORTED  not support

--*/
{
  EFI_STATUS                            Status;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
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
  UINT64                                *F64;
  CHAR8                                 *FixupData;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  UINT32                                NumberOfRvaAndSizes;
  UINT16                                Magic;
#ifdef EFI_NT_EMULATOR
  VOID                                  *DllEntryPoint;
  VOID                                  *ModHandle;

  ModHandle = NULL;
#endif

  if (NULL == ImageContext) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Assume success
  //
  ImageContext->ImageError = EFI_IMAGE_ERROR_SUCCESS;

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
  if (ImageContext->DestinationAddress != 0) {
    BaseAddress = ImageContext->DestinationAddress;
  } else {
    BaseAddress = ImageContext->ImageAddress;
  }

  if (!(ImageContext->IsTeImage)) {
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)ImageContext->ImageAddress + ImageContext->PeCoffHeaderOffset);
    
    //
    // NOTE: We use Machine to identify PE32/PE32+, instead of Magic.
    //       It is for backward-compatibility consideration, because
    //       some system will generate PE32+ image with PE32 Magic.
    //
    if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA32) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_X64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else {
      Magic = Hdr.Pe32->OptionalHeader.Magic;
    }

    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset
      //
      Adjust = (UINT64)BaseAddress - Hdr.Pe32->OptionalHeader.ImageBase;
      Hdr.Pe32->OptionalHeader.ImageBase = (UINT32)BaseAddress;
  
      NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
      RelocDir  = &Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    } else {
      //
      // Use PE32+ offset
      //
      Adjust = (UINT64) BaseAddress - Hdr.Pe32Plus->OptionalHeader.ImageBase;
      Hdr.Pe32Plus->OptionalHeader.ImageBase = (UINT64)BaseAddress;

      NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
      RelocDir  = &Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    }

    //
    // Find the relocation block
    // Per the PE/COFF spec, you can't assume that a given data directory
    // is present in the image. You have to check the NumberOfRvaAndSizes in
    // the optional header to verify a desired directory entry is there.
    //

    if (NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC && RelocDir->Size > 0) {
      RelocBase = PeCoffLoaderImageAddress (ImageContext, RelocDir->VirtualAddress);
      RelocBaseEnd = PeCoffLoaderImageAddress (
                      ImageContext,
                      RelocDir->VirtualAddress + RelocDir->Size - 1
                      );
      if ((RelocBase == NULL) || (RelocBaseEnd == NULL)) {
        //
        // If the base start or end address resolved to 0, then fail.
        //
        return EFI_LOAD_ERROR;
      }
    } else {
      //
      // Set base and end to bypass processing below.
      //
      RelocBase = RelocBaseEnd = 0;
    }
  } else {
    Hdr.Te             = (EFI_TE_IMAGE_HEADER *)(UINTN)(ImageContext->ImageAddress);
    Adjust             = (UINT64) (BaseAddress - Hdr.Te->StrippedSize + sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->ImageBase);
    Hdr.Te->ImageBase  = (UINT64) (BaseAddress - Hdr.Te->StrippedSize + sizeof (EFI_TE_IMAGE_HEADER));

    //
    // Find the relocation block
    //
    RelocDir = &Hdr.Te->DataDirectory[0];
    if (RelocDir->Size > 0) {
      RelocBase = (EFI_IMAGE_BASE_RELOCATION *)(UINTN)(
                                      ImageContext->ImageAddress +
                                      RelocDir->VirtualAddress +
                                      sizeof(EFI_TE_IMAGE_HEADER) -
                                      Hdr.Te->StrippedSize
                                      );
      RelocBaseEnd = (EFI_IMAGE_BASE_RELOCATION *) ((UINTN) RelocBase + (UINTN) RelocDir->Size - 1);
    } else {
      //
      // Set base and end to bypass processing below.
      //
      RelocBase = NULL;
      RelocBaseEnd = NULL;
    }
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

      if (FixupBase == NULL) {
        //
        // If the FixupBase address resolved to 0, then fail.
        //
        return EFI_LOAD_ERROR;
      }
    } else {
      FixupBase = (CHAR8 *)(UINTN)(ImageContext->ImageAddress +
                    RelocBase->VirtualAddress +
                    sizeof(EFI_TE_IMAGE_HEADER) - 
                    Hdr.Te->StrippedSize
                    );
    }

    if ((CHAR8 *) RelocEnd < (CHAR8 *) ((UINTN) ImageContext->ImageAddress) ||
        (CHAR8 *) RelocEnd > (CHAR8 *)((UINTN)ImageContext->ImageAddress + 
          (UINTN)ImageContext->ImageSize)) {
      ImageContext->ImageError = EFI_IMAGE_ERROR_FAILED_RELOCATION;
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
        F16   = (UINT16 *) Fixup;
        *F16  = (UINT16) (*F16 + (UINT16)(((UINT32)Adjust) >> 16));
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

      case EFI_IMAGE_REL_BASED_DIR64:
        //
        // For X64 and IPF
        //
        F64 = (UINT64 *) Fixup;
        *F64 = *F64 + (UINT64) Adjust;
        if (FixupData != NULL) {
          FixupData = ALIGN_POINTER (FixupData, sizeof(UINT64));
          *(UINT64 *)(FixupData) = *F64;
          FixupData = FixupData + sizeof(UINT64);
        }
        break;

      case EFI_IMAGE_REL_BASED_HIGHADJ:
        //
        // Return the same EFI_UNSUPPORTED return code as
        // PeCoffLoaderRelocateImageEx() returns if it does not recognize
        // the relocation type.
        //
        ImageContext->ImageError = EFI_IMAGE_ERROR_FAILED_RELOCATION;
        return EFI_UNSUPPORTED;

      default:
        //
        // The common code does not handle some of the stranger IPF relocations
        // PeCoffLoaderRelocateImageEx () addes support for these complex fixups
        // on IPF and is a No-Op on other archtiectures.
        //
        Status = PeCoffLoaderRelocateImageEx (Reloc, Fixup, &FixupData, Adjust);
        if (EFI_ERROR (Status)) {
          ImageContext->ImageError = EFI_IMAGE_ERROR_FAILED_RELOCATION;
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

#ifdef EFI_NT_EMULATOR
  DllEntryPoint           = NULL;
  ImageContext->ModHandle = NULL;
  //
  // Load the DLL if it's not an EBC image.
  //
  if ((ImageContext->PdbPointer != NULL) && 
      (ImageContext->Machine != EFI_IMAGE_MACHINE_EBC)) {
    Status = mPeCoffLoaderWinNtLoadAsDll->Entry (
                                            ImageContext->PdbPointer,
                                            &DllEntryPoint,
                                            &ModHandle
                                            );

    if (!EFI_ERROR (Status) && DllEntryPoint != NULL) {
      ImageContext->EntryPoint  = (EFI_PHYSICAL_ADDRESS) (UINTN) DllEntryPoint;
      ImageContext->ModHandle   = ModHandle;
    }
  }
#endif

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PeCoffLoaderLoadImage (
  IN     EFI_PEI_PE_COFF_LOADER_PROTOCOL       *This,
  IN OUT EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:

  Loads a PE/COFF image into memory

Arguments:

  This         - Calling context

  ImageContext - Contains information on image to load into memory

Returns:

  EFI_SUCCESS            if the PE/COFF image was loaded
  EFI_BUFFER_TOO_SMALL   if the caller did not provide a large enough buffer
  EFI_LOAD_ERROR         if the image is a runtime driver with no relocations
  EFI_INVALID_PARAMETER  if the image address is invalid

--*/
{
  EFI_STATUS                            Status;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT  CheckContext;
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
  UINT32                                NumberOfRvaAndSizes;
  UINT16                                Magic;
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  EFI_IMAGE_RESOURCE_DIRECTORY          *ResourceDirectory;
  EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY    *ResourceDirectoryEntry;
  EFI_IMAGE_RESOURCE_DIRECTORY_STRING   *ResourceDirectoryString;
  EFI_IMAGE_RESOURCE_DATA_ENTRY         *ResourceDataEntry;
#endif

  if (NULL == ImageContext) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Assume success
  //
  ImageContext->ImageError = EFI_IMAGE_ERROR_SUCCESS;

  //
  // Copy the provided context info into our local version, get what we
  // can from the original image, and then use that to make sure everything
  // is legit.
  //
  CopyMem (&CheckContext, ImageContext, sizeof (EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT));

  Status = PeCoffLoaderGetImageInfo (This, &CheckContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure there is enough allocated space for the image being loaded
  //
  if (ImageContext->ImageSize < CheckContext.ImageSize) {
    ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_IMAGE_SIZE;
    return EFI_BUFFER_TOO_SMALL;
  }
  if (ImageContext->ImageAddress == 0) {
    //
    // Image cannot be loaded into 0 address.
    //
    ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_IMAGE_ADDRESS;
    return EFI_INVALID_PARAMETER;
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
      ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_SUBSYSTEM;
      return EFI_LOAD_ERROR;
    }
    //
    // If the image does not contain relocations, and the requested load address
    // is not the linked address, then return an error.
    //
    if (CheckContext.ImageAddress != ImageContext->ImageAddress) {
      ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_IMAGE_ADDRESS;
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Make sure the allocated space has the proper section alignment
  //
  if (!(ImageContext->IsTeImage)) {
    if ((ImageContext->ImageAddress & (CheckContext.SectionAlignment - 1)) != 0) {
      ImageContext->ImageError = EFI_IMAGE_ERROR_INVALID_SECTION_ALIGNMENT;
      return EFI_INVALID_PARAMETER;
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

    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)ImageContext->ImageAddress + ImageContext->PeCoffHeaderOffset);

    FirstSection = (EFI_IMAGE_SECTION_HEADER *) (
                      (UINTN)ImageContext->ImageAddress +
                      ImageContext->PeCoffHeaderOffset +
                      sizeof(UINT32) + 
                      sizeof(EFI_IMAGE_FILE_HEADER) + 
                      Hdr.Pe32->FileHeader.SizeOfOptionalHeader
      );
    NumberOfSections = (UINTN) (Hdr.Pe32->FileHeader.NumberOfSections);
  } else {
    Status = ImageContext->ImageRead (
                            ImageContext->Handle,
                            0,
                            &ImageContext->SizeOfHeaders,
                            (VOID *)(UINTN)ImageContext->ImageAddress
                            );

    Hdr.Te = (EFI_TE_IMAGE_HEADER *)(UINTN)(ImageContext->ImageAddress);

    FirstSection = (EFI_IMAGE_SECTION_HEADER *) (
                      (UINTN)ImageContext->ImageAddress +
                      sizeof(EFI_TE_IMAGE_HEADER)
                      );
    NumberOfSections  = (UINTN) (Hdr.Te->NumberOfSections);

  }

  if (EFI_ERROR (Status)) {
    ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
    return EFI_LOAD_ERROR;
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
    if (ImageContext->IsTeImage) {
      Base = (CHAR8 *)((UINTN) Base + sizeof (EFI_TE_IMAGE_HEADER) - (UINTN)Hdr.Te->StrippedSize);
      End  = (CHAR8 *)((UINTN) End +  sizeof (EFI_TE_IMAGE_HEADER) - (UINTN)Hdr.Te->StrippedSize);
    }

    if (End > MaxEnd) {
      MaxEnd = End;
    }
    //
    // If the base start or end address resolved to 0, then fail.
    //
    if ((Base == NULL) || (End == NULL)) {
      ImageContext->ImageError = EFI_IMAGE_ERROR_SECTION_NOT_LOADED;
      return EFI_LOAD_ERROR;
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
                                Section->PointerToRawData + sizeof (EFI_TE_IMAGE_HEADER) - (UINTN)Hdr.Te->StrippedSize,
                                &Size,
                                Base
                                );
      }

      if (EFI_ERROR (Status)) {
        ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
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

  Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;

  //
  // Get image's entry point
  //
  if (!(ImageContext->IsTeImage)) {

    //
    // NOTE: We use Machine to identify PE32/PE32+, instead of Magic.
    //       It is for backward-compatibility consideration, because
    //       some system will generate PE32+ image with PE32 Magic.
    //
    if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA32) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_IA64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else if (Hdr.Pe32->FileHeader.Machine == EFI_IMAGE_MACHINE_X64) {
      Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    } else {
      Magic = Hdr.Pe32->OptionalHeader.Magic;
    }

    //
    // Sizes of AddressOfEntryPoint are different so we need to do this safely
    //
    ImageContext->EntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)PeCoffLoaderImageAddress (
                                                              ImageContext,
                                                              (UINTN)Hdr.Pe32->OptionalHeader.AddressOfEntryPoint
                                                              );

  } else {
    ImageContext->EntryPoint =  (EFI_PHYSICAL_ADDRESS) (
                                (UINTN)ImageContext->ImageAddress  +
                                (UINTN)Hdr.Te->AddressOfEntryPoint +
                                (UINTN)sizeof(EFI_TE_IMAGE_HEADER) -
                                (UINTN)Hdr.Te->StrippedSize
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
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset
      //
      NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    } else {
      //
      // Use PE32+ offset
      //
      NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC];
    }
 
    if (NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
      ImageContext->FixupDataSize = DirectoryEntry->Size / sizeof (UINT16) * sizeof (UINTN);
    } else {
      ImageContext->FixupDataSize = 0;
    }
  } else {
    DirectoryEntry              = &Hdr.Te->DataDirectory[0];
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
                      Hdr.Te->StrippedSize
                      );
    }

    if (DebugEntry != NULL) {
      TempDebugEntryRva = DebugEntry->RVA;
      if (DebugEntry->RVA == 0 && DebugEntry->FileOffset != 0) {
        Section--;
        if ((UINTN)Section->SizeOfRawData < Section->Misc.VirtualSize) {
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
                                    (UINTN)sizeof (EFI_TE_IMAGE_HEADER) -
                                    (UINTN) Hdr.Te->StrippedSize
                                    );
        }

        if (ImageContext->CodeView == NULL) {
          ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
          return EFI_LOAD_ERROR;
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
                                    DebugEntry->FileOffset + sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize,
                                    &Size,
                                    ImageContext->CodeView
                                    );
            //
            // Should we apply fix up to this field according to the size difference between PE and TE?
            // Because now we maintain TE header fields unfixed, this field will also remain as they are
            // in original PE image.
            //
          }

          if (EFI_ERROR (Status)) {
            ImageContext->ImageError = EFI_IMAGE_ERROR_IMAGE_READ;
            return EFI_LOAD_ERROR;
          }

          DebugEntry->RVA = TempDebugEntryRva;
        }

        switch (*(UINT32 *) ImageContext->CodeView) {
        case CODEVIEW_SIGNATURE_NB10:
          ImageContext->PdbPointer = (CHAR8 *)ImageContext->CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
          break;

        case CODEVIEW_SIGNATURE_RSDS:
          ImageContext->PdbPointer = (CHAR8 *)ImageContext->CodeView + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
          break;

        default:
          break;
        }
      }
    }
  }

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  //
  // Get Image's HII resource section
  //
  if (!(ImageContext->IsTeImage)) {
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset
      //
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE];
    } else {
      //
      // Use PE32+ offset
      //
      DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_RESOURCE];
    }

    if (DirectoryEntry->Size != 0) {
      Base = PeCoffLoaderImageAddress (ImageContext, DirectoryEntry->VirtualAddress);

      ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) Base;
      ResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (ResourceDirectory + 1);

      for (Index = 0; Index < ResourceDirectory->NumberOfNamedEntries; Index++) {
        if (ResourceDirectoryEntry->u1.s.NameIsString) {
          ResourceDirectoryString = (EFI_IMAGE_RESOURCE_DIRECTORY_STRING *) (Base + ResourceDirectoryEntry->u1.s.NameOffset);

          if (ResourceDirectoryString->Length == 3 &&
              ResourceDirectoryString->String[0] == L'H' &&
              ResourceDirectoryString->String[1] == L'I' &&
              ResourceDirectoryString->String[2] == L'I') {
            //
            // Resource Type "HII" found
            //
            if (ResourceDirectoryEntry->u2.s.DataIsDirectory) {
              //
              // Move to next level - resource Name
              //
              ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) (Base + ResourceDirectoryEntry->u2.s.OffsetToDirectory);
              ResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (ResourceDirectory + 1);

              if (ResourceDirectoryEntry->u2.s.DataIsDirectory) {
                //
                // Move to next level - resource Language
                //
                ResourceDirectory = (EFI_IMAGE_RESOURCE_DIRECTORY *) (Base + ResourceDirectoryEntry->u2.s.OffsetToDirectory);
                ResourceDirectoryEntry = (EFI_IMAGE_RESOURCE_DIRECTORY_ENTRY *) (ResourceDirectory + 1);
              }
            }

            //
            // Now it ought to be resource Data
            //
            if (!ResourceDirectoryEntry->u2.s.DataIsDirectory) {
              ResourceDataEntry = (EFI_IMAGE_RESOURCE_DATA_ENTRY *) (Base + ResourceDirectoryEntry->u2.OffsetToData);
              ImageContext->HiiResourceData = (EFI_PHYSICAL_ADDRESS) (UINTN) PeCoffLoaderImageAddress (ImageContext, ResourceDataEntry->OffsetToData);
              break;
            }
          }
        }

        ResourceDirectoryEntry++;
      }
    }
  }
#endif

#if defined (EFI_DEBUG_ITP_BREAK) && !defined (_CONSOLE)
  AsmEfiSetBreakSupport ((UINTN)(ImageContext->ImageAddress));
#endif

  return Status;
}

EFI_STATUS
EFIAPI
PeCoffLoaderUnloadImage (
  IN EFI_PEI_PE_COFF_LOADER_IMAGE_CONTEXT   *ImageContext
  )
/*++

Routine Description:

  Unload a PE/COFF image from memory

Arguments:

  ImageContext - Contains information on image to load into memory

Returns:

  EFI_SUCCESS            

--*/
{
#ifdef EFI_NT_EMULATOR
  //
  // Calling Win32 API free library
  //
  mPeCoffLoaderWinNtLoadAsDll->FreeLibrary (ImageContext->ModHandle);

#endif

  return EFI_SUCCESS;
}
