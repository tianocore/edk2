/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data to smbios.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/


#include "CommonHeader.h"

#include "SmbiosMisc.h"


//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE, MiscOnboardDeviceVideo) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_VIDEO),    // OnBoardDeviceDescription
  {                                               // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeVideo,                    // DeviceType
    1,                                            // DeviceEnabled
    0                                             // Reserved
  },
  {0}                                               // OnBoardDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE, MiscOnboardDeviceNetwork) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_NETWORK),  // OnBoardDeviceDescription
  {                                               // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeEthernet,                 // DeviceType
    1,                                            // DeviceEnabled
    0                                             // Reserved
  },
  {0}                                               // OnBoardDevicePath
};

MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE, MiscOnboardDeviceAudio) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_AUDIO),    // OnBoardDeviceDescription
  {                                               // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeSound,                    // DeviceType
    1,                                            // DeviceEnabled
    0                                             // Reserved
  },
  DP_END                                          // OnBoardDevicePath
};
