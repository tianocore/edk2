/** @file

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _INTERNAL_LEGACY_BM_H_
#define _INTERNAL_LEGACY_BM_H_

#include <PiDxe.h>
#include <Guid/LegacyDevOrder.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PerformanceLib.h>

#pragma pack(1)
typedef struct {
  UINT16     BbsIndex;
} LEGACY_BM_BOOT_OPTION_BBS_DATA;
#pragma pack()

/**
  Boot the legacy system with the boot option.

  @param  BootOption The legacy boot option which have BBS device path
                     On return, BootOption->Status contains the boot status.
                     EFI_UNSUPPORTED    There is no legacybios protocol, do not support
                                        legacy boot.
                     EFI_STATUS         The status of LegacyBios->LegacyBoot ().
**/
VOID
EFIAPI
LegacyBmBoot (
  IN  EFI_BOOT_MANAGER_LOAD_OPTION           *BootOption
  );

/**
  Refresh all legacy boot options.

**/
VOID
EFIAPI
LegacyBmRefreshAllBootOption (
  VOID
  );

#endif // _INTERNAL_LEGACY_BM_H_