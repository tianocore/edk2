/** @file
  UefiCpu X86 architectures support library for both Ia32 and X64.

  Copyright 2024 Google LLC

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>

UINT8
ArchGetPhysicalAddressBits (
  VOID
  )
{
  UINT32  RegEax;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    return (UINT8)RegEax;
  }

  return 36;
}
