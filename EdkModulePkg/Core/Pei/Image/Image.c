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
  DEBUG_CODE (
    EFI_IMAGE_DATA_DIRECTORY * DirectoryEntry;
    EFI_IMAGE_DEBUG_DIRECTORY_ENTRY * DebugEntry;
    UINTN DirCount;
    UINTN Index;
    UINTN Index1;
    BOOLEAN FileNameFound;
    CHAR8 *AsciiString;
    CHAR8 AsciiBuffer[512];
    VOID *CodeViewEntryPointer;
    INTN TEImageAdjust;
    EFI_IMAGE_DOS_HEADER  *DosHeader;
    EFI_IMAGE_NT_HEADERS  *PeHeader;

    //
    // Pe32Data is NULL when load TE image 
    //    
    PeHeader = NULL;
    if (TEImageHeader == NULL) {
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
    }

    //
    // Find the codeview info in the image and display the file name
    // being loaded.
    //
    // Per the PE/COFF spec, you can't assume that a given data directory
    // is present in the image. You have to check the NumberOfRvaAndSizes in
    // the optional header to verify a desired directory entry is there.
    //
    DebugEntry      = NULL;
    DirectoryEntry  = NULL;
    TEImageAdjust   = 0;
    if (TEImageHeader == NULL) {
      if (PeHeader->OptionalHeader.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
        DirectoryEntry = (EFI_IMAGE_DATA_DIRECTORY *) &(PeHeader->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG]);
        DebugEntry = (EFI_IMAGE_DEBUG_DIRECTORY_ENTRY *) ((UINTN) ImageAddress + DirectoryEntry->VirtualAddress);
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
      for (DirCount = 0; DirCount < DirectoryEntry->Size; DirCount++, DebugEntry++) {
        if (DebugEntry->Type == EFI_IMAGE_DEBUG_TYPE_CODEVIEW) {
          if (DebugEntry->SizeOfData > 0) {
            CodeViewEntryPointer = (VOID *) ((UINTN) DebugEntry->RVA + (UINTN) ImageAddress + (UINTN)TEImageAdjust);
            switch (* (UINT32 *) CodeViewEntryPointer) {
              case CODEVIEW_SIGNATURE_NB10:
                AsciiString = (CHAR8 *) CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_NB10_ENTRY);
                break;

              case CODEVIEW_SIGNATURE_RSDS:
                AsciiString = (CHAR8 *) CodeViewEntryPointer + sizeof (EFI_IMAGE_DEBUG_CODEVIEW_RSDS_ENTRY);
                break;

              default:
                AsciiString = NULL;
                break;
            }
            if (AsciiString != NULL) {
              FileNameFound = FALSE;
              for (Index = 0, Index1 = 0; AsciiString[Index] != 0; Index++) {
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
  );

  DEBUG ((EFI_D_INFO | EFI_D_LOAD, "\n"));

  return EFI_SUCCESS;
}
