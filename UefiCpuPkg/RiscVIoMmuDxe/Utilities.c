/** @file
  RISC-V IOMMU helper functions.

  Copyright (c) 2025-2026, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include "RiscVIoMmu.h"

/**
  Write a 32-bit IOMMU register and wait for a mask to be set/unset.

  @param[in]  Address       The register to write.
  @param[in]  Value         The value to write to the register.
  @param[in]  Mask          The bitmask to wait for.
  @param[in]  Set           Whether the mask should be set or unset.

**/
VOID
IoMmuWriteAndWait32 (
  IN UINTN    Address,
  IN UINT32   Value,
  IN UINT32   Mask,
  IN BOOLEAN  Set
  )
{
  UINT32  RegValue;

  MmioWrite32 (Address, Value);

  RegValue = MmioRead32 (Address);
  while ((Set && !(RegValue & Mask)) || (!Set && (RegValue & Mask))) {
    MicroSecondDelay (1000);
    RegValue = MmioRead32 (Address);
  }
}

/**
  Write a 64-bit IOMMU register and wait for a mask to be set/unset.

  @param[in]  Address       The register to write.
  @param[in]  Value         The value to write to the register.
  @param[in]  Mask          The bitmask to wait for.
  @param[in]  Set           Whether the mask should be set or unset.

**/
VOID
IoMmuWriteAndWait64 (
  IN UINTN    Address,
  IN UINT64   Value,
  IN UINT64   Mask,
  IN BOOLEAN  Set
  )
{
  UINT64  RegValue;

  MmioWrite64 (Address, RegValue);

  RegValue = MmioRead64 (Address);
  while ((Set && !(RegValue & Mask)) || (!Set && (RegValue & Mask))) {
    MicroSecondDelay (1000);
    RegValue = MmioRead64 (Address);
  }
}
