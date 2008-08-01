/** @file
  Pei Core Load Image Support
  
Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PeiMain.h>

/**
  Routine for loading file image.

  @param PeiServices      The PEI core services table.
  @param FileHandle       Pointer to the FFS file header of the image.
  @param ImageAddressArg  Pointer to PE/TE image.
  @param ImageSizeArg     Size of PE/TE image.
  @param EntryPoint       Pointer to entry point of specified image file for output.
  @param AuthenticationState - Pointer to attestation authentication state of image.

  @retval EFI_SUCCESS    - Image is successfully loaded.
  @retval EFI_NOT_FOUND  - Fail to locate necessary PPI
  @retval Others         - Fail to load file.

**/
EFI_STATUS
PeiLoadImageLoadImage (
  IN     EFI_PEI_SERVICES             **PeiServices,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
;

/**
  The wrapper function of PeiLoadImageLoadImage().

  @param This            - Pointer to EFI_PEI_LOAD_FILE_PPI.
  @param FileHandle      - Pointer to the FFS file header of the image.
  @param ImageAddressArg - Pointer to PE/TE image.
  @param ImageSizeArg    - Size of PE/TE image.
  @param EntryPoint      - Pointer to entry point of specified image file for output.
  @param AuthenticationState - Pointer to attestation authentication state of image.

  @return Status of PeiLoadImageLoadImage().

**/
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
;

STATIC EFI_PEI_LOAD_FILE_PPI   mPeiLoadImagePpi = {
  PeiLoadImageLoadImageWrapper
};


STATIC EFI_PEI_PPI_DESCRIPTOR     gPpiLoadFilePpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiLoadFilePpiGuid,
  &mPeiLoadImagePpi
};

/**

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file


  @param FileHandle      - The handle to the PE/COFF file
  @param FileOffset      - The offset, in bytes, into the file to read
  @param ReadSize        - The number of bytes to read from the file starting at FileOffset
  @param Buffer          - A pointer to the buffer to read the data into.

  @return EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

**/
EFI_STATUS
EFIAPI
PeiImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
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

/**

  Support routine to return the Image Read.

  @param ImageContext    - The context of the image being loaded

  @retval EFI_SUCCESS - If Image function location is found

**/
EFI_STATUS
GetImageReadFunction (
  IN      PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  VOID*  MemoryBuffer;

  MemoryBuffer = AllocatePages (0x400 / EFI_PAGE_SIZE + 1);
  ASSERT (MemoryBuffer != NULL);

  CopyMem (MemoryBuffer, (CONST VOID *) (UINTN) PeiImageRead, 0x400);

  ImageContext->ImageRead = (PE_COFF_LOADER_READ_FILE) (UINTN) MemoryBuffer;

  return EFI_SUCCESS;
}

/**

  Loads and relocates a PE/COFF image into memory.


  @param Pe32Data        - The base address of the PE/COFF file that is to be loaded and relocated
  @param ImageAddress    - The base address of the relocated PE/COFF image
  @param ImageSize       - The size of the relocated PE/COFF image
  @param EntryPoint      - The entry point of the relocated PE/COFF image

  @retval EFI_SUCCESS           The file was loaded and relocated
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory to load and relocate the PE/COFF file

**/
EFI_STATUS
LoadAndRelocatePeCoffImage (
  IN  VOID                                      *Pe32Data,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  )
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle = Pe32Data;
  Status              = GetImageReadFunction (&ImageContext);

  ASSERT_EFI_ERROR (Status);

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // When Image has no reloc section, it can't be relocated into memory.
  //
  if (ImageContext.RelocationsStripped) {
    DEBUG ((EFI_D_ERROR, "The image at 0x%08x without reloc section can't be loaded into memory\n", (UINTN) Pe32Data));
    return EFI_INVALID_PARAMETER;
  }
  //
  // Allocate Memory for the image
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN) AllocatePages (EFI_SIZE_TO_PAGES ((UINT32) ImageContext.ImageSize));
  ASSERT (ImageContext.ImageAddress != 0);
  
  //
  // Skip the reserved space for the stripped PeHeader when load TeImage into memory.
  //
  if (ImageContext.IsTeImage) {
    ImageContext.ImageAddress = ImageContext.ImageAddress + 
                                ((EFI_TE_IMAGE_HEADER *) Pe32Data)->StrippedSize -
                                sizeof (EFI_TE_IMAGE_HEADER);
  }

  //
  // Load the image to our new buffer
  //
  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Relocate the image in our new buffer
  //
  Status = PeCoffLoaderRelocateImage (&ImageContext);
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

/**
  Routine for loading file image.

  @param PeiServices      The PEI core services table.
  @param FileHandle       Pointer to the FFS file header of the image.
  @param ImageAddressArg  Pointer to PE/TE image.
  @param ImageSizeArg     Size of PE/TE image.
  @param EntryPoint       Pointer to entry point of specified image file for output.
  @param AuthenticationState - Pointer to attestation authentication state of image.

  @retval EFI_SUCCESS    - Image is successfully loaded.
  @retval EFI_NOT_FOUND  - Fail to locate necessary PPI
  @retval Others         - Fail to load file.

**/
EFI_STATUS
PeiLoadImageLoadImage (
  IN     EFI_PEI_SERVICES             **PeiServices,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
{
  EFI_STATUS                  Status;
  VOID                        *Pe32Data;
  EFI_PHYSICAL_ADDRESS        ImageAddress;
  UINT64                      ImageSize;
  EFI_PHYSICAL_ADDRESS        ImageEntryPoint;
  UINT16                      Machine;
  PEI_CORE_INSTANCE           *Private;
  VOID                        *EntryPointArg;
  EFI_SECTION_TYPE            SearchType1;
  EFI_SECTION_TYPE            SearchType2;

  *EntryPoint          = 0;
  ImageSize            = 0;
  *AuthenticationState = 0;

  if (FeaturePcdGet (PcdPeiCoreImageLoaderSearchTeSectionFirst)) {
    SearchType1 = EFI_SECTION_TE;
    SearchType2 = EFI_SECTION_PE32;
  } else {
    SearchType1 = EFI_SECTION_PE32;
    SearchType2 = EFI_SECTION_TE;
  }
  //
  // Try to find a first exe section (if PcdPeiCoreImageLoaderSearchTeSectionFirst 
  // is true, TE will be searched first).
  //
  Status = PeiServicesFfsFindSectionData (
             SearchType1,
             FileHandle,
             &Pe32Data
             );
  //
  // If we didn't find a first exe section, try to find the second exe section.
  //
  if (EFI_ERROR (Status)) {
    Status = PeiServicesFfsFindSectionData (
               SearchType2,
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
    //
    // If memory is installed, perform the shadow operations
    //
    Status = LoadAndRelocatePeCoffImage (
      Pe32Data,
      &ImageAddress,
      &ImageSize,
      &ImageEntryPoint
    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Got the entry point from the loaded Pe32Data
    //
    Pe32Data    = (VOID *) ((UINTN) ImageAddress);
    *EntryPoint = ImageEntryPoint;
  } else {
    //
    // Retrieve the entry point from the PE/COFF or TE image header
    //
    ImageAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) Pe32Data;
    Status = PeCoffLoaderGetEntryPoint (Pe32Data, &EntryPointArg);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    *EntryPoint = (EFI_PHYSICAL_ADDRESS) (UINTN) EntryPointArg;
  }
  
  Machine = PeCoffLoaderGetMachineType (Pe32Data);
  
  if (!EFI_IMAGE_MACHINE_TYPE_SUPPORTED (Machine)) {
    return EFI_UNSUPPORTED;  
  }

  if (ImageAddressArg != NULL) {
    *ImageAddressArg = ImageAddress;
  }

  if (ImageSizeArg != NULL) {
    *ImageSizeArg = ImageSize;
  }
  
  DEBUG_CODE_BEGIN ();
    CHAR8                              *AsciiString;
    CHAR8                              AsciiBuffer[512];
    INT32                              Index;
    INT32                              Index1;

    //
    // Print debug message: Loading PEIM at 0x12345678 EntryPoint=0x12345688 Driver.efi
    //
    if (Machine != IMAGE_FILE_MACHINE_IA64) {
      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Loading PEIM at 0x%11p EntryPoint=0x%11p ", (VOID *)(UINTN)ImageAddress, (VOID *)(UINTN)*EntryPoint));
    } else {
      //
      // For IPF Image, the real entry point should be print.
      //
      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Loading PEIM at 0x%11p EntryPoint=0x%11p ", (VOID *)(UINTN)ImageAddress, (VOID *)(UINTN)(*(UINT64 *)(UINTN)*EntryPoint)));
    }
    
    //
    // Print Module Name by PeImage PDB file name.
    //
    AsciiString = PeCoffLoaderGetPdbPointer (Pe32Data);
    
    if (AsciiString != NULL) {
      for (Index = (INT32) AsciiStrLen (AsciiString) - 1; Index >= 0; Index --) {
        if (AsciiString[Index] == '\\') {
          break;
        }
      }

      if (Index != 0) {
        for (Index1 = 0; AsciiString[Index + 1 + Index1] != '.'; Index1 ++) {
          AsciiBuffer [Index1] = AsciiString[Index + 1 + Index1];
        }
        AsciiBuffer [Index1] = '\0';
        DEBUG ((EFI_D_INFO | EFI_D_LOAD, "%a.efi", AsciiBuffer));
      }
    }

  DEBUG_CODE_END ();

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "\n"));

  return EFI_SUCCESS;

}


/**
  The wrapper function of PeiLoadImageLoadImage().

  @param This            - Pointer to EFI_PEI_LOAD_FILE_PPI.
  @param FileHandle      - Pointer to the FFS file header of the image.
  @param ImageAddressArg - Pointer to PE/TE image.
  @param ImageSizeArg    - Size of PE/TE image.
  @param EntryPoint      - Pointer to entry point of specified image file for output.
  @param AuthenticationState - Pointer to attestation authentication state of image.

  @return Status of PeiLoadImageLoadImage().

**/
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

/**

  Routine for load image file.


  @param PeiServices     - The PEI core services table.
  @param FileHandle      - Pointer to the FFS file header of the image.
  @param EntryPoint      - Pointer to entry point of specified image file for output.
  @param AuthenticationState - Pointer to attestation authentication state of image.

  @retval EFI_SUCCESS    - Image is successfully loaded.
  @retval EFI_NOT_FOUND  - Fail to locate necessary PPI
  @retval Others         - Fail to load file.

**/
EFI_STATUS
PeiLoadImage (
  IN     EFI_PEI_SERVICES             **PeiServices,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  )
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


/**

  Install Pei Load File PPI.


  @param PrivateData     - Pointer to PEI_CORE_INSTANCE.
  @param OldCoreData     - Pointer to PEI_CORE_INSTANCE.

**/
VOID
InitializeImageServices (
  IN  PEI_CORE_INSTANCE   *PrivateData,
  IN  PEI_CORE_INSTANCE   *OldCoreData
  )
{
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



