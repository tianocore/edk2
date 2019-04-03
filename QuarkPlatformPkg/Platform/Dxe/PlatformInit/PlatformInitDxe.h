/** @file
Platform init DXE driver header file.

Copyright (c) 2013 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PLATFORM_TYPES_H_
#define _PLATFORM_TYPES_H_

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PlatformHelperLib.h>
#include <Library/PlatformPcieHelperLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/I2cLib.h>
#include <Protocol/Variable.h>
#include <Protocol/Cpu.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/Spi.h>
#include <Protocol/PlatformSmmSpiReady.h>
#include <Protocol/SmmConfiguration.h>
#include <Guid/HobList.h>
#include <IntelQNCRegs.h>
#include <Platform.h>
#include <Pcal9555.h>
#include <PlatformBoards.h>
#include <IohAccess.h>

#define BLOCK_SIZE_32KB                             0x8000
#define BLOCK_SIZE_64KB                             0x10000

//
// Function prototypes for routines private to this driver.
//
EFI_STATUS
EFIAPI
CreateConfigEvents (
  VOID
  );

EFI_STATUS
EFIAPI
PlatformPcal9555Config (
  IN CONST EFI_PLATFORM_TYPE              PlatformType
  );

#endif
