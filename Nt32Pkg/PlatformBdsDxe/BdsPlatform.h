/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  BdsPlatform.h

Abstract:

  Head file for BDS Platform specific code

--*/

#ifndef _BDS_PLATFORM_H
#define _BDS_PLATFORM_H

#define FIRMWARE_REVISION 0

extern UINT8 PlatformBdsStrings[];

#include "IndustryStandard/Pci22.h"

extern BDS_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformConnectSequence[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformDriverOption[];

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE,\
    END_ENTIRE_DEVICE_PATH_SUBTYPE,\
    END_DEVICE_PATH_LENGTH,\
    0\
  }

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
  WIN_NT_VENDOR_DEVICE_PATH_NODE  NtUgaDevice;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_PLATFORM_UGA_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  NtGopDevice;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_PLATFORM_GOP_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  NtCpuModelDevice;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_PLATFORM_CPU_MODEL_VIRTUAL_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  NtCpuSpeedDevice;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_PLATFORM_CPU_SPEED_VIRTUAL_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH              NtBus;
  WIN_NT_VENDOR_DEVICE_PATH_NODE  NtMemoryDeivce;
  EFI_DEVICE_PATH_PROTOCOL        End;
} NT_PLATFORM_MEMORY_VIRTUAL_DEVICE_PATH;

//
// Platform BDS Functions
//
VOID
PlatformBdsInit (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData
  )
;

VOID
PlatformBdsPolicyBehavior (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData,
  IN LIST_ENTRY                      *DriverOptionList,
  IN LIST_ENTRY                      *BootOptionList
  )
;

VOID
PlatformBdsGetDriverOption (
  IN LIST_ENTRY               *BdsDriverLists
  )
;

EFI_STATUS
BdsMemoryTest (
  EXTENDMEM_COVERAGE_LEVEL Level
  )
;

EFI_STATUS
PlatformBdsShowProgress (
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  CHAR16                        *Title,
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  UINTN                         Progress,
  UINTN                         PreviousValue
  )
;

VOID
PlatformBdsConnectSequence (
  VOID
  )
;

VOID
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  )
;

VOID
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION *Option
  )
;

EFI_STATUS
ProcessCapsules (
  EFI_BOOT_MODE BootMode
  )
;

EFI_STATUS
PlatformBdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  )
;

EFI_STATUS
PlatformBdsNoConsoleAction (
  VOID
  )
;

#endif // _BDS_PLATFORM_H
