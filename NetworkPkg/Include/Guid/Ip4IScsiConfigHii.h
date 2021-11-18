/** @file
  GUIDs used as HII FormSet and HII Package list GUID in IP4 IScsiDxe driver.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IP4_ISCSI_CONFIG_HII_GUID_H__
#define __IP4_ISCSI_CONFIG_HII_GUID_H__

#define IP4_ISCSI_CONFIG_GUID \
  { \
    0x6456ed61, 0x3579, 0x41c9, { 0x8a, 0x26, 0x0a, 0x0b, 0xd6, 0x2b, 0x78, 0xfc } \
  }

#define ISCSI_CHAP_AUTH_INFO_GUID \
  { \
    0x786ec0ac, 0x65ae, 0x4d1b, {0xb1, 0x37, 0xd, 0x11, 0xa, 0x48, 0x37, 0x97} \
  }

extern EFI_GUID  gIp4IScsiConfigGuid;
extern EFI_GUID  gIScsiCHAPAuthInfoGuid;

#endif
