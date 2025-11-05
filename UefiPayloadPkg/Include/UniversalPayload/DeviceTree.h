/** @file
  This file defines the structure for the PCI Root Bridges.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - Universal Payload Specification 0.8 (https://universalpayload.github.io/spec/)
**/

#ifndef UNIVERSAL_PAYLOAD_DEVICE_TREE_H_
#define UNIVERSAL_PAYLOAD_DEVICE_TREE_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  EFI_PHYSICAL_ADDRESS                DeviceTreeAddress;
} UNIVERSAL_PAYLOAD_DEVICE_TREE;

#pragma pack()

#define UNIVERSAL_PAYLOAD_DEVICE_TREE_REVISION  1

extern GUID  gUniversalPayloadDeviceTreeGuid;

#endif // UNIVERSAL_PAYLOAD_SMBIOS_TABLE_H_
