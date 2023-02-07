/** @file
  Header file for NV data structure definition.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RAM_DISK_NVDATA_H_
#define _RAM_DISK_NVDATA_H_

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/RamDiskHii.h>

#define MAIN_FORM_ID                     0x1000
#define MAIN_GOTO_FILE_EXPLORER_ID       0x1001
#define MAIN_REMOVE_RD_QUESTION_ID       0x1002
#define MAIN_LABEL_LIST_START            0x1003
#define MAIN_LABEL_LIST_END              0x1004
#define MAIN_CHECKBOX_QUESTION_ID_START  0x1100

#define CREATE_RAW_RAM_DISK_FORM_ID         0x2000
#define CREATE_RAW_SIZE_QUESTION_ID         0x2001
#define CREATE_RAW_SUBMIT_QUESTION_ID       0x2002
#define CREATE_RAW_DISCARD_QUESTION_ID      0x2003
#define CREATE_RAW_MEMORY_TYPE_QUESTION_ID  0x2004

#define RAM_DISK_BOOT_SERVICE_DATA_MEMORY  0x00
#define RAM_DISK_RESERVED_MEMORY           0x01
#define RAM_DISK_MEMORY_TYPE_MAX           0x02

typedef struct {
  //
  // The size of the RAM disk to be created.
  //
  UINT64    Size;
  //
  // Selected RAM Disk Memory Type
  //
  UINT8     MemType;
} RAM_DISK_CONFIGURATION;

#endif
/** @file
  GUID indicates that the form set contains forms designed to be used
  for platform configuration and this form set will be displayed.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  GUID defined in UEFI 2.1.

**/

#ifndef __HII_PLATFORM_SETUP_FORMSET_GUID_H__
#define __HII_PLATFORM_SETUP_FORMSET_GUID_H__

#define EFI_HII_PLATFORM_SETUP_FORMSET_GUID \
  { 0x93039971, 0x8545, 0x4b04, { 0xb4, 0x5e, 0x32, 0xeb, 0x83, 0x26, 0x4, 0xe } }

#define EFI_HII_DRIVER_HEALTH_FORMSET_GUID \
  { 0xf22fc20c, 0x8cf4, 0x45eb, { 0x8e, 0x6, 0xad, 0x4e, 0x50, 0xb9, 0x5d, 0xd3 } }

#define EFI_HII_USER_CREDENTIAL_FORMSET_GUID \
  { 0x337f4407, 0x5aee, 0x4b83, { 0xb2, 0xa7, 0x4e, 0xad, 0xca, 0x30, 0x88, 0xcd } }

#define EFI_HII_REST_STYLE_FORMSET_GUID \
  { 0x790217bd, 0xbecf, 0x485b, { 0x91, 0x70, 0x5f, 0xf7, 0x11, 0x31, 0x8b, 0x27 } }

extern EFI_GUID  gEfiHiiPlatformSetupFormsetGuid;
extern EFI_GUID  gEfiHiiDriverHealthFormsetGuid;
extern EFI_GUID  gEfiHiiUserCredentialFormsetGuid;
extern EFI_GUID  gEfiHiiRestStyleFormsetGuid;

#endif
/** @file
  GUIDs used as HII FormSet and HII Package list GUID in RamDiskDxe driver.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __RAM_DISK_HII_GUID_H__
#define __RAM_DISK_HII_GUID_H__

#define RAM_DISK_FORM_SET_GUID \
  { \
    0x2a46715f, 0x3581, 0x4a55, {0x8e, 0x73, 0x2b, 0x76, 0x9a, 0xaa, 0x30, 0xc5} \
  }

extern EFI_GUID  gRamDiskFormSetGuid;

#endif
