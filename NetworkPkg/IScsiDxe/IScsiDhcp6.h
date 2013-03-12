/** @file
  The header file of iSCSI DHCP6 related configuration routines.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_DHCP6_H_
#define _ISCSI_DHCP6_H_

#define DHCP6_OPT_REQUEST_OPTION 6
#define DHCP6_OPT_VENDOR_INFO    17
#define DHCP6_OPT_DNS_SERVERS    23
///
/// Assigned by IANA, RFC 5970
///
#define DHCP6_OPT_BOOT_FILE_URL  59
#define DHCP6_OPT_BOOT_FILE_PARA 60

#define ISCSI_ROOT_PATH_ID                   "iscsi:"
#define ISCSI_ROOT_PATH_FIELD_DELIMITER      ':'
#define ISCSI_ROOT_PATH_ADDR_START_DELIMITER '['
#define ISCSI_ROOT_PATH_ADDR_END_DELIMITER   ']'


/**
  Extract the Root Path option and get the required target information from
  Boot File Uniform Resource Locator (URL) Option.

  @param[in]       RootPath      The RootPath string.
  @param[in]       Length        Length of the RootPath option payload.
  @param[in, out]  ConfigData    The iSCSI session configuration data read from
                                 nonvolatile device.

  @retval EFI_SUCCESS            All required information is extracted from the
                                 RootPath option.
  @retval EFI_NOT_FOUND          The RootPath is not an iSCSI RootPath.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER  The RootPath is malformatted.

**/
EFI_STATUS
IScsiDhcp6ExtractRootPath (
  IN     CHAR8                        *RootPath,
  IN     UINT16                       Length,
  IN OUT ISCSI_ATTEMPT_CONFIG_NVDATA *ConfigData
  );

/**
  Parse the DHCP ACK to get the address configuration and DNS information.

  @param[in]       Image         The handle of the driver image.
  @param[in]       Controller    The handle of the controller;
  @param[in, out]  ConfigData    The attempt configuration data.

  @retval EFI_SUCCESS            The DNS information is got from the DHCP ACK.
  @retval EFI_NO_MAPPING         DHCP failed to acquire address and other
                                 information.
  @retval EFI_INVALID_PARAMETER  The DHCP ACK's DNS option is malformatted.
  @retval EFI_DEVICE_ERROR       Some unexpected error happened.
  @retval EFI_OUT_OF_RESOURCES   There is no sufficient resource to finish the
                                 operation.
  @retval EFI_NO_MEDIA           There was a media error.

**/
EFI_STATUS
IScsiDoDhcp6 (
  IN     EFI_HANDLE                  Image,
  IN     EFI_HANDLE                  Controller,
  IN OUT ISCSI_ATTEMPT_CONFIG_NVDATA *ConfigData
  );

#endif
