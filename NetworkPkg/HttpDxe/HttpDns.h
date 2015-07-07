/** @file
  The header file of routines for HttpDxe driver to perform DNS resolution based on UEFI DNS protocols.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  IN     HTTP_PROTOCOL            *HttpInstance,
  IN     CHAR16                   *HostName,
     OUT EFI_IPv4_ADDRESS         *IpAddress                
  );

#endif