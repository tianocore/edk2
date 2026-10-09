/** @file
  Legacy Boot Maintenance UI definition.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_LEGACY_BOOT_OPTION_VFR_H_
#define _EFI_LEGACY_BOOT_OPTION_VFR_H_

#include <Guid/HiiBootMaintenanceFormset.h>

#define MAX_MENU_NUMBER  100

#define LEGACY_BOOT_OPTION_FORMSET_GUID  { 0x6bc75598, 0x89b4, 0x483d, { 0x91, 0x60, 0x7f, 0x46, 0x9a, 0x96, 0x35, 0x31 } }

#define VARSTORE_ID_LEGACY_BOOT  0x0001

#define LEGACY_BOOT_FORM_ID          0x1000
#define LEGACY_ORDER_CHANGE_FORM_ID  0x1001

#define FORM_FLOPPY_BOOT_ID    0x2000
#define FORM_HARDDISK_BOOT_ID  0x2001
#define FORM_CDROM_BOOT_ID     0x2002
#define FORM_NET_BOOT_ID       0x2003
#define FORM_BEV_BOOT_ID       0x2004

#define FORM_BOOT_LEGACY_DEVICE_ID  0x9000
#define FORM_BOOT_LEGACY_LABEL_END  0x9001

#pragma pack(1)

///
/// This is the structure that will be used to store the
/// question's current value. Use it at initialize time to
/// set default value for each question. When using at run
/// time, this map is returned by the callback function,
/// so dynamically changing the question's value will be
/// possible through this mechanism
///
typedef struct {
  //
  // Legacy Device Order Selection Storage
  //
  UINT16    LegacyFD[MAX_MENU_NUMBER];
  UINT16    LegacyHD[MAX_MENU_NUMBER];
  UINT16    LegacyCD[MAX_MENU_NUMBER];
  UINT16    LegacyNET[MAX_MENU_NUMBER];
  UINT16    LegacyBEV[MAX_MENU_NUMBER];
} LEGACY_BOOT_NV_DATA;

///
/// This is the structure that will be used to store the
/// question's current value. Use it at initialize time to
/// set default value for each question. When using at run
/// time, this map is returned by the callback function,
/// so dynamically changing the question's value will be
/// possible through this mechanism
///
typedef struct {
  //
  // Legacy Device Order Selection Storage
  //
  LEGACY_BOOT_NV_DATA    InitialNvData;
  LEGACY_BOOT_NV_DATA    CurrentNvData;
  LEGACY_BOOT_NV_DATA    LastTimeNvData;
  UINT8                  DisableMap[32];
} LEGACY_BOOT_MAINTAIN_DATA;

#pragma pack()

#endif
