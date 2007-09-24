/** @file
  Mde PI library functions.

  Copyright (c) 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PiLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>


/**
  Internal function which read the image specified by Firmware File GUID name and 
  the Firmware Section tyep from a specified Firmware Volume 


  @param  Fv                      The Firmware Volume Protocol instance.
  @param  NameGuid             The GUID name of a Firmware File.
  @param  SectionType         The Firmware Section type.
  @param  Buffer                  On output, Buffer contains the the data read from the section in the Firmware File found.
  @param  Size                    On output, the size of Buffer.

  @retval  EFI_SUCCESS	      The image is found and data and size is returned.
  @retval  EFI_NOT_FOUND	    The image specified by NameGuid and SectionType can't be found.
  @retval  EFI_OUT_OF_RESOURCES	There were not enough resources to allocate the output data buffer or complete the operations.
  @retval  EFI_DEVICE_ERROR	A hardware error occurs during reading from the Firmware Volume.
  @retval  EFI_ACCESS_DENIED	The firmware volume containing the searched Firmware File is configured to disallow reads.

**/
STATIC
EFI_STATUS
InternalGetImageFromFv (
  IN         EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv,
  IN  CONST  EFI_GUID           *NameGuid,
  IN         EFI_SECTION_TYPE   SectionType,
  OUT        VOID               **Buffer,
  OUT        UINTN              *Size
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

/**
  Allocate and fill a buffer with an image identified by a Firmware File GUID name and a Firmware Section type. 
  The Firmware Volumes to search for the Firmware File can be specified to be either all Firmware Volumes 
  in the system, or the Firmware Volume which contains the Firmware File specified by an image handle.

  If ImageHandle is NULL, all Firmware Volumes in the system will be searched. If ImageHandle is not NULL, 
  ImageHandle is interpreted as EFI_PEI_FILE_HANDLE for the implementation of this function for PEI phase. 
  The input parameter ImageHandle is interpreted as EFI_HANDLE, on which an EFI_LOADED_IMAGE_PROTOCOL 
  is installed, for the implementation of this function for DXE phase. The search always starts from the FV 
  identified by ImageHandle. If WithinImageFv is TRUE, search will only be performed on the first FV. If WithinImageFv 
  is FALSE, search will continue on other FVs if it fails on the first FV. The search order of Firmware Volumes is 
  deterministic but arbitrary if no new firmware volume is added into the system between each search. 
  
  The search order for the section type specified by SectionType in the Firmware File is using a depth-first 
  and left-to-right algorithm through all sections. The first section found to match SectionType will be returned. 
  
  If SectionType is EFI_SECTION_PE32, EFI_SECTION_PE32 will be used as Firmware Section type 
  to read Firmware Section data from the Firmware File. If no such section exists, the function will try 
  to read a Firmware File named with NameGuid. If no such file exists, EFI_NOT_FOUND is returned.
  
  If SectionType is EFI_SECTION_TE, EFI_SECTION_TE will be used as Firmware Section type to read Firmware Section 
  data from the Firmware File. If no such section exists, EFI_SECTION_PE32 will be used as Firmware Section type to 
  read Firmware Section data from the Firmware File. If no such section exists, the function will try to read a Firmware 
  File named with NameGuid. If no such file exists, EFI_NOT_FOUND is returned.
  
  The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
  by this function. This function can only be called at TPL_NOTIFY and below.
  
  If ImageHandle is NULL and WithinImage is TRUE, then ASSERT ();
  If NameGuid is NULL, then ASSERT();
  If Buffer is NULL, then ASSERT();
  If Size is NULL, then ASSERT().

  @param  NameGuid             The GUID name of a Firmware File.
  @param  SectionType         The Firmware Section type.
  @param  Buffer                  On output, Buffer contains the the data read from the section in the Firmware File found.
  @param  Size                    On output, the size of Buffer.

  @retval  EFI_SUCCESS	      The image is found and data and size is returned.
  @retval  EFI_NOT_FOUND	    The image specified by NameGuid and SectionType can't be found.
  @retval  EFI_OUT_OF_RESOURCES	There were not enough resources to allocate the output data buffer or complete the operations.
  @retval  EFI_DEVICE_ERROR	A hardware error occurs during reading from the Firmware Volume.
  @retval  EFI_ACCESS_DENIED	The firmware volume containing the searched Firmware File is configured to disallow reads.

**/

EFI_STATUS
EFIAPI
GetSectionFromFvFile (
  IN CONST  VOID               *ImageHandle,
  IN CONST  EFI_GUID           *NameGuid,
  IN        EFI_SECTION_TYPE   SectionType,
  OUT       VOID               **Buffer,
  OUT       UINTN              *Size,
  IN        BOOLEAN            WithinImageFv
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;
  UINTN                         Index;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *ImageFv;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;

  ASSERT (NameGuid != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (Size != NULL);
  ASSERT (!(ImageHandle == NULL && WithinImageFv));

  Status  = EFI_NOT_FOUND;
  ImageFv = NULL;
  if (ImageHandle != NULL) {
    Status = gBS->HandleProtocol (
               (EFI_HANDLE *) ImageHandle,
               &gEfiLoadedImageProtocolGuid,
               (VOID **) &LoadedImage
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = gBS->HandleProtocol (
                    LoadedImage->DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &ImageFv
                    );
    if (!EFI_ERROR (Status)) {
      Status = InternalGetImageFromFv (ImageFv, NameGuid, SectionType, Buffer, Size);
    }
  }

  if (Status == EFI_SUCCESS || WithinImageFv) {
    return Status;
  }

  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Find desired image in all Fvs
  //
  for (Index = 0; Index < HandleCount; ++Index) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID**)&Fv
                    );

    if (EFI_ERROR (Status)) {
      goto Done;
    }

    if (ImageFv != NULL && Fv == ImageFv) {
      continue;
    }

    Status = InternalGetImageFromFv (Fv, NameGuid, SectionType, Buffer, Size);

    if (!EFI_ERROR (Status)) {
      goto Done;
    }
  }

  //
  // Not found image
  //
  if (Index == HandleCount) {
    Status = EFI_NOT_FOUND;
  }

Done:

  if (HandleBuffer != NULL) {  
    FreePool(HandleBuffer);
  }

  return Status;
}

EFI_HANDLE
EFIAPI
ImageHandleToFvHandle (
  EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS                    Status;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  
  ASSERT (ImageHandle != NULL);

  Status = gBS->HandleProtocol (
             (EFI_HANDLE *) ImageHandle,
             &gEfiLoadedImageProtocolGuid,
             (VOID **) &LoadedImage
             );

  ASSERT_EFI_ERROR (Status);

  return LoadedImage->DeviceHandle;

}

EFI_STATUS
EFIAPI
GetSectionFromAnyFv (
  IN CONST  EFI_GUID           *NameGuid,
  IN        EFI_SECTION_TYPE   SectionType,
  IN        UINTN              Instance,
  OUT       VOID               **Buffer,
  OUT       UINTN              *Size
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;
  UINTN                         Index;
  EFI_HANDLE                    FvHandle;
  EFI_TPL                       OldTpl;

  //
  // Search the FV that contain the caller's FFS first.
  // FV builder can choose to build FFS into the this FV
  // so that this implementation of GetSectionFromAnyFv
  // will locate the FFS faster.
  //
  FvHandle = ImageHandleToFvHandle (gImageHandle);
  Status = GetSectionFromFv (
             FvHandle,
             NameGuid,
             SectionType,
             Instance,
             Buffer,
             Size
             );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  for (Index = 0; Index < HandleCount; ++Index) {
    //
    // Skip the FV that contain the caller's FFS
    //
    if (HandleBuffer[Index] == FvHandle) {
      continue;
    }

    Status = GetSectionFromFv (
               HandleBuffer[Index], 
               NameGuid, 
               SectionType, 
               Instance,
               Buffer, 
               Size
               );

    if (!EFI_ERROR (Status)) {
      goto Done;
    }
  }

  if (Index == HandleCount) {
    Status = EFI_NOT_FOUND;
  }

Done:
  
  gBS->RestoreTPL (OldTpl);
  
  if (HandleBuffer != NULL) {  
    FreePool(HandleBuffer);
  }
  return Status;
  
}

EFI_STATUS
EFIAPI
GetSectionFromFv (
  IN  EFI_HANDLE                    FvHandle,
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         Instance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
{
  EFI_STATUS                    Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  UINT32                        AuthenticationStatus;

  ASSERT (FvHandle != NULL);

  Status = gBS->HandleProtocol (
                  FvHandle,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  (VOID **) &Fv
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
    // Try reading PE32 section, if the required section is TE type 
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

  return Status;
}


EFI_STATUS
EFIAPI
GetSectionFromCurrentFv (
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         Instance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
    )
{
  return GetSectionFromFv(
          ImageHandleToFvHandle(gImageHandle),
          NameGuid,
          SectionType,
          Instance,
          Buffer,
          Size
          );
}



EFI_STATUS
EFIAPI
GetSectionFromCurrentFfs (
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         Instance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
    )
{
  return GetSectionFromFv(
          ImageHandleToFvHandle(gImageHandle),
          &gEfiCallerIdGuid,
          SectionType,
          Instance,
          Buffer,
          Size
          );
}

