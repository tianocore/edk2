/** @file

  RISC-V backtrace definition file.

  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <PiPei.h>
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiLib.h>
#include <Guid/DebugImageInfoTable.h>
#include <CpuExceptionHelpers.h>
#include "ExceptionHandler.h"

/**
  Display a backtrace.

  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
DumpCpuBacktrace (
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );
