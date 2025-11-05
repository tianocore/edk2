/** @file
 The header file of routines for IScsi driver to perform DNS
 resolution based on UEFI DNS protocols.

Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_DNS_H_
#define _ISCSI_DNS_H_

/**
  Retrieve the host address using the EFI_DNS4_PROTOCOL.

  @param[in]  Image               The handle of the driver image.
  @param[in]  Controller          The handle of the controller.
  @param[in, out]  NvData         The Session config data structure.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
IScsiDns4 (
  IN     EFI_HANDLE                   Image,
  IN     EFI_HANDLE                   Controller,
  IN OUT ISCSI_SESSION_CONFIG_NVDATA  *NvData
  );

/**
  Retrieve the host address using the EFI_DNS6_PROTOCOL.

  @param[in]  Image               The handle of the driver image.
  @param[in]  Controller          The handle of the controller.
  @param[in, out]  NvData         The Session config data structure.

  @retval EFI_SUCCESS             Operation succeeded.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate needed resources.
  @retval EFI_DEVICE_ERROR        An unexpected network error occurred.
  @retval Others                  Other errors as indicated.

**/
EFI_STATUS
IScsiDns6 (
  IN     EFI_HANDLE                   Image,
  IN     EFI_HANDLE                   Controller,
  IN OUT ISCSI_SESSION_CONFIG_NVDATA  *NvData
  );

#endif
