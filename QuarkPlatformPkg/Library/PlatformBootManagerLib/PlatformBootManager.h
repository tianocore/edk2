/** @file
Head file for BDS Platform specific code

Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _PLATFORM_BOOT_MANAGER_H
#define _PLATFORM_BOOT_MANAGER_H

#include <PiDxe.h>

#include <Library/PlatformBootManagerLib.h>

#include <Protocol/FirmwareVolume2.h>
#include <Protocol/AcpiS3Save.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/EsrtManagement.h>
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
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/PrintLib.h>
#include <Library/HobLib.h>
#include <Library/CapsuleLib.h>
#include <Library/DxeServicesLib.h>

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} PLATFORM_CONSOLE_CONNECT_ENTRY;

extern PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];

#define CONSOLE_OUT BIT0
#define CONSOLE_IN  BIT1
#define STD_ERROR   BIT2

#endif // _PLATFORM_BOOT_MANAGER_H
