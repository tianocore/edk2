/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  LoadedImageDevicePath.h

Abstract:

  The Loaded Image Device Path Protocol as defined in UEFI 2.1.

  When installed, the Loaded Image Device Path Protocol specifies the device
  path that was used when a PE/COFF image was loaded through the EFI Boot
  Service LoadImage().


--*/

#ifndef _LOADED_IMAGE_DEVICE_PATH_H_
#define _LOADED_IMAGE_DEVICE_PATH_H_

#include EFI_PROTOCOL_DEFINITION (DevicePath)

//
// Loaded Image Device Path protocol
//
#define EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL_GUID \
  { \
    0xbc62157e, 0x3e33, 0x4fec, {0x99, 0x20, 0x2d, 0x3b, 0x36, 0xd7, 0x50, 0xdf} \
  }

typedef EFI_DEVICE_PATH_PROTOCOL EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL;

extern EFI_GUID gEfiLoadedImageDevicePathProtocolGuid;

#endif
