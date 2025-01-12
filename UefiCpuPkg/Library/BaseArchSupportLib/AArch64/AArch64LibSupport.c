/** @file
  UefiCpu AArch64 architecture support library.

  Copyright 2024 Google LLC

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/ArmLib.h>

VOID
EFIAPI
InitializeFloatingPointUnits (
  VOID
  )
{
  return;
}

UINTN
ArchGetPhysicalAddressBits (
  VOID
  )
{
  return ArmGetPhysicalAddressBits ();
}
