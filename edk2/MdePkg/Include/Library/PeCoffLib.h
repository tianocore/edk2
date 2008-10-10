/** @file
  Memory Only PE COFF loader. 

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __BASE_PE_COFF_LIB_H__
#define __BASE_PE_COFF_LIB_H__

#include <IndustryStandard/PeImage.h>
//
// Return status codes from the PE/COFF Loader services
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

///
/// Context structure used while PE/COFF image is being loaded and relocated
///
typedef struct {
  ///
  /// Is set by PeCoffLoaderGetImageInfo() to the ImageBase in the PE/COFF header
  ///
  PHYSICAL_ADDRESS                  ImageAddress;
  ///
  /// Is set by PeCoffLoaderGetImageInfo() to the SizeOfImage in the PE/COFF header.
  /// Image size includes the size of Debug Entry if it is present.
  ///
  UINT64                            ImageSize;
  ///
  /// Is set to zero by PeCoffLoaderGetImageInfo(). If DestinationAddress is non zero,
  /// PeCoffLoaderRelocateImage() will relocate the image using this base address.
  /// If the DestinationAddress is zero, the ImageAddress will be used as the base
  /// address of relocation.
  ///
  PHYSICAL_ADDRESS                  DestinationAddress;
  ///
  /// PeCoffLoaderLoadImage() sets EntryPoint to to the entry point of the PE/COFF image.
  ///
  PHYSICAL_ADDRESS                  EntryPoint;
  ///
  /// Passed in by the caller to PeCoffLoaderGetImageInfo() and PeCoffLoaderLoadImage()
  /// to abstract accessing the image from the library.
  ///
  PE_COFF_LOADER_READ_FILE          ImageRead;
  ///
  /// Used as the FileHandle passed into the ImageRead function when it's called.
  ///
  VOID                              *Handle;
  ///
  /// Caller allocated buffer of size FixupDataSize that can be optionally allocated
  /// prior to calling PeCoffLoaderRelocateImage(). 
  /// This buffer is filled with the information used to fix up the image. 
  /// The fixups have been applied to the image and this entry is just for information.
  ///
  VOID                              *FixupData;
  ///
  /// Is set by PeCoffLoaderGetImageInfo() to the Section Alignment in the PE/COFF header
  /// If the image is a TE image, then this field is set to 0.
  ///
  UINT32                            SectionAlignment;
  ///
  /// Set by PeCoffLoaderGetImageInfo() to offset to the PE/COFF header.
  /// If the PE/COFF image does not start with a DOS header, this value is zero; 
  /// otherwise, it's the offset to the PE/COFF header.
  ///
  UINT32                            PeCoffHeaderOffset;
  ///
  /// Set by PeCoffLoaderGetImageInfo() to the Relative Virtual Address of the debug directory
  /// if it exists in the image
  ///
  UINT32                            DebugDirectoryEntryRva;
  ///
  /// Set by PeCoffLoaderLoadImage() to CodeView area of the PE/COFF Debug directory.
  ///
  VOID                              *CodeView;
  ///
  /// Set by PeCoffLoaderLoadImage() to point to the PDB entry contained in the CodeView area.
  /// The PdbPointer points to the filename of the PDB file used for source-level debug of 
  /// the image by a debugger.
  ///
  CHAR8                             *PdbPointer;
  ///
  /// Is set by PeCoffLoaderGetImageInfo() to the Section Alignment in the PE/COFF header.
  ///
  UINTN                             SizeOfHeaders;
  ///
  /// Not used by this library class. Other library classes that layer on  top of this library
  /// class fill in this value as part of their GetImageInfo call. 
  /// This allows the caller of the library to know what type of memory needs to be allocated
  /// to load and relocate the image.
  ///
  UINT32                            ImageCodeMemoryType;
  ///
  /// Not used by this library class. Other library classes that layer on top of this library 
  /// class fill in this value as part of their GetImageInfo call.
  /// This allows the caller of the library to know what type of memory needs to be allocated
  /// to load and relocate the image
  ///
  UINT32                            ImageDataMemoryType;
  ///
  /// Set by any of the library functions if they encounter an error. 
  ///
  UINT32                            ImageError;
  ///
  /// Set by PeCoffLoaderLoadImage() to indicate the size of FixupData that the caller must
  /// allocate before calling PeCoffLoaderRelocateImage()
  ///
  UINTN                             FixupDataSize;
  ///
  /// Set by PeCoffLoaderGetImageInfo() to the machine type stored in the PE/COFF header
  ///
  UINT16                            Machine;
  ///
  /// Set by PeCoffLoaderGetImageInfo() to the subsystem type stored in the PE/COFF header.
  ///
  UINT16                            ImageType;
  ///
  /// Set by PeCoffLoaderGetImageInfo() to TRUE if the PE/COFF image does not contain
  /// relocation information.
  ///
  BOOLEAN                           RelocationsStripped;
  ///
  /// Set by PeCoffLoaderGetImageInfo() to TRUE if the image is a TE image. 
  /// For a definition of the TE Image format, see the Platform Initialization Pre-EFI
  /// Initialization Core Interface Specification.
  ///
  BOOLEAN                           IsTeImage;
} PE_COFF_LOADER_IMAGE_CONTEXT;

/**
  Retrieves information about a PE/COFF image.

  Computes the PeCoffHeaderOffset, IsTeImage, ImageType, ImageAddress, ImageSize, 
  DestinationAddress, RelocationsStripped, SectionAlignment, SizeOfHeaders, and 
  DebugDirectoryEntryRva fields of the ImageContext structure.  
  If ImageContext is NULL, then return RETURN_INVALID_PARAMETER.  
  If the PE/COFF image accessed through the ImageRead service in the ImageContext 
  structure is not a supported PE/COFF image type, then return RETURN_UNSUPPORTED.  
  If any errors occur while computing the fields of ImageContext, 
  then the error status is returned in the ImageError field of ImageContext.  
  If the image is a TE image, then SectionAlignment is set to 0.

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
  );

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
  );

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
  );


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
  );


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
  );

/**
  Unloads a loaded PE/COFF image from memory and releases its taken resource.
   
  For NT32 emulator, the PE/COFF image loaded by system needs to release.
  For real platform, the PE/COFF image loaded by Core doesn't needs to be unloaded, 
  this function can simply return RETURN_SUCCESS.

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image to be unloaded.

  @retval RETURN_SUCCESS            The PE/COFF image was unloaded successfully.
**/
RETURN_STATUS
EFIAPI
PeCoffLoaderUnloadImage (
  IN PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );
#endif
