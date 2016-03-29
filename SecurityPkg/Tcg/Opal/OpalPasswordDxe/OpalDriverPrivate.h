/** @file
  Private structures and functions used within OPAL_DRIVER

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _OPAL_DRIVER_PRIVATE_H_
#define _OPAL_DRIVER_PRIVATE_H_
#include "OpalDriver.h"

#define OPAL_MSID_LENGHT    128

#pragma pack(1)
//
// Structure that is used to represent an OPAL_DISK.
//
typedef struct {
  UINT32                                          MsidLength;             // Byte length of MSID Pin for device
  UINT8                                           Msid[OPAL_MSID_LENGHT]; // MSID Pin for device
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL           *Sscp;
  UINT32                                          MediaId;                // MediaId is used by Ssc Protocol.
  EFI_DEVICE_PATH_PROTOCOL                        *OpalDevicePath;
  UINT16                                          OpalBaseComId;          // Opal SSC 1 base com id.
  OPAL_OWNER_SHIP                                 Owner;
  OPAL_DISK_SUPPORT_ATTRIBUTE                     SupportedAttributes;
  TCG_LOCKING_FEATURE_DESCRIPTOR                  LockingFeature;         // Locking Feature Descriptor retrieved from performing a Level 0 Discovery
} OPAL_DISK;

//
// Device with block IO protocol
//
typedef struct _OPAL_DRIVER_DEVICE OPAL_DRIVER_DEVICE;

struct _OPAL_DRIVER_DEVICE {
  OPAL_DRIVER_DEVICE                              *Next;              ///< Linked list pointer
  EFI_HANDLE                                      Handle;             ///< Device handle
  OPAL_DISK                                       OpalDisk;           ///< User context
  CHAR16                                          *Name16;            ///< Allocated/freed by UEFI Filter Driver at device creation/removal
  CHAR8                                           *NameZ;             ///< Allocated/freed by UEFI Filter Driver at device creation/removal
  UINT32                                          MediaId;            ///< Required parameter for EFI_STORAGE_SECURITY_COMMAND_PROTOCOL, from BLOCK_IO_MEDIA

  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL           *Sscp;              /// Device protocols consumed
  EFI_DEVICE_PATH_PROTOCOL                        *OpalDevicePath;
};

//
// Opal Driver UEFI Driver Model
//
typedef struct {
  EFI_HANDLE           Handle;              ///< Driver image handle
  OPAL_DRIVER_DEVICE   *DeviceList;         ///< Linked list of controllers owned by this Driver
} OPAL_DRIVER;
#pragma pack()

//
// Retrieves a OPAL_DRIVER_DEVICE based on the pointer to its StorageSecurity protocol.
//
#define DRIVER_DEVICE_FROM_OPALDISK(OpalDiskPointer) (OPAL_DRIVER_DEVICE*)(BASE_CR(OpalDiskPointer, OPAL_DRIVER_DEVICE, OpalDisk))

/**
  Get devcie list info.

  @retval     return the device list pointer.
**/
OPAL_DRIVER_DEVICE*
OpalDriverGetDeviceList(
  VOID
  );

/**
  Get devcie name through the component name protocol.

  @param[in]       Dev                The device which need to get name.

  @retval     TRUE        Find the name for this device.
  @retval     FALSE       Not found the name for this device.
**/
BOOLEAN
OpalDriverGetDriverDeviceName(
  OPAL_DRIVER_DEVICE          *Dev
  );

/**
  Get current device count.

  @retval  return the current created device count.

**/
UINT8
GetDeviceCount (
  VOID
  );

#endif // _OPAL_DRIVER_P_H_
