/** @file
  GUIDs used as HII FormSet and HII Package list GUID in Driver Sample driver.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DRIVER_SAMPLE_HII_GUID_H__
#define __DRIVER_SAMPLE_HII_GUID_H__

#define DRIVER_SAMPLE_FORMSET_GUID \
  { \
    0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} \
  }

#define DRIVER_SAMPLE_INVENTORY_GUID \
  { \
    0xb3f56470, 0x6141, 0x4621, {0x8f, 0x19, 0x70, 0x4e, 0x57, 0x7a, 0xa9, 0xe8} \
  }

#define EFI_IFR_REFRESH_ID_OP_GUID \
  { \
    0xF5E655D9, 0x02A6, 0x46f2, {0x9E, 0x76, 0xB8, 0xBE, 0x8E, 0x60, 0xAB, 0x22} \
  }

extern EFI_GUID gDriverSampleFormSetGuid;
extern EFI_GUID gDriverSampleInventoryGuid;
extern EFI_GUID gEfiIfrRefreshIdOpGuid;

#endif
