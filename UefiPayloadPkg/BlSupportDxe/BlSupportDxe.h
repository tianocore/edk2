/** @file
  The header file of bootloader support DXE.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DXE_BOOTLOADER_SUPPORT_H__
#define __DXE_BOOTLOADER_SUPPORT_H__

#include <PiDxe.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/SystemResourceTable.h>
#include <Guid/AcpiBoardInfoGuid.h>
#include <Guid/FirmwareInfoGuid.h>
#include <Guid/GraphicsInfoHob.h>

EFI_STATUS
EFIAPI
BlArchAdditionalOps (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
