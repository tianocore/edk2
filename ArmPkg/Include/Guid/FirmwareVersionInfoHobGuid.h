/** @file
*
*  Copyright (c) 2020, NUVIA Inc. All rights reserved.
*  Copyright (c) 2016, Hisilicon Limited. All rights reserved.
*  Copyright (c) 2016, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef FIRMWARE_VERSION_INFO_HOB_GUID_H_
#define FIRMWARE_VERSION_INFO_HOB_GUID_H_

// {78ba5a73-04d0-4adf-b614-04a6a3d8a257}
#define FIRMWARE_VERSION_INFO_HOB_GUID \
  {0x78ba5a73, 0x04d0, 0x4adf, {0xb6, 0x14, 0x04, 0xa6, 0xa3, 0xd8, 0xa2, 0x57}}

extern GUID gFirmwareVersionInfoHobGuid;

#pragma pack(1)

typedef struct {
  EFI_TIME BuildTime;
  CHAR16   String[1];
} FIRMWARE_VERSION_INFO;

#pragma pack()

#endif /* FIRMWARE_VERSION_INFO_HOB_GUID_H_ */
