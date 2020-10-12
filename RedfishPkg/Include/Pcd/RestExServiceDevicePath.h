/** @file
  This library defines the UEFI device path data of network device for REST
  service to decide which should be used as the Redfish host interface.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REST_EX_SERVICE_DEVICE_PATH_H_
#define REST_EX_SERVICE_DEVICE_PATH_H_

#include <Protocol/DevicePath.h>

typedef enum {
  DEVICE_PATH_MATCH_MAC_NODE = 1,
  DEVICE_PATH_MATCH_PCI_NODE = 2,
  DEVICE_PATH_MATCH_MODE_MAX
} DEVICE_PATH_MATCH_MODE;

typedef struct {
  UINT32 DevicePathMatchMode;
  UINT32 DevicePathNum;
  //
  // Example:
  //   {DEVICE_PATH("PciRoot(0)/Pci(0,0)/MAC(005056C00002,0x1)")}
  // DevicePath will be parsed as below:
  //   {0x02,0x01,0x0c,0x00,0xd0,0x41,0x03,0x0a,0x00,0x00,0x00,0x00,
  //    0x01,0x01,0x06,0x00,0x00,0x00,
  //    0x03,0x0b,0x25,0x00,0x00,0x50,0x56,0xc0,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,
  //    0x7f,0xff,0x04,0x00}
  //
  EFI_DEVICE_PATH_PROTOCOL      DevicePath[];
} REST_EX_SERVICE_DEVICE_PATH_DATA;

#endif
