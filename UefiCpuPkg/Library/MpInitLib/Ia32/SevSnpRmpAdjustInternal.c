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
  //
  // RMPADJUST is not supported in 32-bit mode
  //
  return RETURN_UNSUPPORTED;
}
