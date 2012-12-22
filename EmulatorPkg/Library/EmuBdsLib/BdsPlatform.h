/*++ @file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BDS_PLATFORM_H
#define _BDS_PLATFORM_H

#include <PiDxe.h>

#include <Guid/EmuSystemConfig.h>
#include <Protocol/EmuThunk.h>
#include <Protocol/EmuIoThunk.h>
#include <Protocol/EmuGraphicsWindow.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/GenericBdsLib.h>
#include <Library/PlatformBdsLib.h>
#include <Library/DevicePathLib.h>


extern BDS_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformConnectSequence[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformDriverOption[];

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE,\
    END_ENTIRE_DEVICE_PATH_SUBTYPE,\
    { \
      END_DEVICE_PATH_LENGTH,\
      0\
    }\
  }


typedef struct {
  EMU_VENDOR_DEVICE_PATH_NODE     EmuBus;
  EMU_VENDOR_DEVICE_PATH_NODE     EmuGraphicsWindow;
  EFI_DEVICE_PATH_PROTOCOL        End;
} EMU_PLATFORM_UGA_DEVICE_PATH;


//
// Platform BDS Functions
//
VOID
PlatformBdsGetDriverOption (
  IN LIST_ENTRY               *BdsDriverLists
  );

EFI_STATUS
BdsMemoryTest (
  EXTENDMEM_COVERAGE_LEVEL Level
  );


VOID
PlatformBdsConnectSequence (
  VOID
  );

EFI_STATUS
ProcessCapsules (
  EFI_BOOT_MODE BootMode
  );

EFI_STATUS
PlatformBdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  );

EFI_STATUS
PlatformBdsNoConsoleAction (
  VOID
  );

VOID
PlatformBdsEnterFrontPage (
  IN UINT16                 TimeoutDefault,
  IN BOOLEAN                ConnectAllHappened
  );

#endif // _BDS_PLATFORM_H
