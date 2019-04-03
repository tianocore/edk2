/** @file
This driver parses the mMiscSubclassDataTable structure and reports
any generated data to smbios.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


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
