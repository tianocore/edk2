/**@file
  Subsystem IDs setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
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
