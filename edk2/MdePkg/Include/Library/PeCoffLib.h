/** @file
  Memory Only PE COFF loader. 

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  PeCoffLib.h

**/

#ifndef __BASE_PE_COFF_LIB_H__
#define __BASE_PE_COFF_LIB_H__

//
// Return status codes from the PE/COFF Loader services
// BUGBUG: Find where used and see if can be replaced by RETURN_STATUS codes
//
#define IMAGE_ERROR_SUCCESS                      0
#define IMAGE_ERROR_IMAGE_READ                   1  
#define IMAGE_ERROR_INVALID_PE_HEADER_SIGNATURE  2
#define IMAGE_ERROR_INVALID_MACHINE_TYPE         3
#define IMAGE_ERROR_INVALID_SUBSYSTEM            4
#define IMAGE_ERROR_INVALID_IMAGE_ADDRESS        5
#define IMAGE_ERROR_INVALID_IMAGE_SIZE           6
#define IMAGE_ERROR_INVALID_SECTION_ALIGNMENT    7
#define IMAGE_ERROR_SECTION_NOT_LOADED           8
#define IMAGE_ERROR_FAILED_RELOCATION            9
#define IMAGE_ERROR_FAILED_ICACHE_FLUSH          10


//
// PE/COFF Loader Read Function passed in by caller
//
typedef
RETURN_STATUS
(EFIAPI *PE_COFF_LOADER_READ_FILE) (
  IN     VOID   *FileHandle,
  IN     UINTN  FileOffset,
  IN OUT UINTN  *ReadSize,
  OUT    VOID   *Buffer
  );

//
// Context structure used while PE/COFF image is being loaded and relocated
//
typedef struct {
  PHYSICAL_ADDRESS                  ImageAddress;
  UINT64                            ImageSize;
  PHYSICAL_ADDRESS                  DestinationAddress;
  PHYSICAL_ADDRESS                  EntryPoint;
  PE_COFF_LOADER_READ_FILE          ImageRead;
  VOID                              *Handle;
  VOID                              *FixupData;
  UINT32                            SectionAlignment;
  UINT32                            PeCoffHeaderOffset;
  UINT32                            DebugDirectoryEntryRva;
  VOID                              *CodeView;
  CHAR8                             *PdbPointer;
  UINTN                             SizeOfHeaders;
  UINT32                            ImageCodeMemoryType;
  UINT32                            ImageDataMemoryType;
  UINT32                            ImageError;
  UINTN                             FixupDataSize;
  UINT16                            Machine;
  UINT16                            ImageType;
  BOOLEAN                           RelocationsStripped;
  BOOLEAN                           IsTeImage;
} PE_COFF_LOADER_IMAGE_CONTEXT;


/**
  Retrieves information about a PE/COFF image.

  Computes the PeCoffHeaderOffset, ImageAddress, ImageSize, DestinationAddress, CodeView,
  PdbPointer, RelocationsStripped, SectionAlignment, SizeOfHeaders, and DebugDirectoryEntryRva
  fields of the ImageContext structure.  If ImageContext is NULL, then return RETURN_INVALID_PARAMETER.
  If the PE/COFF image accessed through the ImageRead service in the ImageContext structure is not
  a supported PE/COFF image type, then return RETURN_UNSUPPORTED.  If any errors occur while
  computing the fields of ImageContext, then the error status is returned in the ImageError field of
  ImageContext. 

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image that needs to be examined by this function.

  @retval RETURN_SUCCESS            The information on the PE/COFF image was collected.
  @retval RETURN_INVALID_PARAMETER  ImageContext is NULL.
  @retval RETURN_UNSUPPORTED        The PE/COFF image is not supported.

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderGetImageInfo (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;

/**
  Applies relocation fixups to a PE/COFF image that was loaded with PeCoffLoaderLoadImage().

  If the DestinationAddress field of ImageContext is 0, then use the ImageAddress field of
  ImageContext as the relocation base address.  Otherwise, use the DestinationAddress field
  of ImageContext as the relocation base address.  The caller must allocate the relocation
  fixup log buffer and fill in the FixupData field of ImageContext prior to calling this function.  
  If ImageContext is NULL, then ASSERT().

  @param  ImageContext        Pointer to the image context structure that describes the PE/COFF
                              image that is being relocated.

  @retval RETURN_SUCCESS      The PE/COFF image was relocated.
                              Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_LOAD_ERROR   The image in not a valid PE/COFF image.
                              Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_UNSUPPORTED  A relocation record type is not supported.
                              Extended status information is in the ImageError field of ImageContext.

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderRelocateImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;

/**
  Loads a PE/COFF image into memory.

  Loads the PE/COFF image accessed through the ImageRead service of ImageContext into the buffer
  specified by the ImageAddress and ImageSize fields of ImageContext.  The caller must allocate
  the load buffer and fill in the ImageAddress and ImageSize fields prior to calling this function.
  The EntryPoint, FixupDataSize, CodeView, and PdbPointer fields of ImageContext are computed.
  If ImageContext is NULL, then ASSERT().

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image that is being loaded.

  @retval RETURN_SUCCESS            The PE/COFF image was loaded into the buffer specified by
                                    the ImageAddress and ImageSize fields of ImageContext.
                                    Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_BUFFER_TOO_SMALL   The caller did not provide a large enough buffer.
                                    Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_LOAD_ERROR         The PE/COFF image is an EFI Runtime image with no relocations.
                                    Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_INVALID_PARAMETER  The image address is invalid.
                                    Extended status information is in the ImageError field of ImageContext.

**/
RETURN_STATUS
EFIAPI
PeCoffLoaderLoadImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
;


/**
  ImageRead function that operates on a memory buffer whos base is passed into
  FileHandle. 

  @param  FileHandle        Ponter to baes of the input stream
  @param  FileOffset        Offset to the start of the buffer
  @param  ReadSize          Number of bytes to copy into the buffer
  @param  Buffer            Location to place results of read

  @retval RETURN_SUCCESS    Data is read from FileOffset from the Handle into 
                            the buffer.
**/
RETURN_STATUS
EFIAPI
PeCoffLoaderImageReadFromMemory (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
;


/**
  Reapply fixups on a fixed up PE32/PE32+ image to allow virutal calling at EFI
  runtime. 
  
  PE_COFF_LOADER_IMAGE_CONTEXT.FixupData stores information needed to reapply
  the fixups with a virtual mapping.


  @param  ImageBase          Base address of relocated image
  @param  VirtImageBase      Virtual mapping for ImageBase
  @param  ImageSize          Size of the image to relocate
  @param  RelocationData     Location to place results of read
  
**/
VOID
EFIAPI
PeCoffLoaderRelocateImageForRuntime (
  IN  PHYSICAL_ADDRESS        ImageBase,
  IN  PHYSICAL_ADDRESS        VirtImageBase,
  IN  UINTN                   ImageSize,
  IN  VOID                    *RelocationData
  )
;


#endif
