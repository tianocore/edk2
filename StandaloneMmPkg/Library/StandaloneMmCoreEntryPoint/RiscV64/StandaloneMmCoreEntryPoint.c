/** @file
  Entry point to the Standalone MM Foundation on RISCV platforms

Copyright (c) 2025, Rivos Inc<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

/**
  The entry point of Standalone MM Foundation.

  @param  [in]  CpuId             The Id assigned to this running CPU
  @param  [in]  PayloadInfoAddress   The address of boot info

**/
VOID
EFIAPI
CModuleEntryPoint (
  IN UINT64  CpuId,
  IN VOID    *PayloadInfoAddress
  )
{
  ASSERT (0);
}
