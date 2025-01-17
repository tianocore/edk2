/** @file
  The implementation of EFI_LOAD_FILE_PROTOCOL for UEFI HTTP boot.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpBootDxe.h"

/**
  Install HTTP Boot Callback Protocol if not installed before.

  @param[in] Private           Pointer to HTTP Boot private data.

  @retval EFI_SUCCESS          HTTP Boot Callback Protocol installed successfully.
  @retval Others               Failed to install HTTP Boot Callback Protocol.

**/
EFI_STATUS
HttpBootInstallCallback (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  ControllerHandle;

  if (!Private->UsingIpv6) {
    ControllerHandle = Private->Ip4Nic->Controller;
  } else {
    ControllerHandle = Private->Ip6Nic->Controller;
  }

  //
  // Check whether gEfiHttpBootCallbackProtocolGuid already installed.
  //
  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gEfiHttpBootCallbackProtocolGuid,
                  (VOID **)&Private->HttpBootCallback
                  );
  if (Status == EFI_UNSUPPORTED) {
    CopyMem (
      &Private->LoadFileCallback,
      &gHttpBootDxeHttpBootCallback,
      sizeof (EFI_HTTP_BOOT_CALLBACK_PROTOCOL)
      );

    //
    // Install a default callback if user didn't offer one.
    //
    Status = gBS->InstallProtocolInterface (
                    &ControllerHandle,
                    &gEfiHttpBootCallbackProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &Private->LoadFileCallback
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Private->HttpBootCallback = &Private->LoadFileCallback;
  }

  return EFI_SUCCESS;
}

/**
  Uninstall HTTP Boot Callback Protocol if it's installed by this driver.

  @param[in] Private           Pointer to HTTP Boot private data.

**/
VOID
HttpBootUninstallCallback (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  EFI_HANDLE  ControllerHandle;

  if (Private->HttpBootCallback == &Private->LoadFileCallback) {
    if (!Private->UsingIpv6) {
      ControllerHandle = Private->Ip4Nic->Controller;
    } else {
      ControllerHandle = Private->Ip6Nic->Controller;
    }

    gBS->UninstallProtocolInterface (
           ControllerHandle,
           &gEfiHttpBootCallbackProtocolGuid,
           Private->HttpBootCallback
           );
    Private->HttpBootCallback = NULL;
  }
}

/**
  Enable the use of UEFI HTTP boot function.

  If the driver has already been started but not satisfy the requirement (IP stack and
  specified boot file path), this function will stop the driver and start it again.

  @param[in]    Private            The pointer to the driver's private data.
  @param[in]    UsingIpv6          Specifies the type of IP addresses that are to be
                                   used during the session that is being started.
                                   Set to TRUE for IPv6, and FALSE for IPv4.
  @param[in]    FilePath           The device specific path of the file to load.

  @retval EFI_SUCCESS              HTTP boot was successfully enabled.
  @retval EFI_INVALID_PARAMETER    Private is NULL or FilePath is NULL.
  @retval EFI_INVALID_PARAMETER    The FilePath doesn't contain a valid URI device path node.
  @retval EFI_ALREADY_STARTED      The driver is already in started state.
  @retval EFI_OUT_OF_RESOURCES     There are not enough resources.

**/
EFI_STATUS
HttpBootStart (
  IN HTTP_BOOT_PRIVATE_DATA    *Private,
  IN BOOLEAN                   UsingIpv6,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  CHAR8       *Uri;

  Uri = NULL;

  if ((Private == NULL) || (FilePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the URI in the input FilePath, in order to see whether it is
  // required to boot from a new specified boot file.
  //
  Status = HttpBootParseFilePath (FilePath, &Uri);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether we need to stop and restart the HTTP boot driver.
  //
  if (Private->Started) {
    //
    // Restart is needed in 2 cases:
    // 1. Http boot driver has already been started but not on the required IP stack.
    // 2. The specified boot file URI in FilePath is different with the one we have
    // recorded before.
    //
    if ((UsingIpv6 != Private->UsingIpv6) ||
        ((Uri != NULL) && (AsciiStrCmp (Private->BootFileUri, Uri) != 0)))
    {
      //
      // Restart is required, first stop then continue this start function.
      //
      Status = HttpBootStop (Private);
      if (EFI_ERROR (Status)) {
        if (Uri != NULL) {
          FreePool (Uri);
        }

        return Status;
      }
    } else {
      //
      // Restart is not required.
      //
      if (Uri != NULL) {
        FreePool (Uri);
      }

      return EFI_ALREADY_STARTED;
    }
  }

  //
  // Detect whether using ipv6 or not, and set it to the private data.
  //
  if (UsingIpv6 && (Private->Ip6Nic != NULL)) {
    Private->UsingIpv6 = TRUE;
  } else if (!UsingIpv6 && (Private->Ip4Nic != NULL)) {
    Private->UsingIpv6 = FALSE;
  } else {
    if (Uri != NULL) {
      FreePool (Uri);
    }

    return EFI_UNSUPPORTED;
  }

  //
  // Record the specified URI and prepare the URI parser if needed.
  //
  Private->FilePathUri = Uri;
  if (Private->FilePathUri != NULL) {
    Status = HttpParseUrl (
               Private->FilePathUri,
               (UINT32)AsciiStrLen (Private->FilePathUri),
               FALSE,
               &Private->FilePathUriParser
               );
    if (EFI_ERROR (Status)) {
      FreePool (Private->FilePathUri);
      return Status;
    }
  }

  //
  // Init the content of cached DHCP offer list.
  //
  ZeroMem (Private->OfferBuffer, sizeof (Private->OfferBuffer));
  if (!Private->UsingIpv6) {
    for (Index = 0; Index < HTTP_BOOT_OFFER_MAX_NUM; Index++) {
      Private->OfferBuffer[Index].Dhcp4.Packet.Offer.Size = HTTP_CACHED_DHCP4_PACKET_MAX_SIZE;
    }
  } else {
    for (Index = 0; Index < HTTP_BOOT_OFFER_MAX_NUM; Index++) {
      Private->OfferBuffer[Index].Dhcp6.Packet.Offer.Size = HTTP_CACHED_DHCP6_PACKET_MAX_SIZE;
    }
  }

  if (Private->UsingIpv6) {
    //
    // Set Ip6 policy to Automatic to start the Ip6 router discovery.
    //
    Status = HttpBootSetIp6Policy (Private);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Private->Started = TRUE;
  Print (L"\n>>Start HTTP Boot over IPv%d", Private->UsingIpv6 ? 6 : 4);

  return EFI_SUCCESS;
}

/**
  Attempt to complete a DHCPv4 D.O.R.A or DHCPv6 S.R.A.A sequence to retrieve the boot resource information.

  @param[in]    Private            The pointer to the driver's private data.

  @retval EFI_SUCCESS              Boot info was successfully retrieved.
  @retval EFI_INVALID_PARAMETER    Private is NULL.
  @retval EFI_NOT_STARTED          The driver is in stopped state.
  @retval EFI_DEVICE_ERROR         An unexpected network error occurred.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
HttpBootDhcp (
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  EFI_STATUS  Status;

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }

  Status = EFI_DEVICE_ERROR;

  if (!Private->UsingIpv6) {
    //
    // Start D.O.R.A process to get a IPv4 address and other boot information.
    //
    Status = HttpBootDhcp4Dora (Private);
  } else {
    //
    // Start S.A.R.R process to get a IPv6 address and other boot information.
    //
    Status = HttpBootDhcp6Sarr (Private);
  }

  return Status;
}

/**
  Issue calls to HttpBootGetBootFile() based on current Boot File State
  @param[in]          Private         The pointer to the driver's private data.
  @param[in, out]     BufferSize      On input the size of Buffer in bytes. On output with a return
                                      code of EFI_SUCCESS, the amount of data transferred to
                                      Buffer. On output with a return code of EFI_BUFFER_TOO_SMALL,
                                      the size of Buffer required to retrieve the requested file.
  @param[in]          Buffer          The memory buffer to transfer the file to. If Buffer is NULL,
                                      then the size of the requested file is returned in
                                      BufferSize.
  @param[out]         ImageType       The image type of the downloaded file.
  @retval EFI_SUCCESS              The file was loaded.
  @retval EFI_INVALID_PARAMETER    BufferSize is NULL or Buffer Size is not NULL but Buffer is NULL.
  @retval EFI_OUT_OF_RESOURCES     Could not allocate needed resources
  @retval EFI_BUFFER_TOO_SMALL     The BufferSize is too small to read the current directory entry.
                                   BufferSize has been updated with the size needed to complete
                                   the request.
  @retval EFI_ACCESS_DENIED        Server authentication failed.
  @retval Others                   Unexpected error happened.
**/
EFI_STATUS
HttpBootGetBootFileCaller (
  IN     HTTP_BOOT_PRIVATE_DATA  *Private,
  IN OUT UINTN                   *BufferSize,
  IN     VOID                    *Buffer        OPTIONAL,
  OUT    HTTP_BOOT_IMAGE_TYPE    *ImageType
  )
{
  HTTP_GET_BOOT_FILE_STATE  State;
  EFI_STATUS                Status;
  UINT32                    Retries;

  if (Private->BootFileSize == 0) {
    State = GetBootFileHead;
  } else {
    State = LoadBootFile;
  }

  for ( ; ;) {
    switch (State) {
      case GetBootFileHead:
        //
        // Try to use HTTP HEAD method.
        //
        Status = HttpBootGetBootFile (
                   Private,
                   TRUE,
                   &Private->BootFileSize,
                   NULL,
                   &Private->ImageType
                   );
        if ((EFI_ERROR (Status)) && (Status != EFI_BUFFER_TOO_SMALL)) {
          if ((Private->AuthData != NULL) && (Status == EFI_ACCESS_DENIED)) {
            //
            // Try to use HTTP HEAD method again since the Authentication information is provided.
            //
            State = GetBootFileHead;
          } else {
            State = GetBootFileGet;
          }
        } else {
          State = LoadBootFile;
        }

        break;

      case GetBootFileGet:
        //
        // Failed to get file size by HEAD method, may be trunked encoding, try HTTP GET method.
        //
        ASSERT (Private->BootFileSize == 0);
        Status = HttpBootGetBootFile (
                   Private,
                   FALSE,
                   &Private->BootFileSize,
                   NULL,
                   &Private->ImageType
                   );
        if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
          State = GetBootFileError;
        } else {
          State = LoadBootFile;
        }

        break;

      case LoadBootFile:
        if (*BufferSize < Private->BootFileSize) {
          *BufferSize = Private->BootFileSize;
          *ImageType  = Private->ImageType;
          Status      = EFI_BUFFER_TOO_SMALL;
          return Status;
        }

        //
        // Load the boot file into Buffer
        //
        for (Retries = 1; Retries <= PcdGet32 (PcdMaxHttpResumeRetries); Retries++) {
          Status = HttpBootGetBootFile (
                     Private,
                     FALSE,
                     BufferSize,
                     Buffer,
                     ImageType
                     );
          if (!EFI_ERROR (Status) ||
              ((Status != EFI_TIMEOUT) && (Status != EFI_DEVICE_ERROR)) ||
              (Retries >= PcdGet32 (PcdMaxHttpResumeRetries)))
          {
            break;
          }

          //
          // HttpBootGetBootFile returned EFI_TIMEOUT or EFI_DEVICE_ERROR.
          // We may attempt to resume the interrupted download.
          //

          Private->HttpCreated = FALSE;
          HttpIoDestroyIo (&Private->HttpIo);
          Status = HttpBootCreateHttpIo (Private);
          if (EFI_ERROR (Status)) {
            break;
          }

          DEBUG ((DEBUG_WARN | DEBUG_INFO, "HttpBootGetBootFileCaller: NBP file download interrupted, will try to resume the operation.\n"));
          gBS->Stall (1000 * 1000 * PcdGet32 (PcdHttpDelayBetweenResumeRetries));
        }

        if (EFI_ERROR (Status) && (Retries >= PcdGet32 (PcdMaxHttpResumeRetries))) {
          DEBUG ((DEBUG_ERROR, "HttpBootGetBootFileCaller: Error downloading NBP file, even after trying to resume %d times.\n", Retries));
        }

        return Status;

      case GetBootFileError:
      default:
        AsciiPrint ("\n  Error: Could not retrieve NBP file size from HTTP server.\n");
        return Status;
    }
  }
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
  @param[out]         ImageType       The image type of the downloaded file.

  @retval EFI_SUCCESS                 Boot file was loaded successfully.
  @retval EFI_INVALID_PARAMETER       Private is NULL, or ImageType is NULL, or BufferSize is NULL.
  @retval EFI_INVALID_PARAMETER       *BufferSize is not zero, and Buffer is NULL.
  @retval EFI_NOT_STARTED             The driver is in stopped state.
  @retval EFI_BUFFER_TOO_SMALL        The BufferSize is too small to read the boot file. BufferSize has
                                      been updated with the size needed to complete the request.
  @retval EFI_DEVICE_ERROR            An unexpected network error occurred.
  @retval Others                      Other errors as indicated.

**/
EFI_STATUS
HttpBootLoadFile (
  IN     HTTP_BOOT_PRIVATE_DATA  *Private,
  IN OUT UINTN                   *BufferSize,
  IN     VOID                    *Buffer        OPTIONAL,
  OUT HTTP_BOOT_IMAGE_TYPE       *ImageType
  )
{
  EFI_STATUS  Status;

  if ((Private == NULL) || (ImageType == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*BufferSize != 0) && (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Private->Started) {
    return EFI_NOT_STARTED;
  }

  Status = HttpBootInstallCallback (Private);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  if (Private->BootFileUri == NULL) {
    //
    // Parse the cached offer to get the boot file URL first.
    //
    Status = HttpBootDiscoverBootInfo (Private);
    if (EFI_ERROR (Status)) {
      AsciiPrint ("\n  Error: Could not retrieve NBP file size from HTTP server.\n");
      goto ON_EXIT;
    }
  }

  if (!Private->HttpCreated) {
    //
    // Create HTTP child.
    //
    Status = HttpBootCreateHttpIo (Private);
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  //
  // Load the boot file
  //
  Status = HttpBootGetBootFileCaller (Private, BufferSize, Buffer, ImageType);

ON_EXIT:
  HttpBootUninstallCallback (Private);

  if (EFI_ERROR (Status)) {
    if (Status == EFI_ACCESS_DENIED) {
      AsciiPrint ("\n  Error: Could not establish connection with HTTP server.\n");
    } else if ((Status == EFI_BUFFER_TOO_SMALL) && (Buffer != NULL)) {
      AsciiPrint ("\n  Error: Buffer size is smaller than the requested file.\n");
    } else if (Status == EFI_OUT_OF_RESOURCES) {
      AsciiPrint ("\n  Error: Could not allocate I/O buffers.\n");
    } else if (Status == EFI_DEVICE_ERROR) {
      AsciiPrint ("\n  Error: Network device error.\n");
    } else if (Status == EFI_TIMEOUT) {
      AsciiPrint ("\n  Error: Server response timeout.\n");
    } else if (Status == EFI_ABORTED) {
      AsciiPrint ("\n  Error: Remote boot cancelled.\n");
    } else if (Status != EFI_BUFFER_TOO_SMALL) {
      AsciiPrint ("\n  Error: Unexpected network error.\n");
    }
  }

  return Status;
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
  IN HTTP_BOOT_PRIVATE_DATA  *Private
  )
{
  UINTN  Index;

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
  Private->Port                   = 0;
  Private->BootFileUri            = NULL;
  Private->BootFileUriParser      = NULL;
  Private->BootFileSize           = 0;
  Private->SelectIndex            = 0;
  Private->SelectProxyType        = HttpOfferTypeMax;
  Private->PartialTransferredSize = 0;

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
    //
    // Stop and release the DHCP6 child.
    //
    Private->Dhcp6->Stop (Private->Dhcp6);
    Private->Dhcp6->Configure (Private->Dhcp6, NULL);

    for (Index = 0; Index < HTTP_BOOT_OFFER_MAX_NUM; Index++) {
      if (Private->OfferBuffer[Index].Dhcp6.UriParser) {
        HttpUrlFreeParser (Private->OfferBuffer[Index].Dhcp6.UriParser);
      }
    }
  }

  if (Private->AuthData != NULL) {
    FreePool (Private->AuthData);
    Private->AuthData = NULL;
  }

  if (Private->AuthScheme != NULL) {
    FreePool (Private->AuthScheme);
    Private->AuthScheme = NULL;
  }

  if (Private->DnsServerIp != NULL) {
    FreePool (Private->DnsServerIp);
    Private->DnsServerIp = NULL;
  }

  if (Private->FilePathUri != NULL) {
    FreePool (Private->FilePathUri);
    HttpUrlFreeParser (Private->FilePathUriParser);
    Private->FilePathUri       = NULL;
    Private->FilePathUriParser = NULL;
  }

  if (Private->LastModifiedOrEtag != NULL) {
    FreePool (Private->LastModifiedOrEtag);
    Private->LastModifiedOrEtag = NULL;
  }

  ZeroMem (Private->OfferBuffer, sizeof (Private->OfferBuffer));
  Private->OfferNum = 0;
  ZeroMem (Private->OfferCount, sizeof (Private->OfferCount));
  ZeroMem (Private->OfferIndex, sizeof (Private->OfferIndex));

  HttpBootFreeCacheList (Private);

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
  IN EFI_LOAD_FILE_PROTOCOL    *This,
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN BOOLEAN                   BootPolicy,
  IN OUT UINTN                 *BufferSize,
  IN VOID                      *Buffer OPTIONAL
  )
{
  HTTP_BOOT_PRIVATE_DATA  *Private;
  HTTP_BOOT_VIRTUAL_NIC   *VirtualNic;
  EFI_STATUS              MediaStatus;
  BOOLEAN                 UsingIpv6;
  EFI_STATUS              Status;
  HTTP_BOOT_IMAGE_TYPE    ImageType;

  if ((This == NULL) || (BufferSize == NULL) || (FilePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Only support BootPolicy
  //
  if (!BootPolicy) {
    return EFI_UNSUPPORTED;
  }

  VirtualNic = HTTP_BOOT_VIRTUAL_NIC_FROM_LOADFILE (This);
  Private    = VirtualNic->Private;

  //
  // Check media status before HTTP boot start
  //
  MediaStatus = EFI_SUCCESS;
  NetLibDetectMediaWaitTimeout (Private->Controller, HTTP_BOOT_CHECK_MEDIA_WAITING_TIME, &MediaStatus);
  if (MediaStatus != EFI_SUCCESS) {
    AsciiPrint ("\n  Error: Could not detect network connection.\n");
    return EFI_NO_MEDIA;
  }

  //
  // Check whether the virtual nic is using IPv6 or not.
  //
  UsingIpv6 = FALSE;
  if (VirtualNic == Private->Ip6Nic) {
    UsingIpv6 = TRUE;
  }

  //
  // Initialize HTTP boot.
  //
  Status = HttpBootStart (Private, UsingIpv6, FilePath);
  if ((Status != EFI_SUCCESS) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  //
  // Load the boot file.
  //
  ImageType = ImageTypeMax;
  Status    = HttpBootLoadFile (Private, BufferSize, Buffer, &ImageType);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_BUFFER_TOO_SMALL) && ((ImageType == ImageTypeVirtualCd) || (ImageType == ImageTypeVirtualDisk))) {
      Status = EFI_WARN_FILE_SYSTEM;
    } else if (Status != EFI_BUFFER_TOO_SMALL) {
      HttpBootStop (Private);
    }

    return Status;
  }

  //
  // Register the RAM Disk to the system if needed.
  //
  if ((ImageType == ImageTypeVirtualCd) || (ImageType == ImageTypeVirtualDisk)) {
    Status = HttpBootRegisterRamDisk (Private, *BufferSize, Buffer, ImageType);
    if (!EFI_ERROR (Status)) {
      Status = EFI_WARN_FILE_SYSTEM;
    } else {
      AsciiPrint ("\n  Error: Could not register RAM disk to the system.\n");
    }
  }

  //
  // Stop the HTTP Boot service after the boot image is downloaded.
  //
  HttpBootStop (Private);
  return Status;
}

///
/// Load File Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_LOAD_FILE_PROTOCOL  gHttpBootDxeLoadFile = {
  HttpBootDxeLoadFile
};

/**
  Callback function that is invoked when the HTTP Boot driver is about to transmit or has received a
  packet.

  This function is invoked when the HTTP Boot driver is about to transmit or has received packet.
  Parameters DataType and Received specify the type of event and the format of the buffer pointed
  to by Data. Due to the polling nature of UEFI device drivers, this callback function should not
  execute for more than 5 ms.
  The returned status code determines the behavior of the HTTP Boot driver.

  @param[in]  This                Pointer to the EFI_HTTP_BOOT_CALLBACK_PROTOCOL instance.
  @param[in]  DataType            The event that occurs in the current state.
  @param[in]  Received            TRUE if the callback is being invoked due to a receive event.
                                  FALSE if the callback is being invoked due to a transmit event.
  @param[in]  DataLength          The length in bytes of the buffer pointed to by Data.
  @param[in]  Data                A pointer to the buffer of data, the data type is specified by
                                  DataType.

  @retval EFI_SUCCESS             Tells the HTTP Boot driver to continue the HTTP Boot process.
  @retval EFI_ABORTED             Tells the HTTP Boot driver to abort the current HTTP Boot process.
**/
EFI_STATUS
EFIAPI
HttpBootCallback (
  IN EFI_HTTP_BOOT_CALLBACK_PROTOCOL   *This,
  IN EFI_HTTP_BOOT_CALLBACK_DATA_TYPE  DataType,
  IN BOOLEAN                           Received,
  IN UINT32                            DataLength,
  IN VOID                              *Data     OPTIONAL
  )
{
  EFI_HTTP_MESSAGE        *HttpMessage;
  EFI_HTTP_HEADER         *HttpHeader;
  HTTP_BOOT_PRIVATE_DATA  *Private;
  UINT32                  Percentage;

  Private = HTTP_BOOT_PRIVATE_DATA_FROM_CALLBACK_PROTOCOL (This);

  switch (DataType) {
    case HttpBootDhcp4:
    case HttpBootDhcp6:
      Print (L".");
      break;

    case HttpBootHttpRequest:
      if (Data != NULL) {
        HttpMessage = (EFI_HTTP_MESSAGE *)Data;
        if ((HttpMessage->Data.Request->Method == HttpMethodGet) &&
            (HttpMessage->Data.Request->Url != NULL) &&
            (Private->PartialTransferredSize == 0))
        {
          Print (L"\n  URI: %s\n", HttpMessage->Data.Request->Url);
        }
      }

      break;

    case HttpBootHttpResponse:
      if (Data != NULL) {
        HttpMessage = (EFI_HTTP_MESSAGE *)Data;

        if (HttpMessage->Data.Response != NULL) {
          if (HttpBootIsHttpRedirectStatusCode (HttpMessage->Data.Response->StatusCode)) {
            //
            // Server indicates the resource has been redirected to a different URL
            // according to the section 6.4 of RFC7231 and the RFC 7538.
            // Display the redirect information on the screen.
            //
            HttpHeader = HttpFindHeader (
                           HttpMessage->HeaderCount,
                           HttpMessage->Headers,
                           HTTP_HEADER_LOCATION
                           );
            if (HttpHeader != NULL) {
              Print (L"\n  HTTP ERROR: Resource Redirected.\n  New Location: %a\n", HttpHeader->FieldValue);
            }

            break;
          }
        }

        // If download was resumed, do not change progress variables
        HttpHeader = HttpFindHeader (
                       HttpMessage->HeaderCount,
                       HttpMessage->Headers,
                       HTTP_HEADER_CONTENT_RANGE
                       );
        if (HttpHeader) {
          break;
        }

        HttpHeader = HttpFindHeader (
                       HttpMessage->HeaderCount,
                       HttpMessage->Headers,
                       HTTP_HEADER_CONTENT_LENGTH
                       );
        if (HttpHeader != NULL) {
          Private->FileSize     = AsciiStrDecimalToUintn (HttpHeader->FieldValue);
          Private->ReceivedSize = 0;
          Private->Percentage   = 0;
        }
      }

      break;

    case HttpBootHttpEntityBody:
      if (DataLength != 0) {
        if (Private->FileSize != 0) {
          //
          // We already know the file size, print in percentage format.
          //
          if (Private->ReceivedSize == 0) {
            Print (L"  File Size: %lu Bytes\n", Private->FileSize);
          }

          Private->ReceivedSize += DataLength;
          Percentage             = (UINT32)DivU64x64Remainder (MultU64x32 (Private->ReceivedSize, 100), Private->FileSize, NULL);
          if (Private->Percentage != Percentage) {
            Private->Percentage = Percentage;
            Print (L"\r  Downloading...%d%%", Percentage);
          }
        } else {
          //
          // In some case we couldn't get the file size from the HTTP header, so we
          // just print the downloaded file size.
          //
          Private->ReceivedSize += DataLength;
          Print (L"\r  Downloading...%lu Bytes", Private->ReceivedSize);
        }
      }

      break;

    default:
      break;
  }

  return EFI_SUCCESS;
}

///
/// HTTP Boot Callback Protocol instance
///
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_HTTP_BOOT_CALLBACK_PROTOCOL  gHttpBootDxeHttpBootCallback = {
  HttpBootCallback
};
