/** @file
*
*  Copyright (c) 2017, Linaro, Ltd. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Return a pool allocated copy of the DTB image that is appropriate for
  booting the current platform via DT.

  @param[out]   Dtb                   Pointer to the DTB copy
  @param[out]   DtbSize               Size of the DTB copy

  @retval       EFI_SUCCESS           Operation completed successfully
  @retval       EFI_NOT_FOUND         No suitable DTB image could be located
  @retval       EFI_OUT_OF_RESOURCES  No pool memory available

**/
EFI_STATUS
EFIAPI
DtPlatformLoadDtb (
  OUT   VOID   **Dtb,
  OUT   UINTN  *DtbSize
  )
{
  EFI_STATUS  Status;
  VOID        *OrigDtb;
  VOID        *CopyDtb;
  UINTN       OrigDtbSize;

  Status = GetSectionFromAnyFv (
             &gDtPlatformDefaultDtbFileGuid,
             EFI_SECTION_RAW,
             0,
             &OrigDtb,
             &OrigDtbSize
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  CopyDtb = AllocateCopyPool (OrigDtbSize, OrigDtb);
  if (CopyDtb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Dtb     = CopyDtb;
  *DtbSize = OrigDtbSize;

  return EFI_SUCCESS;
}
