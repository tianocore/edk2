/** @file
  The header file of routines for HttpDxe driver to perform DNS resolution based on UEFI DNS protocols.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_HTTP_DNS_H__
#define __EFI_HTTP_DNS_H__

/**
  Retrieve the host address using the EFI_DNS4_PROTOCOL.

  @param[in]  HttpInstance        Pointer to HTTP_PROTOCOL instance.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv4 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
HttpDns4 (
  IN     HTTP_PROTOCOL  *HttpInstance,
  IN     CHAR16         *HostName,
  OUT EFI_IPv4_ADDRESS  *IpAddress
  );

/**
  Retrieve the host address using the EFI_DNS6_PROTOCOL.

  @param[in]  HttpInstance        Pointer to HTTP_PROTOCOL instance.
  @param[in]  HostName            Pointer to buffer containing hostname.
  @param[out] IpAddress           On output, pointer to buffer containing IPv6 address.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
HttpDns6 (
  IN     HTTP_PROTOCOL  *HttpInstance,
  IN     CHAR16         *HostName,
  OUT EFI_IPv6_ADDRESS  *IpAddress
  );

#endif
