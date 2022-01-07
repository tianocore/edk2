/** @file
  This instance uses RISC-V OpenSBI Firmware Extension SBI to
  get the pointer of firmware context.

  Copyright (c) 2021-2022 Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/RiscVCpuLib.h>
#include <Library/RiscVEdk2SbiLib.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_platform.h>

/**
  Get pointer to OpenSBI Firmware Context

  Get the pointer of firmware context through OpenSBI FW Extension SBI.

  @param    FirmwareContextPtr   Pointer to retrieve pointer to the
                                 Firmware Context.
**/
VOID
EFIAPI
GetFirmwareContextPointer (
  IN OUT EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT **FirmwareContextPtr
  )
{
  SbiGetFirmwareContext (FirmwareContextPtr);
}

/**
  Set the pointer to OpenSBI Firmware Context

  Set the pointer of firmware context through OpenSBI FW Extension SBI.

  @param    FirmwareContextPtr   Pointer to Firmware Context.
**/
VOID
EFIAPI
SetFirmwareContextPointer (
  IN EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContextPtr
  )
{
  //
  // We don't have to set firmware context pointer using
  // OpenSBI FW Extension SBI.
  //
}

