/** @file
  GUIDs used as HII FormSet and HII Package list GUID in BdsDxe driver.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BDS_HII_GUIDS_H__
#define __BDS_HII_GUIDS_H__

#define FRONT_PAGE_FORMSET_GUID \
  { \
    0x9e0c30bc, 0x3f06, 0x4ba6, {0x82, 0x88, 0x9, 0x17, 0x9b, 0x85, 0x5d, 0xbe} \
  }

#define BOOT_MANAGER_FORMSET_GUID \
  { \
  0x847bc3fe, 0xb974, 0x446d, {0x94, 0x49, 0x5a, 0xd5, 0x41, 0x2e, 0x99, 0x3b} \
  }

#define DEVICE_MANAGER_FORMSET_GUID  \
  { \
  0x3ebfa8e6, 0x511d, 0x4b5b, {0xa9, 0x5f, 0xfb, 0x38, 0x26, 0xf, 0x1c, 0x27} \
  }

#define DRIVER_HEALTH_FORMSET_GUID  \
  { \
  0xf76e0a70, 0xb5ed, 0x4c38, {0xac, 0x9a, 0xe5, 0xf5, 0x4b, 0xf1, 0x6e, 0x34} \
  }

#define BOOT_MAINT_FORMSET_GUID \
  { \
  0x642237c7, 0x35d4, 0x472d, {0x83, 0x65, 0x12, 0xe0, 0xcc, 0xf2, 0x7a, 0x22} \
  }

#define FILE_EXPLORE_FORMSET_GUID \
  { \
  0x1f2d63e1, 0xfebd, 0x4dc7, {0x9c, 0xc5, 0xba, 0x2b, 0x1c, 0xef, 0x9c, 0x5b} \
  }

extern EFI_GUID gFrontPageFormSetGuid;
extern EFI_GUID gBootMaintFormSetGuid;
extern EFI_GUID gFileExploreFormSetGuid;
extern EFI_GUID gBootManagerFormSetGuid;
extern EFI_GUID gDeviceManagerFormSetGuid;
extern EFI_GUID gDriverHealthFormSetGuid;

#endif
