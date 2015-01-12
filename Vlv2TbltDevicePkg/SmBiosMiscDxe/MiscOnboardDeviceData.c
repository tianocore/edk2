/** @file

Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  MiscOnboardDeviceData.c

Abstract:

  Static data of Onboard device information .
  The onboard device information is Misc. subclass type 8 and SMBIOS type 10.


**/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

//
// Static (possibly build generated) Bios Vendor data.
//
MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE_DATA, MiscOnboardDeviceVideo) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_VIDEO),  // OnBoardDeviceDescription
  {                             // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeVideo,  // DeviceType
    1,                          // DeviceEnabled
    0                           // Reserved
  },
  0                             // OnBoardDevicePath
};
MISC_SMBIOS_TABLE_DATA(EFI_MISC_ONBOARD_DEVICE_DATA, MiscOnboardDeviceAudio) = {
  STRING_TOKEN(STR_MISC_ONBOARD_DEVICE_AUDIO),    // OnBoardDeviceDescription
  {                                 // OnBoardDeviceStatus
    EfiOnBoardDeviceTypeSound,      // DeviceType
    1,                              // DeviceEnabled
    0                               // Reserved
  },
  0                                 // OnBoardDevicePath
};

