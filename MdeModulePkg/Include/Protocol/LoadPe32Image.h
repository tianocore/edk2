/** @file

  Load Pe32 Image protocol provides capability to load and unload EFI image into memory and execute it.
  This protocol bases on File Device Path to get EFI image.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LOAD_PE32_IMAGE_H__
#define __LOAD_PE32_IMAGE_H__

#define PE32_IMAGE_PROTOCOL_GUID  \
  {0x5cb5c776,0x60d5,0x45ee,{0x88,0x3c,0x45,0x27,0x8,0xcd,0x74,0x3f }}

#define EFI_LOAD_PE_IMAGE_ATTRIBUTE_NONE                                 0x00
#define EFI_LOAD_PE_IMAGE_ATTRIBUTE_RUNTIME_REGISTRATION                 0x01
#define EFI_LOAD_PE_IMAGE_ATTRIBUTE_DEBUG_IMAGE_INFO_TABLE_REGISTRATION  0x02

typedef struct _EFI_PE32_IMAGE_PROTOCOL   EFI_PE32_IMAGE_PROTOCOL;

/**

  Loads an EFI image into memory and returns a handle to the image with extended parameters.

  @param  This                Pointer to the LoadPe32Image protocol instance
  @param  ParentImageHandle   The caller's image handle.
  @param  FilePath            The specific file path from which the image is loaded.
  @param  SourceBuffer        If not NULL, a pointer to the memory location containing a copy of
                              the image to be loaded.
  @param  SourceSize          The size in bytes of SourceBuffer.
  @param  DstBuffer           The buffer to store the image.
  @param  NumberOfPages       For input, specifies the space size of the image by caller if not NULL.
                              For output, specifies the actual space size needed.
  @param  ImageHandle         Image handle for output.
  @param  EntryPoint          Image entry point for output.
  @param  Attribute           The bit mask of attributes to set for the load PE image.

  @retval EFI_SUCCESS           The image was loaded into memory.
  @retval EFI_NOT_FOUND         The FilePath was not found.
  @retval EFI_INVALID_PARAMETER One of the parameters has an invalid value.
  @retval EFI_UNSUPPORTED       The image type is not supported, or the device path cannot be
                                parsed to locate the proper protocol for loading the file.
  @retval EFI_OUT_OF_RESOURCES  Image was not loaded due to insufficient memory resources.
**/
typedef
EFI_STATUS
(EFIAPI *LOAD_PE_IMAGE)(
  IN EFI_PE32_IMAGE_PROTOCOL           *This,
  IN  EFI_HANDLE                       ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN  VOID                             *SourceBuffer       OPTIONAL,
  IN  UINTN                            SourceSize,
  IN  EFI_PHYSICAL_ADDRESS             DstBuffer           OPTIONAL,
  OUT UINTN                            *NumberOfPages      OPTIONAL,
  OUT EFI_HANDLE                       *ImageHandle,
  OUT EFI_PHYSICAL_ADDRESS             *EntryPoint         OPTIONAL,
  IN  UINT32                           Attribute
  );

/**

  Unload the specified image.

  @param  This             Pointer to the LoadPe32Image protocol instance
  @param  ImageHandle      The specified image handle to be unloaded.

  @retval EFI_INVALID_PARAMETER Image handle is NULL.
  @retval EFI_UNSUPPORTED       Attempt to unload an unsupported image.
  @retval EFI_SUCCESS           Image is successfully unloaded.

--*/
typedef
EFI_STATUS
(EFIAPI *UNLOAD_PE_IMAGE)(
  IN EFI_PE32_IMAGE_PROTOCOL          *This,
  IN EFI_HANDLE                       ImageHandle
  );

struct _EFI_PE32_IMAGE_PROTOCOL {
  LOAD_PE_IMAGE     LoadPeImage;
  UNLOAD_PE_IMAGE   UnLoadPeImage;
};

extern EFI_GUID gEfiLoadPeImageProtocolGuid;

#endif

