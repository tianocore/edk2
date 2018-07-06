/** @file
  Define the module hooks used while probing the QEMU flash device.

  Copyright (C) 2018, Advanced Micro Devices. All rights reserved.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

  Status = MemEncryptSevClearPageEncMask (
             0,
             BaseAddress,
             EFI_SIZE_TO_PAGES (FdBlockSize * FdBlockCount),
             FALSE
             );
  ASSERT_EFI_ERROR (Status);
}
