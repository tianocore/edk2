/** @file
  Library to get/set Firmware Context.

  Copyright (c) 2021-2022, Hewlett Packard Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef RISCV_FIRMWARE_CONTEXT_LIB_H_
#define RISCV_FIRMWARE_CONTEXT_LIB_H_

#include <Uefi.h>
#include <IndustryStandard/RiscVOpensbi.h>

/**
  Get pointer to OpenSBI Firmware Context

  Get the pointer of firmware context.

  @param    FirmwareContextPtr   Pointer to retrieve pointer to the
                                 Firmware Context.
**/
VOID
EFIAPI
GetFirmwareContextPointer (
  IN OUT EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  **FirmwareContextPtr
  );

/**
  Set pointer to OpenSBI Firmware Context

  Set the pointer of firmware context.

  @param    FirmwareContextPtr   Pointer to Firmware Context.
**/
VOID
EFIAPI
SetFirmwareContextPointer (
  IN EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  *FirmwareContextPtr
  );

#endif
