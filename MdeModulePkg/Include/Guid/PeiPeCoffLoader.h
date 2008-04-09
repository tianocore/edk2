/** @file
  
  GUID for the PE/COFF Loader APIs shared between SEC, PEI and DXE

Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.               

**/

#ifndef __PEI_PE_COFF_LOADER_H__
#define __PEI_PE_COFF_LOADER_H__

//
// MdePkg/Include/Common/PeCoffLoaderImageContext.h
//
#include <Library/PeCoffLib.h>

#define EFI_PEI_PE_COFF_LOADER_GUID  \
  { 0xd8117cff, 0x94a6, 0x11d4, {0x9a, 0x3a, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

typedef struct _EFI_PEI_PE_COFF_LOADER_PROTOCOL   EFI_PEI_PE_COFF_LOADER_PROTOCOL;

/**
  Retrieves information about a PE/COFF image.

  Computes the PeCoffHeaderOffset, ImageAddress, ImageSize, DestinationAddress, CodeView,
  PdbPointer, RelocationsStripped, SectionAlignment, SizeOfHeaders, and DebugDirectoryEntryRva
  fields of the ImageContext structure.  If ImageContext is NULL, then return RETURN_INVALID_PARAMETER.
  If the PE/COFF image accessed through the ImageRead service in the ImageContext structure is not
  a supported PE/COFF image type, then return RETURN_UNSUPPORTED.  If any errors occur while
  computing the fields of ImageContext, then the error status is returned in the ImageError field of
  ImageContext. 

  @param  This                      Pointer to the EFI_PEI_PE_COFF_LOADER_PROTOCOL instance.
  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image that needs to be examined by this function.

  @retval RETURN_SUCCESS            The information on the PE/COFF image was collected.
  @retval RETURN_INVALID_PARAMETER  ImageContext is NULL.
  @retval RETURN_UNSUPPORTED        The PE/COFF image is not supported.

**/
typedef 
RETURN_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_GET_IMAGE_INFO) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

/**
  Loads a PE/COFF image into memory.

  Loads the PE/COFF image accessed through the ImageRead service of ImageContext into the buffer
  specified by the ImageAddress and ImageSize fields of ImageContext.  The caller must allocate
  the load buffer and fill in the ImageAddress and ImageSize fields prior to calling this function.
  The EntryPoint, FixupDataSize, CodeView, and PdbPointer fields of ImageContext are computed.
  If ImageContext is NULL, then ASSERT().

  @param  This                      Pointer to the EFI_PEI_PE_COFF_LOADER_PROTOCOL instance.
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
(EFIAPI *EFI_PEI_PE_COFF_LOADER_LOAD_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

/**
  Applies relocation fixups to a PE/COFF image that was loaded with PeCoffLoaderLoadImage().

  If the DestinationAddress field of ImageContext is 0, then use the ImageAddress field of
  ImageContext as the relocation base address.  Otherwise, use the DestinationAddress field
  of ImageContext as the relocation base address.  The caller must allocate the relocation
  fixup log buffer and fill in the FixupData field of ImageContext prior to calling this function.  
  If ImageContext is NULL, then ASSERT().

  @param  This                Pointer to the EFI_PEI_PE_COFF_LOADER_PROTOCOL instance.
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
(EFIAPI *EFI_PEI_PE_COFF_LOADER_RELOCATE_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

/**
  Unloads a loaded PE/COFF image from memory and releases its taken resource.
   
  For NT32 emulator, the PE/COFF image loaded by system needs to release.
  For real platform, the PE/COFF image loaded by Core doesn't needs to be unloaded, 
  this function can simply return RETURN_SUCCESS.

  @param  This                      Pointer to the EFI_PEI_PE_COFF_LOADER_PROTOCOL instance.
  @param  ImageContext              Pointer to the image context structure that describes the PE/COFF
                                    image to be unloaded.

  @retval RETURN_SUCCESS            The PE/COFF image was unloaded successfully.
**/
typedef 
RETURN_STATUS
(EFIAPI *EFI_PEI_PE_COFF_LOADER_UNLOAD_IMAGE) (
  IN EFI_PEI_PE_COFF_LOADER_PROTOCOL          *This,
  IN PE_COFF_LOADER_IMAGE_CONTEXT         *ImageContext
  );

struct _EFI_PEI_PE_COFF_LOADER_PROTOCOL {
  EFI_PEI_PE_COFF_LOADER_GET_IMAGE_INFO  GetImageInfo;
  EFI_PEI_PE_COFF_LOADER_LOAD_IMAGE      LoadImage;
  EFI_PEI_PE_COFF_LOADER_RELOCATE_IMAGE  RelocateImage;
  EFI_PEI_PE_COFF_LOADER_UNLOAD_IMAGE    UnloadImage;
};

extern EFI_GUID gEfiPeiPeCoffLoaderGuid;

#endif
