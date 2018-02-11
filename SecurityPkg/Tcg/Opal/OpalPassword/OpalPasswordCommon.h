/** @file
  Opal Password common header file.

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
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
  UINT16            Length;
  OPAL_PCI_DEVICE   Device;
  UINT8             PasswordLength;
  UINT8             Password[OPAL_MAX_PASSWORD_SIZE];
  UINT16            OpalBaseComId;
  UINT32            BarAddr;
} OPAL_DEVICE_COMMON;

#define OPAL_DEVICE_ATA_GUID { 0xcb934fe1, 0xb8cd, 0x46b1, { 0xa0, 0x58, 0xdd, 0xcb, 0x7, 0xb7, 0xb4, 0x17 } }

typedef struct {
  UINT16            Length;
  OPAL_PCI_DEVICE   Device;
  UINT8             PasswordLength;
  UINT8             Password[OPAL_MAX_PASSWORD_SIZE];
  UINT16            OpalBaseComId;
  UINT32            BarAddr;
  UINT16            Port;
  UINT16            PortMultiplierPort;
} OPAL_DEVICE_ATA;

#define OPAL_DEVICE_NVME_GUID { 0xde116925, 0xaf7f, 0x42d9, { 0x83, 0xc0, 0x7e, 0xd6, 0x26, 0x59, 0x0, 0xfb } }

typedef struct {
  UINT16            Length;
  OPAL_PCI_DEVICE   Device;
  UINT8             PasswordLength;
  UINT8             Password[OPAL_MAX_PASSWORD_SIZE];
  UINT16            OpalBaseComId;
  UINT32            BarAddr;
  UINT32            NvmeNamespaceId;
  OPAL_PCI_DEVICE   PciBridgeNode[0];
} OPAL_DEVICE_NVME;

#endif // _OPAL_PASSWORD_COMMON_H_
