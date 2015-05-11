/**@file
  Defined the platform specific device path which will be filled to
  ConIn/ConOut variables.
   
Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#include "PlatformBootManager.h"

//
// Platform specific keyboard device path
//
NT_PLATFORM_GOP_DEVICE_PATH gGopDevicePath0 = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    EFI_WIN_NT_THUNK_PROTOCOL_GUID
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)),
    (UINT8) ((sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)) >> 8),
    EFI_WIN_NT_GOP_GUID,
    0
  },
  gEndEntire
};

NT_PLATFORM_GOP_DEVICE_PATH gGopDevicePath1 = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    EFI_WIN_NT_THUNK_PROTOCOL_GUID
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)),
    (UINT8) ((sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)) >> 8),
    EFI_WIN_NT_GOP_GUID,
    1
  },
  gEndEntire
};

//
// Platform specific serial device path
//
NT_ISA_SERIAL_DEVICE_PATH   gNtSerialDevicePath0 = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    EFI_WIN_NT_THUNK_PROTOCOL_GUID
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)),
    (UINT8) ((sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)) >> 8),
    EFI_WIN_NT_SERIAL_PORT_GUID
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_UART_DP,
    (UINT8) (sizeof (UART_DEVICE_PATH)),
    (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8),
    0,
    115200,
    8,
    1,
    1
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

NT_ISA_SERIAL_DEVICE_PATH   gNtSerialDevicePath1 = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    EFI_WIN_NT_THUNK_PROTOCOL_GUID
  },
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    (UINT8) (sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)),
    (UINT8) ((sizeof (WIN_NT_VENDOR_DEVICE_PATH_NODE)) >> 8),
    EFI_WIN_NT_SERIAL_PORT_GUID,
    1
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_UART_DP,
    (UINT8) (sizeof (UART_DEVICE_PATH)),
    (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8),
    0,
    115200,
    8,
    1,
    1
  },
  {
    MESSAGING_DEVICE_PATH,
    MSG_VENDOR_DP,
    (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
    (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8),
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

//
// Predefined platform default console device path
//
PLATFORM_CONSOLE_CONNECT_ENTRY   gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gNtSerialDevicePath0,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gNtSerialDevicePath1,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gGopDevicePath0,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gGopDevicePath1,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    NULL,
    0
  }
};
