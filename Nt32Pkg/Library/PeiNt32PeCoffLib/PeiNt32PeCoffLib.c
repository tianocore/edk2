/**@file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiNt32PeCoffLib.c

Abstract:

  Wrap the Nt32 PE/COFF loader with the PE COFF LOADER guid structure
  to produce PeCoff library class.


**/

#include <PiPei.h>
#include <Guid/PeiPeCoffLoader.h>
#include <Library/DebugLib.h>
#include <Library/PeCoffLib.h>
#include <Library/HobLib.h>

EFI_PEI_PE_COFF_LOADER_PROTOCOL  *mPeiEfiPeiPeCoffLoader;

/**
  The constructor function caches the pointer of PeCofferLoader guid structure
  into the guid data hob.

  The constructor must be called after PeCofferLoader guid structure is installed.
  It will ASSERT() if PeCofferLoader guid structure is not installed.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiNt32PeCoffLibConstructor (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  EFI_STATUS           Status;
  EFI_HOB_GUID_TYPE    *GuidHob;

  Status = EFI_NOT_FOUND;
  
  //
  // Try to get guid data hob that contains PeCoffLoader guid structure.
  //
  GuidHob = GetFirstGuidHob (&gEfiPeiPeCoffLoaderGuid);

  if (GuidHob == NULL) {
    //
    // GuidHob is not ready, try to locate PeCoffLoader guid structure.
    //
    Status = (*PeiServices)->LocatePpi (
                              PeiServices,
                              &gEfiPeiPeCoffLoaderGuid,
                              0,
                              NULL,
                              (VOID**)&mPeiEfiPeiPeCoffLoader
                              );
    
    //
    // PeCofferLoader guid structure must be installed before this library runs.
    //
    ASSERT_EFI_ERROR (Status);
    
    //
    // Build guid data hob of PeCofferLoader guid structure for DXE module use. 
    //
    BuildGuidDataHob (
      &gEfiPeiPeCoffLoaderGuid,
      (VOID *) &mPeiEfiPeiPeCoffLoader,
      sizeof (VOID *)
      );
  } else {
    //
    // Get PeCofferLoader guid structure directly from guid hob data.
    //
    mPeiEfiPeiPeCoffLoader = (EFI_PEI_PE_COFF_LOADER_PROTOCOL *)(*(UINTN *)(GET_GUID_HOB_DATA (GuidHob)));
  }

  return EFI_SUCCESS;
}

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
{
    return mPeiEfiPeiPeCoffLoader->GetImageInfo (mPeiEfiPeiPeCoffLoader, ImageContext);
}

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
{
  return mPeiEfiPeiPeCoffLoader->RelocateImage (mPeiEfiPeiPeCoffLoader, ImageContext);
}

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
{
  return mPeiEfiPeiPeCoffLoader->LoadImage (mPeiEfiPeiPeCoffLoader, ImageContext);
}

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
{
  return RETURN_UNSUPPORTED;
}


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
{
}

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
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  )
{
  return mPeiEfiPeiPeCoffLoader->UnloadImage (mPeiEfiPeiPeCoffLoader, ImageContext);
}
