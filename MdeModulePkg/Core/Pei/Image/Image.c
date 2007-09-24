/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Image.c

Abstract:

  Pei Core Load Image Support

--*/

#include <PeiMain.h>

/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file
  FileOffset - The offset, in bytes, into the file to read
  ReadSize   - The number of bytes to read from the file starting at FileOffset
  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/  

EFI_STATUS
PeiLoadImageLoadImage (
  IN     EFI_PEI_SERVICES             **PeiServices,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
/*++

Routine Description:

  Routine for loading file image.

Arguments:

  PeiServices          - The PEI core services table.
  FileHandle           - Pointer to the FFS file header of the image.
  ImageAddressArg      - Pointer to PE/TE image.
  ImageSizeArg         - Size of PE/TE image.
  EntryPoint           - Pointer to entry point of specified image file for output.
  AuthenticationState  - Pointer to attestation authentication state of image.

Returns:

  Status - EFI_SUCCESS    - Image is successfully loaded.
           EFI_NOT_FOUND  - Fail to locate necessary PPI
           Others         - Fail to load file.

--*/
;

EFI_STATUS
EFIAPI
PeiLoadImageLoadImageWrapper (
  IN     CONST EFI_PEI_LOAD_FILE_PPI  *This,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
/*++

Routine Description:

  The wrapper function of PeiLoadImageLoadImage().

Arguments:

  This                 - Pointer to EFI_PEI_LOAD_FILE_PPI.
  PeiServices          - The PEI core services table.
  FileHandle           - Pointer to the FFS file header of the image.
  ImageAddressArg      - Pointer to PE/TE image.
  ImageSizeArg         - Size of PE/TE image.
  EntryPoint           - Pointer to entry point of specified image file for output.
  AuthenticationState  - Pointer to attestation authentication state of image.

Returns:

  EFI_STATUS.
  
--*/ 
;

STATIC EFI_PEI_LOAD_FILE_PPI   mPeiLoadImagePpi = {
  PeiLoadImageLoadImageWrapper
};


STATIC EFI_PEI_PPI_DESCRIPTOR     gPpiLoadFilePpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiLoadFilePpiGuid,
  &mPeiLoadImagePpi
};

EFI_STATUS
EFIAPI
PeiImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file
  FileOffset - The offset, in bytes, into the file to read
  ReadSize   - The number of bytes to read from the file starting at FileOffset
  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8 *Destination8;
  CHAR8 *Source8;
  UINTN Length;

  Destination8  = Buffer;
  Source8       = (CHAR8 *) ((UINTN) FileHandle + FileOffset);
  Length        = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetImageReadFunction (
  IN      PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
/*++

Routine Description:

  Support routine to return the Image Read

Arguments:

  PeiServices   - PEI Services Table

  ImageContext  - The context of the image being loaded

Returns:

  EFI_SUCCESS - If Image function location is found

--*/
{
  VOID*  MemoryBuffer;

  MemoryBuffer = AllocatePages (0x400 / EFI_PAGE_SIZE + 1);
  ASSERT (MemoryBuffer != NULL);

  CopyMem (MemoryBuffer, (CONST VOID *) (UINTN) PeiImageRead, 0x400);

  ImageContext->ImageRead = (PE_COFF_LOADER_READ_FILE) (UINTN) MemoryBuffer;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
LoadAndRelocatePeCoffImage (
  IN  EFI_PEI_PE_COFF_LOADER_PROTOCOL           *PeiEfiPeiPeCoffLoader,
  IN  VOID                                      *Pe32Data,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
/*++

Routine Description:

  Loads and relocates a PE/COFF image into memory.

Arguments:

  PeiEfiPeiPeCoffLoader - Pointer to a PE COFF loader protocol

  Pe32Data         - The base address of the PE/COFF file that is to be loaded and relocated

  ImageAddress     - The base address of the relocated PE/COFF image

  ImageSize        - The size of the relocated PE/COFF image

  EntryPoint       - The entry point of the relocated PE/COFF image

Returns:

  EFI_SUCCESS   - The file was loaded and relocated

  EFI_OUT_OF_RESOURCES - There was not enough memory to load and relocate the PE/COFF file

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;

  ASSERT (PeiEfiPeiPeCoffLoader != NULL);

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle = Pe32Data;
  Status              = GetImageReadFunction (&ImageContext);

  ASSERT_EFI_ERROR (Status);

  Status = PeiEfiPeiPeCoffLoader->GetImageInfo (PeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Allocate Memory for the image
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) AllocatePages (EFI_SIZE_TO_PAGES ((UINT32) ImageContext.ImageSize));
  ASSERT (ImageContext.ImageAddress != 0);

  //
  // Load the image to our new buffer
  //
  Status = PeiEfiPeiPeCoffLoader->LoadImage (PeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Relocate the image in our new buffer
  //
  Status = PeiEfiPeiPeCoffLoader->RelocateImage (PeiEfiPeiPeCoffLoader, &ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Flush the instruction cache so the image data is written before we execute it
  //
  InvalidateInstructionCacheRange ((VOID *)(UINTN)ImageContext.ImageAddress, (UINTN)ImageContext.ImageSize);

  *ImageAddress = ImageContext.ImageAddress;
  *ImageSize    = ImageContext.ImageSize;
  *EntryPoint   = ImageContext.EntryPoint;

  return EFI_SUCCESS;
}

EFI_STATUS
PeiLoadImageLoadImage (
  IN     EFI_PEI_SERVICES             **PeiServices,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
/*++

Routine Description:

  Routine for loading file image.

Arguments:

  PeiServices          - The PEI core services table.
  FileHandle           - Pointer to the FFS file header of the image.
  ImageAddressArg      - Pointer to PE/TE image.
  ImageSizeArg         - Size of PE/TE image.
  EntryPoint           - Pointer to entry point of specified image file for output.
  AuthenticationState  - Pointer to attestation authentication state of image.

Returns:

  Status - EFI_SUCCESS    - Image is successfully loaded.
           EFI_NOT_FOUND  - Fail to locate necessary PPI
           Others         - Fail to load file.

--*/
{
  EFI_STATUS                  Status;
  VOID                        *Pe32Data;
  EFI_PHYSICAL_ADDRESS        ImageAddress;
  UINT64                      ImageSize;
  EFI_PHYSICAL_ADDRESS        ImageEntryPoint;
  EFI_TE_IMAGE_HEADER         *TEImageHeader;
  UINT16                      Machine;
  PEI_CORE_INSTANCE           *Private;
  VOID                        *EntryPointArg;

  *EntryPoint   = 0;
  TEImageHeader = NULL;
  ImageSize = 0;
  *AuthenticationState = 0;

  //
  // Try to find a TE section.
  //
  Status = PeiServicesFfsFindSectionData (
             EFI_SECTION_TE,
             FileHandle,
             &Pe32Data
             );
  if (!EFI_ERROR (Status)) {
     TEImageHeader = (EFI_TE_IMAGE_HEADER *)Pe32Data;
  }
  //
  // If we didn't find a PE32 section, try to find a TE section.
  //
  if (EFI_ERROR (Status)) {
    Status = PeiServicesFfsFindSectionData (
               EFI_SECTION_PE32,
               FileHandle,
               &Pe32Data
               );
    if (EFI_ERROR (Status)) {
      //
      // PEI core only carry the loader function fro TE and PE32 executables
      // If this two section does not exist, just return.
      //
      return Status;
    }
  }
  
  Private = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  if (Private->PeiMemoryInstalled && 
      (Private->HobList.HandoffInformationTable->BootMode != BOOT_ON_S3_RESUME)) {
    {
      //
      // If memory is installed, perform the shadow operations
      //
      Status = LoadAndRelocatePeCoffImage (
        Private->PeCoffLoader,
        Pe32Data,
        &ImageAddress,
        &ImageSize,
        &ImageEntryPoint
      );

      if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
      }

      //
      // Got the entry point from ImageEntryPoint and ImageStartAddress
      //
      Pe32Data    = (VOID *) ((UINTN) ImageAddress);
      *EntryPoint = ImageEntryPoint;
    }
  } else {
   if (TEImageHeader != NULL) {
      //
      // Retrieve the entry point from the TE image header
      //
      ImageAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) TEImageHeader;
      ImageSize = 0;
      *EntryPoint  = (EFI_PHYSICAL_ADDRESS)((UINTN) TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER) +
                    TEImageHeader->AddressOfEntryPoint - TEImageHeader->StrippedSize);
    } else {
      //
      // Retrieve the entry point from the PE/COFF image header
      //
      ImageAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) Pe32Data;
      ImageSize = 0;
      Status = PeCoffLoaderGetEntryPoint (Pe32Data, &EntryPointArg);
      *EntryPoint = (EFI_PHYSICAL_ADDRESS) (UINTN) EntryPointArg;
      if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
      }
    }
  }

  if (((EFI_TE_IMAGE_HEADER *) (UINTN) ImageAddress)->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    TEImageHeader = (EFI_TE_IMAGE_HEADER *) (UINTN) ImageAddress;
    Machine = TEImageHeader->Machine;
  } else {
    Machine = PeCoffLoaderGetMachineType (Pe32Data);
  } 
  
  if (!EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Machine)) {
    return EFI_UNSUPPORTED;  
  }

  if (ImageAddressArg != NULL) {
    *ImageAddressArg = ImageAddress;
  }

  if (ImageSizeArg != NULL) {
    *ImageSizeArg = ImageSize;
  }
  
  //
  // Print debug message: Loading PEIM at 0x12345678 EntryPoint=0x12345688 Driver.efi
  //
  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Loading PEIM at 0x%08x EntryPoint=0x%08x ", (UINTN) ImageAddress, *EntryPoint));
  DEBUG_CODE_BEGIN ();
    EFI_IMAGE_DATA_DIRECTORY            *DirectoryEntry;
    EFI_IMAGE_DEBUG_DIRECTORY_ENTRY     *DebugEntry;
    UINTN                               DirCount;
    UINTN                               Index;
    UINTN                               Index1;
    BOOLEAN                             FileNameFound;
    CHAR8                               *AsciiString;
    CHAR8                               AsciiBuffer[512];
    VOID                                *CodeViewEntryPointer;
    INTN                                TEImageAdjust;
    EFI_IMAGE_DOS_HEADER                *DosHeader;
    EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION Hdr;
    UINT32                              NumberOfRvaAndSizes;

    Hdr.Pe32 = NULL;
    if (TEImageHeader == NULL) {
      DosHeader = (EFI_IMAGE_DOS_HEADER *)Pe32Data;
      if (DosHeader->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
        //
        // DOS image header is present, so read the PE header after the DOS image header
        //
        Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINTN)Pe32Data + (UINTN)((DosHeader->e_lfanew) & 0x0ffff));
      } else {
        //
        // DOS image header is not present, so PE header is at the image base
        //
        Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
      }
    }

    //
    // Find the codeview info in the image and display the file name
    // being loaded.
    //
    // Per the PE/COFF spec, you can't assume that a given data directory
    // is present in the image. You have to check the NumberOfRvaAndSizes in
    // the optional header to verify a desired directory entry is there.
    //
    DebugEntry          = NULL;
    DirectoryEntry      = NULL;
    NumberOfRvaAndSizes = 0;
    TEImageAdjust       = 0;
    
    if (TEImageHeader == NULL) {
      if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        //     
        // Use PE32 offset get Debug Directory Entry
        //
        NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
        DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *)&(Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
        DebugEntry     = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) ((UINTN) Pe32Data + DirectoryEntry->VirtualAddress);
      } else if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
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
      if (TEImageHeader->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress != 0) {
        DirectoryEntry  = &TEImageHeader->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG];
        TEImageAdjust   = sizeof (EFI_TE_IMAGE_HEADER) - TEImageHeader->StrippedSize;
        DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *)((UINTN) TEImageHeader +
                      TEImageHeader->DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress +
                      TEImageAdjust);
      }
    }

    if (DebugEntry != NULL && DirectoryEntry != NULL) {
      for (DirCount = 0; DirCount < DirectoryEntry->Size; DirCount += sizeof(EFI_IMAGE_DEBUG_DIRECTORY_ENTRY), DebugEntry++) {
        if (DebugEntry->Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
          if (DebugEntry->SizeOfData > 0) {
            CodeViewEntryPointer = (VOID *) ((UINTN) DebugEntry->RVA + (UINTN) ImageAddress + (UINTN)TEImageAdjust);
            switch (* (UINT32 *) CodeViewEntryPointer) {
              case CODEVIEW_SIGNATURE_NB10:
                AsciiString = (CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
                break;

              case CODEVIEW_SIGNATURE_RSDS:
                AsciiString = (CHAR8 *)CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
                break;

              default:
                AsciiString = NULL;
                break;
            }
            if (AsciiString != NULL) {
              FileNameFound = FALSE;
              for (Index = 0, Index1 = 0; AsciiString[Index] != '\0'; Index++) {
                if (AsciiString[Index] == '\\') {
                  Index1 = Index;
                  FileNameFound = TRUE;
                }
              }

              if (FileNameFound) {
                for (Index = Index1 + 1; AsciiString[Index] != '.'; Index++) {
                  AsciiBuffer[Index - (Index1 + 1)] = AsciiString[Index];
                }
                AsciiBuffer[Index - (Index1 + 1)] = 0;
                DEBUG ((EFI_D_INFO | EFI_D_LOAD, "%a.efi", AsciiBuffer));
                break;
              }
            }
          }
        }
      }
    }
  DEBUG_CODE_END ();

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "\n"));

  return EFI_SUCCESS;

}


EFI_STATUS
EFIAPI
PeiLoadImageLoadImageWrapper (
  IN     CONST EFI_PEI_LOAD_FILE_PPI  *This,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
/*++

Routine Description:

  The wrapper function of PeiLoadImageLoadImage().

Arguments:

  This                 - Pointer to EFI_PEI_LOAD_FILE_PPI.
  PeiServices          - The PEI core services table.
  FileHandle           - Pointer to the FFS file header of the image.
  ImageAddressArg      - Pointer to PE/TE image.
  ImageSizeArg         - Size of PE/TE image.
  EntryPoint           - Pointer to entry point of specified image file for output.
  AuthenticationState  - Pointer to attestation authentication state of image.

Returns:

  EFI_STATUS.
  
--*/      
{
  return PeiLoadImageLoadImage (
           GetPeiServicesTablePointer (),
           FileHandle,
           ImageAddressArg,
           ImageSizeArg,
           EntryPoint,
           AuthenticationState
           );
}

EFI_STATUS
PeiLoadImage (
  IN     EFI_PEI_SERVICES             **PeiServices,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
/*++

Routine Description:

  Routine for load image file.

Arguments:

  PeiServices          - The PEI core services table.
  FileHandle           - Pointer to the FFS file header of the image.
  EntryPoint           - Pointer to entry point of specified image file for output.
  AuthenticationState  - Pointer to attestation authentication state of image.

Returns:

  Status - EFI_SUCCESS    - Image is successfully loaded.
           EFI_NOT_FOUND  - Fail to locate necessary PPI
           Others         - Fail to load file.
  
--*/    
{
  EFI_STATUS              PpiStatus;
  EFI_STATUS              Status;
  UINTN                   Index;
  EFI_PEI_LOAD_FILE_PPI   *LoadFile;
  EFI_PHYSICAL_ADDRESS    ImageAddress;
  UINT64                  ImageSize;

  //
  // If any instances of PEI_LOAD_FILE_PPI are installed, they are called.
  // one at a time, until one reports EFI_SUCCESS.
  //
  Index = 0;
  do {
    PpiStatus = PeiServicesLocatePpi (
                  &gEfiPeiLoadFilePpiGuid,
                  Index,
                  NULL,
                  (VOID **)&LoadFile
                  );
    if (!EFI_ERROR (PpiStatus)) {
      Status = LoadFile->LoadFile (
                          LoadFile, 
                          FileHandle, 
                          &ImageAddress, 
                          &ImageSize,
                          EntryPoint,
                          AuthenticationState
                          );
      if (!EFI_ERROR (Status)) {
        return Status;
      }
    }
    Index++;
  } while (!EFI_ERROR (PpiStatus));

  //
  // If no instances reports EFI_SUCCESS, then build-in support for
  // the PE32+/TE XIP image format is used.
  //
  Status = PeiLoadImageLoadImage (
            PeiServices, 
            FileHandle, 
            NULL, 
            NULL, 
            EntryPoint, 
            AuthenticationState
            );
  return Status;
}


VOID
InitializeImageServices (
  IN  PEI_CORE_INSTANCE   *PrivateData,
  IN  PEI_CORE_INSTANCE   *OldCoreData
  )
/*++

Routine Description:

  Regitser PeCoffLoader to PeiCore PrivateData. And install
  Pei Load File PPI.

Arguments:

  PrivateData    - Pointer to PEI_CORE_INSTANCE.
  OldCoreData    - Pointer to PEI_CORE_INSTANCE.

Returns:

  NONE.
  
--*/      
{
  //
  // Always update PeCoffLoader pointer as PEI core itself may get 
  // shadowed into memory
  //
  PrivateData->PeCoffLoader = GetPeCoffLoaderProtocol ();
  
  if (OldCoreData == NULL) {
    //
    // The first time we are XIP (running from FLASH). We need to remember the
    // FLASH address so we can reinstall the memory version that runs faster
    //
    PrivateData->XipLoadFile = &gPpiLoadFilePpiList;
    PeiServicesInstallPpi (PrivateData->XipLoadFile);
  } else {
    //
    // 2nd time we are running from memory so replace the XIP version with the 
    // new memory version. 
    //
    PeiServicesReInstallPpi (PrivateData->XipLoadFile, &gPpiLoadFilePpiList); 
  }
}



