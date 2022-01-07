/** @file
  RISC-V package definitions.

  Copyright (c) 2016 - 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RISCV_PLATFORM_TEMP_MEM_LIB_H_
#define RISCV_PLATFORM_TEMP_MEM_LIB_H_

#include "RiscVImpl.h"

VOID EFIAPI
RiscVPlatformTemporaryMemInit (
  VOID
  );

UINT32 EFIAPI
RiscVPlatformTemporaryMemSize (
  VOID
  );

UINT32 EFIAPI
RiscVPlatformTemporaryMemBase (
  VOID
  );

#endif
