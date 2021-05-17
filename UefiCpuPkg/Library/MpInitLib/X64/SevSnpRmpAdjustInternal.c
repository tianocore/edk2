/** @file

  RMPADJUST helper function.

  Copyright (c) 2021, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"

/**
  Issue RMPADJUST to adjust the attributes of an SEV-SNP page.

  @param[in]  PageAddress
  @param[in]  VmsaPage

  @return  RMPADJUST return value
**/
UINT32
SevSnpRmpAdjust (
  IN  EFI_PHYSICAL_ADDRESS  PageAddress,
  IN  BOOLEAN               VmsaPage
  )
{
  UINT64  Rdx;
  UINT8   Vmpl;
  UINT8   PermissionMask;

  //
  // Use the highest VMPL level.
  //
  PermissionMask = 0;
  Vmpl = RMPADJUST_VMPL_MAX;

  Rdx = (Vmpl & RMPADJUST_VMPL_MASK) << RMPADJUST_VMPL_SHIFT;
  Rdx |= (PermissionMask & RMPADJUST_PERMISSION_MASK_MASK) << RMPADJUST_PERMISSION_MASK_SHIFT;
  if (VmsaPage) {
    Rdx |= RMPADJUST_VMSA_PAGE_BIT;
  }

  return AsmRmpAdjust ((UINT64) PageAddress, 0, Rdx);
}
