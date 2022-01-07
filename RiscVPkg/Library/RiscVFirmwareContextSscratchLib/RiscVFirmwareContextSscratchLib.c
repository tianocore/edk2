/** @file
  This instance uses Supervisor mode SCRATCH CSR to get/set the
  pointer of firmware context.

  Copyright (c) 2021-2022 Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>

#include <IndustryStandard/RiscVOpensbi.h>

#include <Library/DebugLib.h>
#include <Library/RiscVCpuLib.h>

/**
  Get pointer to OpenSBI Firmware Context

  Get the pointer of firmware context through Supervisor mode SCRATCH CSR.

  @param    FirmwareContextPtr   Pointer to retrieve pointer to the
                                 Firmware Context.
**/
VOID
EFIAPI
GetFirmwareContextPointer (
  IN OUT EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  **FirmwareContextPtr
  )
{
  *FirmwareContextPtr = (EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *)RiscVGetSupervisorScratch ();
}

/**
  Set the pointer to OpenSBI Firmware Context

  Set the pointer of firmware context through Supervisor mode SCRATCH CSR.

  @param    FirmwareContextPtr   Pointer to Firmware Context.
**/
VOID
EFIAPI
SetFirmwareContextPointer (
  IN EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  *FirmwareContextPtr
  )
{
  RiscVSetSupervisorScratch ((UINT64)FirmwareContextPtr);
}
