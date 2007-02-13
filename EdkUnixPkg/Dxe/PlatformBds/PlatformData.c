/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  PlatformData.c

Abstract:
  
  Defined the platform specific device path which will be used by
  platform Bbd to perform the platform policy connect.

--*/

#include "BdsPlatform.h"

//
// Predefined platform default time out value
//
UINT16                      gPlatformBootTimeOutDefault = 10;

//
// Platform specific keyboard device path
//
UNIX_PLATFORM_UGA_DEVICE_PATH gUgaDevicePath0 = 
{
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_UNIX_THUNK_PROTOCOL_GUID
  },
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (UNIX_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (UNIX_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EFI_UNIX_UGA_GUID
    },
    0
  },
  gEndEntire
};

UNIX_PLATFORM_UGA_DEVICE_PATH gUgaDevicePath1 = {
  {
    {
       HARDWARE_DEVICE_PATH,
       HW_VENDOR_DP,
       {
         (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
         (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
       }
    },
    EFI_UNIX_THUNK_PROTOCOL_GUID
  },
  {
    {
       {
         HARDWARE_DEVICE_PATH,
         HW_VENDOR_DP,
         {
           (UINT8) (sizeof (UNIX_VENDOR_DEVICE_PATH_NODE)),
           (UINT8) ((sizeof (UNIX_VENDOR_DEVICE_PATH_NODE)) >> 8)
         }
       },
       EFI_UNIX_UGA_GUID
    },
    1
  },
  gEndEntire
};

UNIX_CONSOLE_DEVICE_PATH   gUnixConsoleDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_UNIX_THUNK_PROTOCOL_GUID
  },
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8) (sizeof (UNIX_VENDOR_DEVICE_PATH_NODE)),
          (UINT8) ((sizeof (UNIX_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EFI_UNIX_CONSOLE_GUID
    },
    0
  },
  gEndEntire
};
//
// Predefined platform default console device path
//
BDS_CONSOLE_CONNECT_ENTRY   gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gUnixConsoleDevicePath,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gUgaDevicePath0,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gUgaDevicePath1,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    NULL,
    0
  }
};

//
// Predefined platform specific driver option
//
EFI_DEVICE_PATH_PROTOCOL    *gPlatformDriverOption[] = { NULL };

//
// Predefined platform connect sequence
//
EFI_DEVICE_PATH_PROTOCOL    *gPlatformConnectSequence[] = { NULL };
