/** @file
  GUIDs used as HII FormSet and HII Package list GUID in SecureBootConfigDxe driver.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SECUREBOOT_CONFIG_HII_GUID_H__
#define __SECUREBOOT_CONFIG_HII_GUID_H__

#define SECUREBOOT_CONFIG_FORM_SET_GUID \
  { \
    0x5daf50a5, 0xea81, 0x4de2, {0x8f, 0x9b, 0xca, 0xbd, 0xa9, 0xcf, 0x5c, 0x14} \
  }

extern EFI_GUID  gSecureBootConfigFormSetGuid;

#endif
