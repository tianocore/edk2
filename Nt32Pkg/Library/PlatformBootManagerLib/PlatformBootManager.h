/**@file
   Head file for BDS Platform specific code

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#ifndef _PLATFORM_BOOT_MANAGER_H
#define _PLATFORM_BOOT_MANAGER_H

#include <PiDxe.h>

#include <Guid/WinNtSystemConfig.h>
#include <Protocol/WinNtThunk.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/LoadedImage.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>


typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} PLATFORM_CONSOLE_CONNECT_ENTRY;

extern PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE,\
    END_ENTIRE_DEVICE_PATH_SUBTYPE,\
    END_DEVICE_PATH_LENGTH,\
    0\
  }

#define CONSOLE_OUT BIT0
#define CONSOLE_IN  BIT1
#define STD_ERROR   BIT2

typedef struct {
  VENDOR_DEVICE_PATH  VendorDevicePath;
  UINT32              Instance;
} WIN_NT_VENDOR_DEVICE_PATH_NODE;

//
// Below is the platform console device path
//
typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  SerialDevice;
  UART_DEVICE_PATH                Uart;
  VENDOR_DEVICE_PATH              TerminalType;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_ISA_SERIAL_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  NtGopDevice;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_PLATFORM_GOP_DEVICE_PATH;

#endif // _PLATFORM_BOOT_MANAGER_H
