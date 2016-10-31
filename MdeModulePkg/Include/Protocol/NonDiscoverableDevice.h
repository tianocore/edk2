/** @file
  Protocol to describe devices that are not on a discoverable bus

  Copyright (c) 2016, Linaro, Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __NON_DISCOVERABLE_DEVICE_H__
#define __NON_DISCOVERABLE_DEVICE_H__

#include <IndustryStandard/Acpi.h>

#define EDKII_NON_DISCOVERABLE_DEVICE_PROTOCOL_GUID \
  { 0x0d51905b, 0xb77e, 0x452a, {0xa2, 0xc0, 0xec, 0xa0, 0xcc, 0x8d, 0x51, 0x4a } }

//
// Protocol interface structure
//
typedef struct _NON_DISCOVERABLE_DEVICE NON_DISCOVERABLE_DEVICE;

//
// Data Types
//
typedef enum {
  NonDiscoverableDeviceDmaTypeCoherent,
  NonDiscoverableDeviceDmaTypeNonCoherent,
  NonDiscoverableDeviceDmaTypeMax,
} NON_DISCOVERABLE_DEVICE_DMA_TYPE;

//
// Function Prototypes
//

/**
  Perform device specific initialization before the device is started

  @param  This          The non-discoverable device protocol pointer

  @retval EFI_SUCCESS   Initialization successful, the device may be used
  @retval Other         Initialization failed, device should not be started
**/
typedef
EFI_STATUS
(EFIAPI *NON_DISCOVERABLE_DEVICE_INIT) (
  IN  NON_DISCOVERABLE_DEVICE       *This
  );

struct _NON_DISCOVERABLE_DEVICE {
  //
  // The type of device
  //
  CONST EFI_GUID                      *Type;
  //
  // Whether this device is DMA coherent
  //
  NON_DISCOVERABLE_DEVICE_DMA_TYPE    DmaType;
  //
  // Initialization function for the device
  //
  NON_DISCOVERABLE_DEVICE_INIT        Initialize;
  //
  // The MMIO and I/O regions owned by the device
  //
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Resources;
};

extern EFI_GUID gEdkiiNonDiscoverableDeviceProtocolGuid;

#endif
