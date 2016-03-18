/** @file
GUIDs used for System Non Volatile HOB entries in the in the HOB list and FV Guids carrying
the System specific information.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SYSTEM_NV_DATA_HOB_GUID_H_
#define _SYSTEM_NV_DATA_HOB_GUID_H_

#define EFI_SYSTEM_NV_DATA_HOB_GUID \
  {0xd6e5092d, 0xc7b2, 0x4872, {0xaf, 0x66, 0xfd, 0xc0, 0xe6, 0xf9, 0x5e, 0x78}}

typedef struct {
  EFI_GUID                  SystemNvDataHobGuid;
  EFI_GUID                  SystemNvDataFvGuid;
  EFI_LBA                   StartLba;
  UINTN                     StartLbaOffset;
  EFI_LBA                   EndLba;
  UINTN                     EndLbaOffset;
  UINT32                    DataTypeSignature;
} NV_SYSTEM_DATA_GUID_TYPE;

extern EFI_GUID gEfiSystemNvDataHobGuid;

#endif
