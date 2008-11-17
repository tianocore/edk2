/** @file
  MDE DXE Services Library provides functions that simplify the development of DXE Drivers.  
  These functions help access data from sections of FFS files.

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
#include <Library/DxeServicesLib.h>
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

  @retval  EFI_HANDLE          The device handle from which the Image is loaded from.

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
InternalGetSectionFromFv (
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
  Searches all the availables firmware volumes and returns the first matching FFS section. 

  This function searches all the firmware volumes for FFS files with an FFS filename specified by NameGuid.  
  The order that the firmware volumes is searched is not deterministic. For each FFS file found a search 
  is made for FFS sections of type SectionType. If the FFS file contains at least SectionInstance instances 
  of the FFS section specified by SectionType, then the SectionInstance instance is returned in Buffer. 
  Buffer is allocated using AllocatePool(), and the size of the allocated buffer is returned in Size. 
  It is the caller's responsibility to use FreePool() to free the allocated buffer.  
  See EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection() for details on how sections 
  are retrieved from an FFS file based on SectionType and SectionInstance.

  If SectionType is EFI_SECTION_TE, and the search with an FFS file fails, 
  the search will be retried with a section type of EFI_SECTION_PE32.
  This function must be called with a TPL <= TPL_NOTIFY.

  If NameGuid is NULL, then ASSERT().
  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().


  @param  NameGuid             A pointer to to the FFS filename GUID to search for within 
                               any of the firmware volumes in the platform. 
  @param  SectionType          Indicates the FFS section type to search for within the FFS file specified by NameGuid.
  @param  SectionInstance      Indicates which section instance within the FFS file specified by NameGuid to retrieve.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.  
                               Is it the caller's respobsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.

  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough rsources available to retrieve the matching FFS section.
  @retval  EFI_DEVICE_ERROR     The FFS section could not be retrieves due to a device error.
  @retval  EFI_ACCESS_DENIED    The FFS section could not be retrieves because the firmware volume that 
                                contains the matching FFS section does not allow reads.
**/
EFI_STATUS
EFIAPI
GetSectionFromAnyFv  (
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
  Status = InternalGetSectionFromFv (
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
      Status = InternalGetSectionFromFv (
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
  Searches the firmware volume that the currently executing module was loaded from and returns the first matching FFS section. 

  This function searches the firmware volume that the currently executing module was loaded 
  from for an FFS file with an FFS filename specified by NameGuid. If the FFS file is found a search 
  is made for FFS sections of type SectionType. If the FFS file contains at least SectionInstance 
  instances of the FFS section specified by SectionType, then the SectionInstance instance is returned in Buffer.
  Buffer is allocated using AllocatePool(), and the size of the allocated buffer is returned in Size. 
  It is the caller's responsibility to use FreePool() to free the allocated buffer. 
  See EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection() for details on how sections are retrieved from 
  an FFS file based on SectionType and SectionInstance.

  If the currently executing module was not loaded from a firmware volume, then EFI_NOT_FOUND is returned.
  If SectionType is EFI_SECTION_TE, and the search with an FFS file fails, 
  the search will be retried with a section type of EFI_SECTION_PE32.
  
  This function must be called with a TPL <= TPL_NOTIFY.
  If NameGuid is NULL, then ASSERT().
  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().

  @param  NameGuid             A pointer to to the FFS filename GUID to search for within 
                               the firmware volumes that the currently executing module was loaded from.
  @param  SectionType          Indicates the FFS section type to search for within the FFS file specified by NameGuid.
  @param  SectionInstance      Indicates which section instance within the FFS file specified by NameGuid to retrieve.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.  
                               Is it the caller's respobsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.


  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough rsources available to retrieve the matching FFS section.
  @retval  EFI_DEVICE_ERROR     The FFS section could not be retrieves due to a device error.
  @retval  EFI_ACCESS_DENIED    The FFS section could not be retrieves because the firmware volume that 
                                contains the matching FFS section does not allow reads.  
**/
EFI_STATUS
EFIAPI
GetSectionFromFv (
  IN  CONST EFI_GUID                *NameGuid,
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
    )
{
  return InternalGetSectionFromFv (
           InternalImageHandleToFvHandle(gImageHandle),
           NameGuid,
           SectionType,
           SectionInstance,
           Buffer,
           Size
           );
}


/**
  Searches the FFS file the the currently executing module was loaded from and returns the first matching FFS section.

  This function searches the FFS file that the currently executing module was loaded from for a FFS sections of type SectionType.
  If the FFS file contains at least SectionInstance instances of the FFS section specified by SectionType, 
  then the SectionInstance instance is returned in Buffer. Buffer is allocated using AllocatePool(), 
  and the size of the allocated buffer is returned in Size. It is the caller's responsibility 
  to use FreePool() to free the allocated buffer. See EFI_FIRMWARE_VOLUME2_PROTOCOL.ReadSection() for 
  details on how sections are retrieved from an FFS file based on SectionType and SectionInstance.

  If the currently executing module was not loaded from an FFS file, then EFI_NOT_FOUND is returned.
  If SectionType is EFI_SECTION_TE, and the search with an FFS file fails, 
  the search will be retried with a section type of EFI_SECTION_PE32.
  This function must be called with a TPL <= TPL_NOTIFY.
  
  If Buffer is NULL, then ASSERT().
  If Size is NULL, then ASSERT().


  @param  SectionType          Indicates the FFS section type to search for within the FFS file 
                               that the currently executing module was loaded from.
  @param  SectionInstance      Indicates which section instance to retrieve within the FFS file 
                               that the currently executing module was loaded from.
  @param  Buffer               On output, a pointer to a callee allocated buffer containing the FFS file section that was found.  
                               Is it the caller's respobsibility to free this buffer using FreePool().
  @param  Size                 On output, a pointer to the size, in bytes, of Buffer.

  @retval  EFI_SUCCESS          The specified FFS section was returned.
  @retval  EFI_NOT_FOUND        The specified FFS section could not be found.
  @retval  EFI_OUT_OF_RESOURCES There are not enough rsources available to retrieve the matching FFS section.
  @retval  EFI_DEVICE_ERROR     The FFS section could not be retrieves due to a device error.
  @retval  EFI_ACCESS_DENIED    The FFS section could not be retrieves because the firmware volume that 
                                contains the matching FFS section does not allow reads.  
  
**/
EFI_STATUS
EFIAPI
GetSectionFromFfs (
  IN  EFI_SECTION_TYPE              SectionType,
  IN  UINTN                         SectionInstance,
  OUT VOID                          **Buffer,
  OUT UINTN                         *Size
    )
{
  return InternalGetSectionFromFv(
           InternalImageHandleToFvHandle(gImageHandle),
           &gEfiCallerIdGuid,
           SectionType,
           SectionInstance,
           Buffer,
           Size
           );
}

