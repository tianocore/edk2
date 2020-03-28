/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  pvscsi devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiSpec.h>

//
// Entry Point
//

EFI_STATUS
EFIAPI
PvScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EFI_UNSUPPORTED;
}
