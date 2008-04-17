/** @file
  Data structure for DHCP support

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiDhcp.h

Abstract:
  The header file of IScsiDhcp.c

**/

#ifndef _ISCSI_DHCP_H_
#define _ISCSI_DHCP_H_

#include <Protocol/Dhcp4.h>

#define DHCP4_TAG_PARA_LIST             55
#define DHCP4_TAG_NETMASK               1
#define DHCP4_TAG_ROUTER                3
#define DHCP4_TAG_DNS                   6
#define DHCP4_TAG_SERVER_ID             54
#define DHCP4_TAG_ROOT_PATH             17
#define ISCSI_ROOT_PATH_ID              "iscsi:"
#define ISCSI_ROOT_PATH_FIELD_DELIMITER ':'

enum {
  RP_FIELD_IDX_SERVERNAME = 0,
  RP_FIELD_IDX_PROTOCOL,
  RP_FIELD_IDX_PORT,
  RP_FIELD_IDX_LUN,
  RP_FIELD_IDX_TARGETNAME,
  RP_FIELD_IDX_MAX
};

typedef struct _ISCSI_ROOT_PATH_FIELD {
  CHAR8 *Str;
  UINT8 Len;
} ISCSI_ROOT_PATH_FIELD;

/**
  Parse the DHCP ACK to get the address configuration and DNS information.
  
  @param  Image[in]             The handle of the driver image.

  @param  Controller[in]        The handle of the controller;

  @param  ConfigData[in]        The session configuration data.

  @retval EFI_SUCCESS           The DNS information is got from the DHCP ACK.

  @retval EFI_NO_MAPPING        DHCP failed to acquire address and other information.

  @retval EFI_INVALID_PARAMETER The DHCP ACK's DNS option is mal-formatted.

  @retval EFI_DEVICE_ERROR      Some unexpected error happened.

**/
EFI_STATUS
IScsiDoDhcp (
  IN EFI_HANDLE                 Image,
  IN EFI_HANDLE                 Controller,
  IN ISCSI_SESSION_CONFIG_DATA  *ConfigData
  );

#endif
