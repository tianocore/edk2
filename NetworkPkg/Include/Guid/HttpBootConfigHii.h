/** @file
  GUIDs used as HII FormSet and HII Package list GUID in HTTP boot driver.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HTTP_BOOT_HII_GUID_H__
#define __HTTP_BOOT_HII_GUID_H__

#define HTTP_BOOT_CONFIG_GUID \
  { \
    0x4d20583a, 0x7765, 0x4e7a, { 0x8a, 0x67, 0xdc, 0xde, 0x74, 0xee, 0x3e, 0xc5 } \
  }

extern EFI_GUID  gHttpBootConfigGuid;

#endif
