/*++

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  GetImage.c

Abstract:

  Image data extraction support for common use.

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"
#include "EfiImageFormat.h"

#include EFI_PROTOCOL_CONSUMER (LoadedImage)

EFI_STATUS
GetImageFromFv (
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  IN  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv,
#else
  IN  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv,
#endif
  IN  EFI_GUID           *NameGuid,
  IN  EFI_SECTION_TYPE   SectionType,
  OUT VOID               **Buffer,
  OUT UINTN              *Size
  )
{
  EFI_STATUS                Status;
  EFI_FV_FILETYPE           FileType;
  EFI_FV_FILE_ATTRIBUTES    Attributes;
  UINT32                    AuthenticationStatus;

  //
  // Read desired section content in NameGuid file
  //
  *Buffer     = NULL;
  *Size       = 0;
  Status      = Fv->ReadSection (
                      Fv,
                      NameGuid,
                      SectionType,
                      0,
                      Buffer,
                      Size,
                      &AuthenticationStatus
                      );

  if (EFI_ERROR (Status) && (SectionType == EFI_SECTION_TE)) {
    //
    // Try reading PE32 section, since the TE section does not exist
    //
    *Buffer = NULL;
    *Size   = 0;
    Status  = Fv->ReadSection (
                    Fv,
                    NameGuid,
                    EFI_SECTION_PE32,
                    0,
                    Buffer,
                    Size,
                    &AuthenticationStatus
                    );
  }

  if (EFI_ERROR (Status) && 
      ((SectionType == EFI_SECTION_TE) || (SectionType == EFI_SECTION_PE32))) {
    //
    // Try reading raw file, since the desired section does not exist
    //
    *Buffer = NULL;
    *Size   = 0;
    Status  = Fv->ReadFile (
                    Fv,
                    NameGuid,
                    Buffer,
                    Size,
                    &FileType,
                    &Attributes,
                    &AuthenticationStatus
                    );
  }

  return Status;
}

EFI_STATUS
GetImage (
  IN  EFI_GUID           *NameGuid,
  IN  EFI_SECTION_TYPE   SectionType,
  OUT VOID               **Buffer,
  OUT UINTN              *Size
  )
{
  return GetImageEx (NULL, NameGuid, SectionType, Buffer, Size, FALSE);
}

EFI_STATUS
GetImageEx (
  IN  EFI_HANDLE         ImageHandle,
  IN  EFI_GUID           *NameGuid,
  IN  EFI_SECTION_TYPE   SectionType,
  OUT VOID               **Buffer,
  OUT UINTN              *Size,
  BOOLEAN                WithinImageFv
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;
  UINTN                         Index;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
#if (PI_SPECIFICATION_VERSION < 0x00010000)
  EFI_FIRMWARE_VOLUME_PROTOCOL  *ImageFv;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *Fv;
#else
  EFI_FIRMWARE_VOLUME2_PROTOCOL *ImageFv;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
#endif

  if (ImageHandle == NULL && WithinImageFv) {
    return EFI_INVALID_PARAMETER;
  }

  Status  = EFI_NOT_FOUND;
  ImageFv = NULL;
  if (ImageHandle != NULL) {
    Status = gBS->HandleProtocol (
               ImageHandle,
               &gEfiLoadedImageProtocolGuid,
               (VOID **) &LoadedImage
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                  #if (PI_SPECIFICATION_VERSION < 0x00010000)    
                    &gEfiFirmwareVolumeProtocolGuid,
                  #else
                    &gEfiFirmwareVolume2ProtocolGuid,
                  #endif
                    (VOID **) &ImageFv
                    );
    if (!EFI_ERROR (Status)) {
      Status = GetImageFromFv (ImageFv, NameGuid, SectionType, Buffer, Size);
    }
  }

  if (Status == EFI_SUCCESS || WithinImageFv) {
    return Status;
  }

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                #if (PI_SPECIFICATION_VERSION < 0x00010000)
                  &gEfiFirmwareVolumeProtocolGuid,
                #else
                  &gEfiFirmwareVolume2ProtocolGuid,
                #endif
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find desired image in all Fvs
  //
  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                  #if (PI_SPECIFICATION_VERSION < 0x00010000)
                    &gEfiFirmwareVolumeProtocolGuid,
                  #else
                    &gEfiFirmwareVolume2ProtocolGuid,
                  #endif
                    (VOID**)&Fv
                    );

    if (EFI_ERROR (Status)) {
      gBS->FreePool(HandleBuffer);
      return Status;
    }

    if (ImageFv != NULL && Fv == ImageFv) {
      continue;
    }

    Status = GetImageFromFv (Fv, NameGuid, SectionType, Buffer, Size);

    if (!EFI_ERROR (Status)) {
      break;
    }
  }
  gBS->FreePool(HandleBuffer);

  //
  // Not found image
  //
  if (Index == HandleCount) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

