/** @file
*  An "early" DXE driver that parses well-known fw-cfg files into dynamic PCDs
*  that control other (universal) DXE drivers.
*
*  Copyright (C) 2015, Red Hat, Inc.
*  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
*  IMPLIED.
*
**/

#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiSpec.h>

#include <Library/PcdLib.h>
#include <Library/QemuFwCfgLib.h>

EFI_STATUS
EFIAPI
ParseQemuFwCfgToPcd (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EFI_SUCCESS;
}
