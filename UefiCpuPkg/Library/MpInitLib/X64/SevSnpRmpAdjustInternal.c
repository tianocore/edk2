/** @file

  RMPADJUST helper function.

  Copyright (c) 2021, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"

/**
  Issue RMPADJUST to adjust the VMSA attribute of an SEV-SNP page.

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

  //
  // The RMPADJUST instruction is used to set or clear the VMSA bit for a
  // page. The VMSA change is only made when running at VMPL0 and is ignored
  // otherwise. If too low a target VMPL is specified, the instruction can
  // succeed without changing the VMSA bit when not running at VMPL0. Using a
  // target VMPL level of 1, RMPADJUST will return a FAIL_PERMISSION error if
  // not running at VMPL0, thus ensuring that the VMSA bit is set appropriately
  // when no error is returned.
  //
  Rdx = 1;
  if (VmsaPage) {
    Rdx |= RMPADJUST_VMSA_PAGE_BIT;
  }

  return AsmRmpAdjust ((UINT64) PageAddress, 0, Rdx);
}
