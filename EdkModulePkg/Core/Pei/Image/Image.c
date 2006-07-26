/*++

Copyright (c) 2006, Intel Corporation                                                         
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



EFI_STATUS
PeiLoadImage (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_FFS_FILE_HEADER      *PeimFileHeader,
  OUT VOID                    **EntryPoint
  )
/*++

Routine Description:

  Routine for loading file image.

Arguments:

  PeiServices     - The PEI core services table.
  PeimFileHeader  - Pointer to the FFS file header of the image.
  EntryPoint      - Pointer to entry point of specified image file for output.

Returns:

  Status - EFI_SUCCESS    - Image is successfully loaded.
           EFI_NOT_FOUND  - Fail to locate necessary PPI
           Others         - Fail to load file.

--*/
{
  EFI_STATUS                  Status;
  VOID                        *Pe32Data;
  EFI_PEI_FV_FILE_LOADER_PPI  *FvLoadFilePpi;
  EFI_PHYSICAL_ADDRESS        ImageAddress;
  UINT64                      ImageSize;
  EFI_PHYSICAL_ADDRESS        ImageEntryPoint;
  EFI_TE_IMAGE_HEADER         *TEImageHeader;

  *EntryPoint   = NULL;
  TEImageHeader = NULL;

  //
  // Try to find a PE32 section.
  //
  Status = PeiServicesFfsFindSectionData (
             EFI_SECTION_PE32,
             PeimFileHeader,
             &Pe32Data
             );
  //
  // If we didn't find a PE32 section, try to find a TE section.
  //
  if (EFI_ERROR (Status)) {
    Status = PeiServicesFfsFindSectionData (
               EFI_SECTION_TE,
               PeimFileHeader,
               (VOID **) &TEImageHeader
               );
    if (EFI_ERROR (Status) || TEImageHeader == NULL) {
      //
      // There was not a PE32 or a TE section, so assume that it's a Compressed section
      // and use the LoadFile
      //
      Status = PeiServicesLocatePpi (
                &gEfiPeiFvFileLoaderPpiGuid,
                0,
                NULL,
                (VOID **)&FvLoadFilePpi
                );
      if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
      }

      Status = FvLoadFilePpi->FvLoadFile (
                                FvLoadFilePpi,
                                PeimFileHeader,
                                &ImageAddress,
                                &ImageSize,
                                &ImageEntryPoint
                                );

      if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
      }

      //
      // Got the entry point from ImageEntryPoint
      //
      *EntryPoint = (VOID *) ((UINTN) ImageEntryPoint);
      return EFI_SUCCESS;
    } else {
      //
      // Retrieve the entry point from the TE image header
      //
      ImageAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) TEImageHeader;
      *EntryPoint = (VOID *)((UINTN) TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER) +
                    TEImageHeader->AddressOfEntryPoint - TEImageHeader->StrippedSize);
    }
  } else {
    //
    // Retrieve the entry point from the PE/COFF image header
    //
    ImageAddress = (EFI_PHYSICAL_ADDRESS) (UINTN) Pe32Data;
    Status = PeCoffLoaderGetEntryPoint (Pe32Data, EntryPoint);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }
  }

  //
  // Print debug message: Loading PEIM at 0x12345678 EntryPoint=0x12345688 Driver.efi
  //
  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "Loading PEIM at 0x%08x EntryPoint=0x%08x ", Pe32Data, *EntryPoint));
  DEBUG_CODE_BEGIN ();
    PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
    UINTN                                 Index;
    CHAR8                                 *PdbStr;
    CHAR8                                 AsciiBuffer[512];


    ZeroMem (&ImageContext, sizeof (ImageContext));
    ImageContext.Handle    = Pe32Data;
    ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

    PeCoffLoaderGetImageInfo (&ImageContext);
    
    if (ImageContext.PdbPointer != NULL) {
      //
      // Copy PDB pointer to AsciiBuffer and replace .PDB with .EFI
      //
      PdbStr = ImageContext.PdbPointer;
      for (Index = 0; PdbStr != 0; Index++, PdbStr++) {
        AsciiBuffer[Index] = *PdbStr;
        if (*PdbStr == '.') {
          AsciiBuffer[Index] = '\0';  
        }
      }
      
      DEBUG ((EFI_D_INFO | EFI_D_LOAD, "%a.efi", AsciiBuffer));
    }

  DEBUG_CODE_END ();

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "\n"));

  return EFI_SUCCESS;
}
