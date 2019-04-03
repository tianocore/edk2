/** @file

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2010, Apple Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PE_COFF_LOADER_H__
#define __PE_COFF_LOADER_H__

// Needed for PE_COFF_LOADER_IMAGE_CONTEXT
#include <Library/PeCoffLib.h>

// B323179B-97FB-477E-B0FE-D88591FA11AB
#define PE_COFF_LOADER_PROTOCOL_GUID \
  { 0xB323179B, 0x97FB, 0x477E, { 0xB0, 0xFE, 0xD8, 0x85, 0x91, 0xFA, 0x11, 0xAB } }


typedef struct _PE_COFF_LOADER_PROTOCOL PE_COFF_LOADER_PROTOCOL;



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
  The ImageRead and Handle fields of ImageContext structure must be valid prior
  to invoking this service.

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image that needs to be examined by this function.

  @retval RETURN_SUCCESS            The information on the PE/COFF image was collected.
  @retval RETURN_INVALID_PARAMETER  ImageContext is NULL.
  @retval RETURN_UNSUPPORTED        The PE/COFF image is not supported.

**/
typedef
RETURN_STATUS
(EFIAPI *PE_COFF_LOADER_GET_IMAGE_INFO) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );


/**
  Applies relocation fixups to a PE/COFF image that was loaded with PeCoffLoaderLoadImage().

  If the DestinationAddress field of ImageContext is 0, then use the ImageAddress field of
  ImageContext as the relocation base address.  Otherwise, use the DestinationAddress field
  of ImageContext as the relocation base address.  The caller must allocate the relocation
  fixup log buffer and fill in the FixupData field of ImageContext prior to calling this function.

  The ImageRead, Handle, PeCoffHeaderOffset, IsTeImage, Machine, ImageType, ImageAddress,
  ImageSize, DestinationAddress, RelocationsStripped, SectionAlignment, SizeOfHeaders,
  DebugDirectoryEntryRva, EntryPoint, FixupDataSize, CodeView, PdbPointer, and FixupData of
  the ImageContext structure must be valid prior to invoking this service.

  If ImageContext is NULL, then ASSERT().

  Note that if the platform does not maintain coherency between the instruction cache(s) and the data
  cache(s) in hardware, then the caller is responsible for performing cache maintenance operations
  prior to transferring control to a PE/COFF image that is loaded using this library.

  @param  ImageContext        Pointer to the image context structure that describes the PE/COFF
                              image that is being relocated.

  @retval RETURN_SUCCESS      The PE/COFF image was relocated.
                              Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_LOAD_ERROR   The image in not a valid PE/COFF image.
                              Extended status information is in the ImageError field of ImageContext.
  @retval RETURN_UNSUPPORTED  A relocation record type is not supported.
                              Extended status information is in the ImageError field of ImageContext.

**/
typedef
RETURN_STATUS
(EFIAPI *PE_COFF_LOADER_RELOCATE_IMAGE) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );


/**
  Loads a PE/COFF image into memory.

  Loads the PE/COFF image accessed through the ImageRead service of ImageContext into the buffer
  specified by the ImageAddress and ImageSize fields of ImageContext.  The caller must allocate
  the load buffer and fill in the ImageAddress and ImageSize fields prior to calling this function.
  The EntryPoint, FixupDataSize, CodeView, PdbPointer and HiiResourceData fields of ImageContext are computed.
  The ImageRead, Handle, PeCoffHeaderOffset, IsTeImage, Machine, ImageType, ImageAddress, ImageSize,
  DestinationAddress, RelocationsStripped, SectionAlignment, SizeOfHeaders, and DebugDirectoryEntryRva
  fields of the ImageContext structure must be valid prior to invoking this service.

  If ImageContext is NULL, then ASSERT().

  Note that if the platform does not maintain coherency between the instruction cache(s) and the data
  cache(s) in hardware, then the caller is responsible for performing cache maintenance operations
  prior to transferring control to a PE/COFF image that is loaded using this library.

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
typedef
RETURN_STATUS
(EFIAPI *PE_COFF_LOADER_LOAD_IMAGE) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );



/**
  Reads contents of a PE/COFF image from a buffer in system memory.

  This is the default implementation of a PE_COFF_LOADER_READ_FILE function
  that assumes FileHandle pointer to the beginning of a PE/COFF image.
  This function reads contents of the PE/COFF image that starts at the system memory
  address specified by FileHandle. The read operation copies ReadSize bytes from the
  PE/COFF image starting at byte offset FileOffset into the buffer specified by Buffer.
  The size of the buffer actually read is returned in ReadSize.

  If FileHandle is NULL, then ASSERT().
  If ReadSize is NULL, then ASSERT().
  If Buffer is NULL, then ASSERT().

  @param  FileHandle        Pointer to base of the input stream
  @param  FileOffset        Offset into the PE/COFF image to begin the read operation.
  @param  ReadSize          On input, the size in bytes of the requested read operation.
                            On output, the number of bytes actually read.
  @param  Buffer            Output buffer that contains the data read from the PE/COFF image.

  @retval RETURN_SUCCESS    Data is read from FileOffset from the Handle into
                            the buffer.
**/
typedef
RETURN_STATUS
(EFIAPI *PE_COFF_LOADER_READ_FROM_MEMORY) (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  );



/**
  Reapply fixups on a fixed up PE32/PE32+ image to allow virutal calling at EFI
  runtime.

  This function reapplies relocation fixups to the PE/COFF image specified by ImageBase
  and ImageSize so the image will execute correctly when the PE/COFF image is mapped
  to the address specified by VirtualImageBase. RelocationData must be identical
  to the FiuxupData buffer from the PE_COFF_LOADER_IMAGE_CONTEXT structure
  after this PE/COFF image was relocated with PeCoffLoaderRelocateImage().

  Note that if the platform does not maintain coherency between the instruction cache(s) and the data
  cache(s) in hardware, then the caller is responsible for performing cache maintenance operations
  prior to transferring control to a PE/COFF image that is loaded using this library.

  @param  ImageBase          Base address of a PE/COFF image that has been loaded
                             and relocated into system memory.
  @param  VirtImageBase      The request virtual address that the PE/COFF image is to
                             be fixed up for.
  @param  ImageSize          The size, in bytes, of the PE/COFF image.
  @param  RelocationData     A pointer to the relocation data that was collected when the PE/COFF
                             image was relocated using PeCoffLoaderRelocateImage().

**/
typedef
VOID
(EFIAPI *PE_COFF_LOADER_RELOCATE_IMAGE_FOR_RUNTIME) (
  IN  PHYSICAL_ADDRESS        ImageBase,
  IN  PHYSICAL_ADDRESS        VirtImageBase,
  IN  UINTN                   ImageSize,
  IN  VOID                    *RelocationData
  );



/**
  Unloads a loaded PE/COFF image from memory and releases its taken resource.
  Releases any environment specific resources that were allocated when the image
  specified by ImageContext was loaded using PeCoffLoaderLoadImage().

  For NT32 emulator, the PE/COFF image loaded by system needs to release.
  For real platform, the PE/COFF image loaded by Core doesn't needs to be unloaded,
  this function can simply return RETURN_SUCCESS.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image to be unloaded.

  @retval RETURN_SUCCESS            The PE/COFF image was unloaded successfully.
**/
typedef
RETURN_STATUS
(EFIAPI *PE_COFF_LOADER_UNLOAD_IMAGE) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );


struct _PE_COFF_LOADER_PROTOCOL {
  PE_COFF_LOADER_GET_IMAGE_INFO             GetImageInfo;
  PE_COFF_LOADER_LOAD_IMAGE                 LoadImage;
  PE_COFF_LOADER_RELOCATE_IMAGE             RelocateImage;
  PE_COFF_LOADER_READ_FROM_MEMORY           ReadFromMemory;
  PE_COFF_LOADER_RELOCATE_IMAGE_FOR_RUNTIME RelocateImageForRuntime;
  PE_COFF_LOADER_UNLOAD_IMAGE               UnloadImage;
};


extern EFI_GUID gPeCoffLoaderProtocolGuid;


#endif

