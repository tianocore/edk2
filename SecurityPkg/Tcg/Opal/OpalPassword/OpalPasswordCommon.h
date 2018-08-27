/** @file
  Opal Password common header file.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
