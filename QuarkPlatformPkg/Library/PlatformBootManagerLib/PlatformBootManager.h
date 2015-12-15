/** @file
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

#include <Library/PlatformBootManagerLib.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Guid/DebugAgentGuid.h>
#include <Guid/EventGroup.h>
#include <Guid/PcAnsi.h>
#include <Guid/TtyTerm.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootManagerLib.h>


typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} PLATFORM_CONSOLE_CONNECT_ENTRY;

extern PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];

#define CONSOLE_OUT BIT0
#define CONSOLE_IN  BIT1
#define STD_ERROR   BIT2

#endif // _PLATFORM_BOOT_MANAGER_H
