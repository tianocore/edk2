/** @file
  Support functions declaration for UEFI HTTP boot driver.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2020 Hewlett-Packard Development Company, L.P.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_HTTP_BOOT_SUPPORT_H__
#define __EFI_HTTP_BOOT_SUPPORT_H__

/**
  Get the Nic handle using any child handle in the IPv4 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv4.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp4Children (
  IN EFI_HANDLE                 ControllerHandle
  );

/**
  Get the Nic handle using any child handle in the IPv6 stack.

  @param[in]  ControllerHandle    Pointer to child handle over IPv6.

  @return NicHandle               The pointer to the Nic handle.
  @return NULL                    Can't find the Nic handle.

**/
EFI_HANDLE
HttpBootGetNicByIp6Children (
  IN EFI_HANDLE                 ControllerHandle
  );

/**
  This function is to convert UINTN to ASCII string with the required formatting.

  @param[in]  Number         Numeric value to be converted.
  @param[in]  Buffer         The pointer to the buffer for ASCII string.
  @param[in]  Length         The length of the required format.

**/
VOID
HttpBootUintnToAscDecWithFormat (
  IN UINTN                       Number,
  IN UINT8                       *Buffer,
  IN INTN                        Length
  );


/**
  This function is to display the IPv4 address.

  @param[in]  Ip        The pointer to the IPv4 address.

**/
VOID
HttpBootShowIp4Addr (
  IN EFI_IPv4_ADDRESS   *Ip
  );

/**
  This function is to display the IPv6 address.

  @param[in]  Ip        The pointer to the IPv6 address.

**/
VOID
HttpBootShowIp6Addr (
  IN EFI_IPv6_ADDRESS   *Ip
  );

/**
  This function is to display the HTTP error status.

  @param[in]      StatusCode      The status code value in HTTP message.

**/
VOID
HttpBootPrintErrorMessage (
  EFI_HTTP_STATUS_CODE            StatusCode
  );

/**
  Retrieve the host address using the EFI_DNS6_PROTOCOL.

  @param[in]  Private             The pointer to the driver's private data.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv6 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.
**/
EFI_STATUS
HttpBootDns (
  IN     HTTP_BOOT_PRIVATE_DATA   *Private,
  IN     CHAR16                   *HostName,
     OUT EFI_IPv6_ADDRESS         *IpAddress
  );

/**
  Notify the callback function when an event is triggered.

  @param[in]  Event           The triggered event.
  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
HttpBootCommonNotify (
  IN EFI_EVENT           Event,
  IN VOID                *Context
  );

/**
  This function checks the HTTP(S) URI scheme.

  @param[in]    Uri              The pointer to the URI string.

  @retval EFI_SUCCESS            The URI scheme is valid.
  @retval EFI_INVALID_PARAMETER  The URI scheme is not HTTP or HTTPS.
  @retval EFI_ACCESS_DENIED      HTTP is disabled and the URI is HTTP.

**/
EFI_STATUS
HttpBootCheckUriScheme (
  IN      CHAR8                  *Uri
  );

/**
  Get the URI address string from the input device path.

  Caller need to free the buffer in the UriAddress pointer.

  @param[in]   FilePath         Pointer to the device path which contains a URI device path node.
  @param[out]  UriAddress       The URI address string extract from the device path.

  @retval EFI_SUCCESS            The URI string is returned.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.

**/
EFI_STATUS
HttpBootParseFilePath (
  IN     EFI_DEVICE_PATH_PROTOCOL     *FilePath,
     OUT CHAR8                        **UriAddress
  );

/**
  This function returns the image type according to server replied HTTP message
  and also the image's URI info.

  @param[in]    Uri              The pointer to the image's URI string.
  @param[in]    UriParser        URI Parse result returned by NetHttpParseUrl().
  @param[in]    HeaderCount      Number of HTTP header structures in Headers list.
  @param[in]    Headers          Array containing list of HTTP headers.
  @param[out]   ImageType        The image type of the downloaded file.

  @retval EFI_SUCCESS            The image type is returned in ImageType.
  @retval EFI_INVALID_PARAMETER  ImageType, Uri or UriParser is NULL.
  @retval EFI_INVALID_PARAMETER  HeaderCount is not zero, and Headers is NULL.
  @retval EFI_NOT_FOUND          Failed to identify the image type.
  @retval Others                 Unexpected error happened.

**/
EFI_STATUS
HttpBootCheckImageType (
  IN      CHAR8                  *Uri,
  IN      VOID                   *UriParser,
  IN      UINTN                  HeaderCount,
  IN      EFI_HTTP_HEADER        *Headers,
     OUT  HTTP_BOOT_IMAGE_TYPE   *ImageType
  );

/**
  This function register the RAM disk info to the system.

  @param[in]       Private         The pointer to the driver's private data.
  @param[in]       BufferSize      The size of Buffer in bytes.
  @param[in]       Buffer          The base address of the RAM disk.
  @param[in]       ImageType       The image type of the file in Buffer.

  @retval EFI_SUCCESS              The RAM disk has been registered.
  @retval EFI_NOT_FOUND            No RAM disk protocol instances were found.
  @retval EFI_UNSUPPORTED          The ImageType is not supported.
  @retval Others                   Unexpected error happened.

**/
EFI_STATUS
HttpBootRegisterRamDisk (
  IN  HTTP_BOOT_PRIVATE_DATA       *Private,
  IN  UINTN                        BufferSize,
  IN  VOID                         *Buffer,
  IN  HTTP_BOOT_IMAGE_TYPE         ImageType
  );

/**
  Indicate if the HTTP status code indicates a redirection.

  @param[in]  StatusCode      HTTP status code from server.

  @return                     TRUE if it's redirection.

**/
BOOLEAN
HttpBootIsHttpRedirectStatusCode (
  IN   EFI_HTTP_STATUS_CODE        StatusCode
  );
#endif
