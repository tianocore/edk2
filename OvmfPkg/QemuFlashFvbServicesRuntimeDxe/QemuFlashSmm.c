/** @file
  Define the module hooks used while probing the QEMU flash device.

  Copyright (C) 2018, Advanced Micro Devices. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/MemEncryptSevLib.h>

#include "QemuFlash.h"

VOID
QemuFlashBeforeProbe (
  IN  EFI_PHYSICAL_ADDRESS    BaseAddress,
  IN  UINTN                   FdBlockSize,
  IN  UINTN                   FdBlockCount
  )
{
  EFI_STATUS              Status;

  ASSERT (FeaturePcdGet (PcdSmmSmramRequire));

  if (!MemEncryptSevIsEnabled ()) {
    return;
  }

  //
  // When SEV is enabled, AmdSevDxe runs early in DXE phase and clears the
  // C-bit from the NonExistent entry -- which is later split and accommodate
  // the flash MMIO but the driver runs in non SMM context hence it cleared the
  // flash ranges from non SMM page table. When SMM is enabled, the flash
  // services are accessed from the SMM mode hence we explicitly clear the
  // C-bit on flash ranges from SMM page table.
  //

  Status = MemEncryptSevClearMmioPageEncMask (
             0,
             BaseAddress,
             EFI_SIZE_TO_PAGES (FdBlockSize * FdBlockCount)
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Write to QEMU Flash

  @param[in] Ptr    Pointer to the location to write.
  @param[in] Value  The value to write.

**/
VOID
QemuFlashPtrWrite (
  IN        volatile UINT8    *Ptr,
  IN        UINT8             Value
  )
{
  *Ptr = Value;
}
