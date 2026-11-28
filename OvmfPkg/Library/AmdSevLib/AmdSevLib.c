/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2026, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AmdSevLib.h>
#include <Library/PcdLib.h>
#include <Uefi/UefiBaseType.h>

/**
   Read the workarea to determine whether SEV is enabled. If enabled,
   then return the SevEsWorkArea pointer.

  **/
SEC_SEV_ES_WORK_AREA *
EFIAPI
AmdSevGetSevEsWorkArea (
  VOID
  )
{
  OVMF_WORK_AREA  *WorkArea;

  WorkArea = (OVMF_WORK_AREA *)FixedPcdGet32 (PcdOvmfWorkAreaBase);

  //
  // If its not SEV guest then SevEsWorkArea is not valid.
  //
  if ((WorkArea == NULL) || (WorkArea->Header.GuestType != CcGuestTypeAmdSev)) {
    return NULL;
  }

  return (SEC_SEV_ES_WORK_AREA *)FixedPcdGet32 (PcdSevEsWorkAreaBase);
}
