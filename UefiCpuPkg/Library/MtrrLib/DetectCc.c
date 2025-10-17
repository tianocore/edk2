/** @file
  CcMode support library for Confidential Computing Mode

  @par Note:
    CcMode is needed to be a library function that is different for Smm build modes
    compared to Confidential Computing build modes (TDX and SEV).

  Copyright (c) 2025, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <ConfidentialComputingGuestAttr.h>
#include <Library/BaseLib.h>
#include "DetectCc.h"

/**
Check if AMD SEV-SNP is active.

@retval TRUE   AMD SEV-SNP is active.
@retval FALSE  AMD SEV-SNP is not active.

**/
STATIC
BOOLEAN
SnpIsEnabled (
  VOID
  )
{
  UINT64  CurrentAttr = PcdGet64 (PcdConfidentialComputingGuestAttr);

  //
  // If attr is for the AMD group then do AMD SEV-SNP specific check.
  //
  if (((RShiftU64 (CurrentAttr, 8)) & 0xff) == 1) {
    UINT64  CurrentLevel = CurrentAttr & CCAttrTypeMask;

    if (CurrentLevel == CCAttrAmdSevSnp) {
      return TRUE;
    }
  }

  return FALSE;
}

BOOLEAN
CcMode (
  VOID
  )
{
  STATIC BOOLEAN  mCcMode     = FALSE;
  STATIC BOOLEAN  mCcModeRead = FALSE;

  if (!mCcModeRead) {
    mCcMode     = TdIsEnabled () || SnpIsEnabled ();
    mCcModeRead = TRUE;
  }

  return mCcMode;
}
