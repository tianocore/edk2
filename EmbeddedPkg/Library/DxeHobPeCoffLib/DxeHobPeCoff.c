/** @file
  PE/COFF Loader Library implementation that wraps a protocol passed up from
  SEC/PEI via a HOB. This is done to save space.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>

#include <Protocol/PeCoffLoader.h>


PE_COFF_LOADER_PROTOCOL  *gPeCoffLoader = NULL;


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
RETURN_STATUS
EFIAPI
PeCoffLoaderGetImageInfo (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  return gPeCoffLoader->GetImageInfo (ImageContext);
}


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
RETURN_STATUS
EFIAPI
PeCoffLoaderRelocateImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  return gPeCoffLoader->RelocateImage (ImageContext);
}

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
RETURN_STATUS
EFIAPI
PeCoffLoaderLoadImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  return gPeCoffLoader->LoadImage (ImageContext);
}



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
RETURN_STATUS
EFIAPI
PeCoffLoaderImageReadFromMemory (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
{
  return gPeCoffLoader->ReadFromMemory (
                          FileHandle,
                          FileOffset,
                          ReadSize,
                          Buffer
                          );

}



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
VOID
EFIAPI
PeCoffLoaderRelocateImageForRuntime (
  IN  PHYSICAL_ADDRESS        ImageBase,
  IN  PHYSICAL_ADDRESS        VirtImageBase,
  IN  UINTN                   ImageSize,
  IN  VOID                    *RelocationData
  )
{
  return gPeCoffLoader->RelocateImageForRuntime (
                          ImageBase,
                          VirtImageBase,
                          ImageSize,
                          RelocationData
                          );
}


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
RETURN_STATUS
EFIAPI
PeCoffLoaderUnloadImage (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  return gPeCoffLoader->UnloadImage (ImageContext);
}

typedef struct {
  EFI_HOB_GUID_TYPE             Hob;
  VOID                          *Interface;
} PROTOCOL_HOB;


/**
  The constructor function caches the pointer of DXE Services Table.

  The constructor function caches the pointer of DXE Services Table.
  It will ASSERT() if that operation fails.
  It will ASSERT() if the pointer of DXE Services Table is NULL.
  It will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeHobPeCoffLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  PROTOCOL_HOB   *Hob;

  Hob = GetFirstGuidHob (&gPeCoffLoaderProtocolGuid);
  if (Hob == NULL) {
    return EFI_NOT_FOUND;
  }

  gPeCoffLoader = Hob->Interface;
  return EFI_SUCCESS;
}


