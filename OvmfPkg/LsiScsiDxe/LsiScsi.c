/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  LSI 53C895A SCSI devices.

  Copyright (C) 2020, SUSE LLC.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi/UefiSpec.h>

//
// Entry point of this driver
//
EFI_STATUS
EFIAPI
LsiScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EFI_UNSUPPORTED;
}
