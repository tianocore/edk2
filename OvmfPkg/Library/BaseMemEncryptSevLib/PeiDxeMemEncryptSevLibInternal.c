/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2017 - 2020, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <Register/QemuSmramSaveStateMap.h>
#include <Register/SmramSaveStateMap.h>
#include <Uefi/UefiBaseType.h>

/**
  Locate the page range that covers the initial (pre-SMBASE-relocation) SMRAM
  Save State Map.

  @param[out] BaseAddress     The base address of the lowest-address page that
                              covers the initial SMRAM Save State Map.

  @param[out] NumberOfPages   The number of pages in the page range that covers
                              the initial SMRAM Save State Map.

  @retval RETURN_SUCCESS      BaseAddress and NumberOfPages have been set on
                              output.

  @retval RETURN_UNSUPPORTED  SMM is unavailable.
**/
RETURN_STATUS
EFIAPI
MemEncryptSevLocateInitialSmramSaveStateMapPages (
  OUT UINTN *BaseAddress,
  OUT UINTN *NumberOfPages
  )
{
  UINTN MapStart;
  UINTN MapEnd;
  UINTN MapPagesStart; // MapStart rounded down to page boundary
  UINTN MapPagesEnd;   // MapEnd rounded up to page boundary
  UINTN MapPagesSize;  // difference between MapPagesStart and MapPagesEnd

  if (!FeaturePcdGet (PcdSmmSmramRequire)) {
    return RETURN_UNSUPPORTED;
  }

  MapStart      = SMM_DEFAULT_SMBASE + SMRAM_SAVE_STATE_MAP_OFFSET;
  MapEnd        = MapStart + sizeof (QEMU_SMRAM_SAVE_STATE_MAP);
  MapPagesStart = MapStart & ~(UINTN)EFI_PAGE_MASK;
  MapPagesEnd   = ALIGN_VALUE (MapEnd, EFI_PAGE_SIZE);
  MapPagesSize  = MapPagesEnd - MapPagesStart;

  ASSERT ((MapPagesSize & EFI_PAGE_MASK) == 0);

  *BaseAddress   = MapPagesStart;
  *NumberOfPages = MapPagesSize >> EFI_PAGE_SHIFT;

  return RETURN_SUCCESS;
}
