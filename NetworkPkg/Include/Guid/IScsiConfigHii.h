/** @file
  GUIDs used as HII FormSet and HII Package list GUID in IScsiConfig driver
  that supports IP4 and IP6 both.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __ISCSI_CONFIG_HII_GUID_H__
#define __ISCSI_CONFIG_HII_GUID_H__

#define ISCSI_CONFIG_GUID \
  { \
    0x4b47d616, 0xa8d6, 0x4552, { 0x9d, 0x44, 0xcc, 0xad, 0x2e, 0xf, 0x4c, 0xf9 } \
  }

extern EFI_GUID gIScsiConfigGuid;

#endif
