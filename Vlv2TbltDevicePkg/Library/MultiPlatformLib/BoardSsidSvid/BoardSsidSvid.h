/**@file
  Subsystem IDs setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   
**/

#include <Guid/PlatformInfo.h>
#include <Library/BaseMemoryLib.h>

//
// Default Vendor ID and Subsystem ID
//
#define SUBSYSTEM_VENDOR_ID1   0x8086
#define SUBSYSTEM_DEVICE_ID1   0x1999
#define SUBSYSTEM_SVID_SSID1   (SUBSYSTEM_VENDOR_ID1 + (SUBSYSTEM_DEVICE_ID1 << 16))

#define SUBSYSTEM_VENDOR_ID2   0x8086
#define SUBSYSTEM_DEVICE_ID2   0x1888
#define SUBSYSTEM_SVID_SSID2   (SUBSYSTEM_VENDOR_ID2 + (SUBSYSTEM_DEVICE_ID2 << 16))

#define SUBSYSTEM_VENDOR_ID   0x8086
#define SUBSYSTEM_DEVICE_ID   0x1234
#define SUBSYSTEM_SVID_SSID   (SUBSYSTEM_VENDOR_ID + (SUBSYSTEM_DEVICE_ID << 16))

EFI_STATUS
InitializeBoardSsidSvid (
    IN CONST EFI_PEI_SERVICES       **PeiServices,
    IN EFI_PLATFORM_INFO_HOB        *PlatformInfoHob
  );
