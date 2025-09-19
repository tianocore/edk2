/** @file
  CcMode support library for Confidential Computing Mode

  @par Note:
    CcMode is needed to be a library function that is different for Smm build modes
    compared to Confidential Computing build modes (TDX and SEV).

  Copyright (c) 2025, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include "DetectCc.h"

BOOLEAN
CcMode (
  VOID
  )
{
  STATIC BOOLEAN  mCcMode     = FALSE;
  STATIC BOOLEAN  mCcModeRead = FALSE;

  if (!mCcModeRead) {
    mCcMode     = TdIsEnabled ();
    mCcModeRead = TRUE;
  }

  return mCcMode;
}
