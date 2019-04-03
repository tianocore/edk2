/** @file
  Opal Password common header file.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _OPAL_PASSWORD_COMMON_H_
#define _OPAL_PASSWORD_COMMON_H_

#define OPAL_MAX_PASSWORD_SIZE      32

#define OPAL_DEVICE_TYPE_UNKNOWN    0x0
#define OPAL_DEVICE_TYPE_ATA        0x1
#define OPAL_DEVICE_TYPE_NVME       0x2

typedef struct {
  UINT16            Segment;
  UINT8             Bus;
  UINT8             Device;
  UINT8             Function;
  UINT8             Reserved;
} OPAL_PCI_DEVICE;

typedef struct {
  UINT32                      Length;
  OPAL_PCI_DEVICE             Device;
  UINT8                       PasswordLength;
  UINT8                       Password[OPAL_MAX_PASSWORD_SIZE];
  UINT16                      OpalBaseComId;
  UINT32                      DevicePathLength;
  EFI_DEVICE_PATH_PROTOCOL    DevicePath[];
} OPAL_DEVICE_LOCKBOX_DATA;

#define OPAL_DEVICE_LOCKBOX_GUID  { 0x56a77f0d, 0x6f05, 0x4d47, { 0xb9, 0x11, 0x4f, 0xd, 0xec, 0x5c, 0x58, 0x61 } }

#endif // _OPAL_PASSWORD_COMMON_H_
