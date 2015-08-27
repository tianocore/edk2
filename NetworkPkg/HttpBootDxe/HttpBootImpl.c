/** @file
  The implementation of EFI_LOAD_FILE_PROTOCOL for UEFI HTTP boot.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HttpBootDxe.h"

/**
  Enable the use of UEFI HTTP boot function.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              HTTP boot was successfully enabled.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval EFI_ALREADY_STARTED      The driver is already in started state.
  
**/
EFI_STATUS
HttpBootStart (
  IN HTTP_BOOT_PRIVATE_DATA           *Private
  )
{
  UINTN          Index;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->Started) {
    return EFI_ALREADY_STARTED;
  }

  if (!Private->UsingIpv6) {
    //
    // Init the content of cached DHCP offer list.
    //
    ZeroMem (Private->OfferBuffer, sizeof (Private->OfferBuffer));
    for (Index = 0; Index < HTTP_BOOT_OFFER_MAX_NUM; Index++) {
      Private->OfferBuffer[Index].Dhcp4.Packet.Offer.Size = HTTP_BOOT_DHCP4_PACKET_MAX_SIZE;
    }
  } else {
    ASSERT (FALSE);
  }

  Private->Started = TRUE;

  return EFI_SUCCESS;
}

/**
  Attempt to complete a DHCPv4 D.O.R.A sequence to retrieve the boot resource information.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              Boot info was successfully retrieved.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval EFI_NOT_STARTED          The driver is in stopped state.
  @retval EFI_DEVICE_ERROR         An unexpected network error occurred.
  @retval Others                   Other errors as indicated.
  
**/
EFI_STATUS
HttpBootDhcp (
  IN HTTP_BOOT_PRIVATE_DATA           *Private
  )
{
  EFI_STATUS                Status;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }

  Status = EFI_DEVICE_ERROR;

  if (!Private->UsingIpv6) {
    Status = HttpBootDhcp4Dora (Private);
  } else {
    ASSERT (FALSE);
  }

  return Status;
}

/**
  Attempt to download the boot file through HTTP message exchange.

  @param[in]          Private         The pointer to the driver's private data.
  @param[in, out]     BufferSize      On input the size of Buffer in bytes. On output with a return
                                      code of EFI_SUCCESS, the amount of data transferred to
                                      Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of Buffer required to retrieve the requested file.
  @param[in]          Buffer          The memory buffer to transfer the file to. If Buffer is NULL,
                                      then the size of the requested file is returned in
                                      BufferSize.

  @retval EFI_SUCCESS                 Boot file was loaded successfully.
  @retval EFI_INVALID_PARAMETER       Private is NULL.
  @retval EFI_NOT_STARTED             The driver is in stopped state.
  @retval EFI_BUFFER_TOO_SMALL        The BufferSize is too small to read the boot file. BufferSize has 
                                      been updated with the size needed to complete the request.
  @retval EFI_DEVICE_ERROR            An unexpected network error occurred.
  @retval Others                      Other errors as indicated.
  
**/
EFI_STATUS
HttpBootLoadFile (
  IN     HTTP_BOOT_PRIVATE_DATA       *Private,
  IN OUT UINTN                        *BufferSize,
  IN     VOID                         *Buffer       OPTIONAL
  )
{
  EFI_STATUS             Status;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }

  Status = EFI_DEVICE_ERROR;

  if (Private->BootFileUri == NULL) {
    //
    // Parse the cached offer to get the boot file URL first.
    //
    Status = HttpBootDiscoverBootInfo (Private);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!Private->HttpCreated) {
    //
    // Create HTTP child.
    //
    Status = HttpBootCreateHttpIo (Private);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Private->BootFileSize == 0) {
    //
    // Discover the information about the bootfile if we haven't.
    //

    //
    // Try to use HTTP HEAD method.
    //
    Status = HttpBootGetBootFile (
               Private,
               TRUE,
               &Private->BootFileSize,
               NULL
               );
    if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
      //
      // Failed to get file size by HEAD method, may be trunked encoding, try HTTP GET method.
      //
      ASSERT (Private->BootFileSize == 0);
      Status = HttpBootGetBootFile (
                 Private,
                 FALSE,
                 &Private->BootFileSize,
                 NULL
                 );
      if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
        return Status;
      }
    }
  }

  if (*BufferSize < Private->BootFileSize) {
    *BufferSize = Private->BootFileSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Load the boot file into Buffer
  //
  return  HttpBootGetBootFile (
            Private,
            FALSE,
            BufferSize,
            Buffer
            );
}

/**
  Disable the use of UEFI HTTP boot function.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              HTTP boot was successfully disabled.
  @retval EFI_NOT_STARTED          The driver is already in stopped state.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval Others                   Unexpected error when stop the function.
  
**/
EFI_STATUS
HttpBootStop (
  IN HTTP_BOOT_PRIVATE_DATA           *Private
  )
{
  UINTN            Index;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }
  
  if (Private->HttpCreated) {
    HttpIoDestroyIo (&Private->HttpIo);
    Private->HttpCreated = FALSE;
  }
  
  Private->Started = FALSE;
  ZeroMem (&Private->StationIp, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->SubnetMask, sizeof (EFI_IP_ADDRESS));
  ZeroMem (&Private->GatewayIp, sizeof (EFI_IP_ADDRESS));
  Private->Port = 0;
  Private->BootFileUri = NULL;
  Private->BootFileUriParser = NULL;
  Private->BootFileSize = 0;
  Private->SelectIndex = 0;
  Private->SelectProxyType = HttpOfferTypeMax;

  if (!Private->UsingIpv6) {
    //
    // Stop and release the DHCP4 child.
    //
    Private->Dhcp4->Stop (Private->Dhcp4);
    Private->Dhcp4->Configure (Private->Dhcp4, NULL);

    for (Index = 0; Index < HTTP_BOOT_OFFER_MAX_NUM; Index++) {
      if (Private->OfferBuffer[Index].Dhcp4.UriParser) {
        HttpUrlFreeParser (Private->OfferBuffer[Index].Dhcp4.UriParser);
      }
    }
  } else {
    ASSERT (FALSE);
  }
  
  ZeroMem (Private->OfferBuffer, sizeof (Private->OfferBuffer));
  Private->OfferNum = 0;
  ZeroMem (Private->OfferCount, sizeof (Private->OfferCount));
  ZeroMem (Private->OfferIndex, sizeof (Private->OfferIndex));

  return EFI_SUCCESS;
}

/**
  Causes the driver to load a specified file.

  @param  This       Protocol instance pointer.
  @param  FilePath   The device specific path of the file to load.
  @param  BootPolicy If TRUE, indicates that the request originates from the
                     boot manager is attempting to load FilePath as a boot
                     selection. If FALSE, then FilePath must match as exact file
                     to be loaded.
  @param  BufferSize On input the size of Buffer in bytes. On output with a return
                     code of EFI_SUCCESS, the amount of data transferred to
                     Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                     the size of Buffer required to retrieve the requested file.
  @param  Buffer     The memory buffer to transfer the file to. IF Buffer is NULL,
                     then the size of the requested file is returned in
                     BufferSize.

  @retval EFI_SUCCESS           The file was loaded.
  @retval EFI_UNSUPPORTED       The device does not support the provided BootPolicy
  @retval EFI_INVALID_PARAMETER FilePath is not a valid device path, or
                                BufferSize is NULL.
  @retval EFI_NO_MEDIA          No medium was present to load the file.
  @retval EFI_DEVICE_ERROR      The file was not loaded due to a device error.
  @retval EFI_NO_RESPONSE       The remote system did not respond.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_ABORTED           The file load process was manually cancelled.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small to read the current directory entry.
                                BufferSize has been updated with the size needed to complete
                                the request.

**/
EFI_STATUS
EFIAPI
HttpBootDxeLoadFile (
  IN EFI_LOAD_FILE_PROTOCOL           *This,
  IN EFI_DEVICE_PATH_PROTOCOL         *FilePath,
  IN BOOLEAN                          BootPolicy,
  IN OUT UINTN                        *BufferSize,
  IN VOID                             *Buffer OPTIONAL
  )
{
  HTTP_BOOT_PRIVATE_DATA        *Private;
  BOOLEAN                       MediaPresent;
  EFI_STATUS                    Status;

  if (This == NULL || BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Only support BootPolicy
  //
  if (!BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  Private = HTTP_BOOT_PRIVATE_DATA_FROM_LOADFILE (This);

  //
  // Check media status before HTTP boot start
  //
  MediaPresent = TRUE;
  NetLibDetectMedia (Private->Controller, &MediaPresent);
  if (!MediaPresent) {
    return EFI_NO_MEDIA;
  }

  //
  // Initialize HTTP boot and load the boot file.
  //
  Status = HttpBootStart (Private);
  if (Status == EFI_SUCCESS || Status == EFI_ALREADY_STARTED) {
    Status = HttpBootLoadFile (Private, BufferSize, Buffer);
  }

  if (Status != EFI_SUCCESS && Status != EFI_BUFFER_TOO_SMALL) {
    HttpBootStop (Private);
  } else {
    //
    // Stop and release the DHCP4 child.
    //
    Private->Dhcp4->Stop (Private->Dhcp4);
    Private->Dhcp4->Configure (Private->Dhcp4, NULL);
  }

  return Status;
}

///
/// Load File Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED 
EFI_LOAD_FILE_PROTOCOL  gHttpBootDxeLoadFile = {
  HttpBootDxeLoadFile
};
