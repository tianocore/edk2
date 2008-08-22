/** @file
  Mde PI library functions.

  Copyright (c) 2007 - 2008, Intel Corporation<BR>
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
#include <Library/DxePiLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>


/**
  Identify the device handle from which the Image is loaded from. As this device handle is passed to
  GetSectionFromFv as the identifier for a Firmware Volume, an EFI_FIRMWARE_VOLUME2_PROTOCOL 
  protocol instance should be located succesfully by calling gBS->HandleProtocol ().

  This function locates the EFI_LOADED_IMAGE_PROTOCOL instance installed
  on ImageHandle. It then returns EFI_LOADED_IMAGE_PROTOCOL.DeviceHandle.
  
  If ImageHandle is NULL, then ASSERT ();
  If failed to locate a EFI_LOADED_IMAGE_PROTOCOL on ImageHandle, then ASSERT ();
  
  @param  ImageHandle         The firmware allocated handle for UEFI image.

  @retval  EFI_HANDLE	        The device handle from which the Image is loaded from.

**/
EFI_HANDLE
InternalImageHandleToFvHandle (
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

/**
    Allocate and fill a buffer from a Firmware Section identified by a Firmware File GUID name, a Firmware 
    Section type and instance number from the specified Firmware Volume.
  
    This functions first locate the EFI_FIRMWARE_VOLUME2_PROTOCOL protocol instance on FvHandle in order to 
    carry out the Firmware Volume read operation. The function then reads the Firmware Section found sepcifed 
    by NameGuid, SectionType and SectionInstance. 
    
    The details of this search order is defined in description of EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection () 
    found in PI Specification.
    
    If SectionType is EFI_SECTION_TE, EFI_SECTION_TE is used as section type to start the search. If EFI_SECTION_TE section 
    is not found, EFI_SECTION_PE32 will be used to try the search again. If no EFI_SECTION_PE32 section is found, EFI_NOT_FOUND 
    is returned.
    
    The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
    by this function. This function can be only called at TPL_NOTIFY and below.
    
    If FvHandle is NULL, then ASSERT ();
    If NameGuid is NULL, then ASSERT();
    If Buffer is NULL, then ASSERT();
    If Size is NULL, then ASSERT().

    @param  FvHandle                The device handle that contains a instance of EFI_FIRMWARE_VOLUME2_PROTOCOL instance.
    @param  NameGuid                The GUID name of a Firmware File.
    @param  SectionType             The Firmware Section type.
    @param  SectionInstance         The instance number of Firmware Section to read from starting from 0.
    @param  Buffer                  On output, Buffer contains the the data read from the section in the Firmware File found.
    @param  Size                    On output, the size of Buffer.
  
    @retval  EFI_SUCCESS            The image is found and data and size is returned.
    @retval  EFI_UNSUPPORTED        FvHandle does not support EFI_FIRMWARE_VOLUME2_PROTOCOL.
    @retval  EFI_NOT_FOUND          The image specified by NameGuid and SectionType can't be found.
    @retval  EFI_OUT_OF_RESOURCES   There were not enough resources to allocate the output data buffer or complete the operations.
    @retval  EFI_DEVICE_ERROR       A hardware error occurs during reading from the Firmware Volume.
    @retval  EFI_ACCESS_DENIED      The firmware volume containing the searched Firmware File is configured to disallow reads.
  
**/
EFI_STATUS
GetSectionFromFv (
  IN  EFI_HANDLE                    FvHandle,
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
  )
{
  EFI_STATUS                    Status;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  UINT32                        AuthenticationStatus;

  ASSERT (NameGuid != NULL);
  ASSERT (Buffer != NULL);
  ASSERT (Size != NULL);
  
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
                      SectionInstance,
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
                    SectionInstance,
                    Buffer,
                    Size,
                    &AuthenticationStatus
                    );
  }

  return Status;
}



/**
  Locates a requested firmware section within a file and returns it to a buffer allocated by this function. 

  PiLibGetSectionFromAnyFv () is used to read a specific section from a file within a firmware volume. The function
  will search the first file with the specified name in all firmware volumes in the system. The search order for firmware 
  volumes in the system is determistic but abitrary if no new firmware volume is added into the system between 
  each calls of this function. 

  After the specific file is located, the function searches the specifc firmware section with type SectionType in this file. 
  The details of this search order is defined in description of EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection () 
  found in PI Specification.

  If SectionType is EFI_SECTION_TE, EFI_SECTION_TE is used as section type to start the search. If EFI_SECTION_TE section 
  is not found, EFI_SECTION_PE32 will be used to try the search again. If no EFI_SECTION_PE32 section is found, EFI_NOT_FOUND 
  is returned.

  The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
  by this function. This function can only be called at TPL_NOTIFY and below.

  If NameGuid is NULL, then ASSERT();
  If Buffer is NULL, then ASSERT();
  If Size is NULL, then ASSERT().

  @param  NameGuid             Pointer to an EFI_GUID, which indicates the file name from which the requested 
                               section will be read. Type EFI_GUID is defined in 
                               InstallProtocolInterface() in the UEFI 2.0 specification. 
  @param  SectionType          Indicates the section type to return. SectionType in conjunction with 
                               SectionInstance indicates which section to return. Type 
                               EFI_SECTION_TYPE is defined in EFI_COMMON_SECTION_HEADER.
  @param  SectionInstance      Indicates which instance of sections with a type of SectionType to return. 
                               SectionType in conjunction with SectionInstance indicates which section to 
                               return. SectionInstance is zero based.
  @param  Buffer               Pointer to a pointer to a buffer in which the section contents are returned, not 
                               including the section header. Caller is responsible to free this memory.
  @param  Size                 Pointer to a caller-allocated UINTN. It indicates the size of the memory represented by 
                               *Buffer.

  @retval  EFI_SUCCESS        The image is found and data and size is returned.
  @retval  EFI_NOT_FOUND      The image specified by NameGuid and SectionType can't be found.
  @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
  @retval  EFI_DEVICE_ERROR A hardware error occurs during reading from the Firmware Volume.
  @retval  EFI_ACCESS_DENIED  The firmware volume containing the searched Firmware File is configured to disallow reads.

**/
EFI_STATUS
EFIAPI
PiLibGetSectionFromAnyFv (
  IN CONST  EFI_GUID           *NameGuid,
  IN        EFI_SECTION_TYPE   SectionType,
  IN        UINTN              SectionInstance,
  OUT       VOID               **Buffer,
  OUT       UINTN              *Size
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;
  UINTN                         Index;
  EFI_HANDLE                    FvHandle;

  //
  // Search the FV that contain the caller's FFS first.
  // FV builder can choose to build FFS into the this FV
  // so that this implementation of GetSectionFromAnyFv
  // will locate the FFS faster.
  //
  FvHandle = InternalImageHandleToFvHandle (gImageHandle);
  Status = GetSectionFromFv (
             FvHandle,
             NameGuid,
             SectionType,
             SectionInstance,
             Buffer,
             Size
             );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
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

  for (Index = 0; Index < HandleCount; Index++) {
    //
    // Skip the FV that contain the caller's FFS
    //
    if (HandleBuffer[Index] != FvHandle) {
      Status = GetSectionFromFv (
                 HandleBuffer[Index], 
                 NameGuid, 
                 SectionType, 
                 SectionInstance,
                 Buffer, 
                 Size
                 );

      if (!EFI_ERROR (Status)) {
        goto Done;
      }
    }

  }

  if (Index == HandleCount) {
    Status = EFI_NOT_FOUND;
  }

Done:
  
  if (HandleBuffer != NULL) {  
    FreePool(HandleBuffer);
  }
  return Status;
  
}

/**
  Locates a requested firmware section within a file and returns it to a buffer allocated by this function. 

  PiLibGetSectionFromCurrentFv () is used to read a specific section from a file within the same firmware volume from which
  the running image is loaded. If the specific file is found, the function searches the specifc firmware section with type SectionType. 
  The details of this search order is defined in description of EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection () 
  found in PI Specification.

  If SectionType is EFI_SECTION_TE, EFI_SECTION_TE is used as section type to start the search. If EFI_SECTION_TE section 
  is not found, EFI_SECTION_PE32 will be used to try the search again. If no EFI_SECTION_PE32 section is found, EFI_NOT_FOUND 
  is returned.

  The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
  by this function. This function can be only called at TPL_NOTIFY and below.

  If NameGuid is NULL, then ASSERT();
  If Buffer is NULL, then ASSERT();
  If Size is NULL, then ASSERT().

  @param  NameGuid             Pointer to an EFI_GUID, which indicates the file name from which the requested 
                               section will be read. Type EFI_GUID is defined in 
                               InstallProtocolInterface() in the UEFI 2.0 specification. 
  @param  SectionType          Indicates the section type to return. SectionType in conjunction with 
                               SectionInstance indicates which section to return. Type 
                               EFI_SECTION_TYPE is defined in EFI_COMMON_SECTION_HEADER.
  @param  SectionInstance      Indicates which instance of sections with a type of SectionType to return. 
                               SectionType in conjunction with SectionInstance indicates which section to 
                               return. SectionInstance is zero based.
  @param  Buffer               Pointer to a pointer to a buffer in which the section contents are returned, not 
                               including the section header. Caller is responsible to free this memory.
  @param  Size                 Pointer to a caller-allocated UINTN. It indicates the size of the memory represented by 
                               *Buffer.


  @retval  EFI_SUCCESS        The image is found and data and size is returned.
  @retval  EFI_UNSUPPORTED   FvHandle does not support EFI_FIRMWARE_VOLUME2_PROTOCOL.
  @retval  EFI_NOT_FOUND      The image specified by NameGuid and SectionType can't be found.
  @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
  @retval  EFI_DEVICE_ERROR A hardware error occurs during reading from the Firmware Volume.
  @retval  EFI_ACCESS_DENIED  The firmware volume containing the searched Firmware File is configured to disallow reads.
  
**/
EFI_STATUS
EFIAPI
PiLibGetSectionFromCurrentFv (
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
    )
{
  return GetSectionFromFv(
          InternalImageHandleToFvHandle(gImageHandle),
          NameGuid,
          SectionType,
          SectionInstance,
          Buffer,
          Size
          );
}


/**
  Locates a requested firmware section within a file and returns it to a buffer allocated by this function. 

  PiLibGetSectionFromCurrentFfs () searches the specifc firmware section with type SectionType in the same firmware file from
  which the running image is loaded. The details of this search order is defined in description of 
  EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection () found in PI Specification.

  If SectionType is EFI_SECTION_TE, EFI_SECTION_TE is used as section type to start the search. If EFI_SECTION_TE section 
  is not found, EFI_SECTION_PE32 will be used to try the search again. If no EFI_SECTION_PE32 section is found, EFI_NOT_FOUND 
  is returned.


  The data and size is returned by Buffer and Size. The caller is responsible to free the Buffer allocated 
  by this function. This function can only be called at TPL_NOTIFY and below.

  If Buffer is NULL, then ASSERT();
  If Size is NULL, then ASSERT().

  @param  SectionType          Indicates the section type to return. SectionType in conjunction with 
                               SectionInstance indicates which section to return. Type 
                               EFI_SECTION_TYPE is defined in EFI_COMMON_SECTION_HEADER.
  @param  SectionInstance      Indicates which instance of sections with a type of SectionType to return. 
                               SectionType in conjunction with SectionInstance indicates which section to 
                               return. SectionInstance is zero based.
  @param  Buffer               Pointer to a pointer to a buffer in which the section contents are returned, not 
                               including the section header. Caller is responsible to free this memory.
  @param  Size                 Pointer to a caller-allocated UINTN. It indicates the size of the memory represented by 
                               *Buffer.

  @retval  EFI_SUCCESS        The image is found and data and size is returned.
  @retval  EFI_NOT_FOUND      The image specified by NameGuid and SectionType can't be found.
  @retval  EFI_OUT_OF_RESOURCES There were not enough resources to allocate the output data buffer or complete the operations.
  @retval  EFI_DEVICE_ERROR A hardware error occurs during reading from the Firmware Volume.
  @retval  EFI_ACCESS_DENIED  The firmware volume containing the searched Firmware File is configured to disallow reads.
  
**/
EFI_STATUS
EFIAPI
PiLibGetSectionFromCurrentFfs (
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
    )
{
  return GetSectionFromFv(
          InternalImageHandleToFvHandle(gImageHandle),
          &gEfiCallerIdGuid,
          SectionType,
          SectionInstance,
          Buffer,
          Size
          );
}

