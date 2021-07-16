/** @file
  instance of TdxProbeLib

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Library/BaseLib.h>
#include <Library/TdxProbeLib.h>
#include "InternalTdxProbe.h"

BOOLEAN mTdxEnabled = FALSE;
BOOLEAN mTdxProbed = FALSE;

/**
  Whether Intel TDX is enabled.

  @return TRUE    TDX enabled
  @return FALSE   TDX not enabled
**/
BOOLEAN
EFIAPI
TdxIsEnabled (
  VOID)
{
  if (mTdxProbed) {
    return mTdxEnabled;
  }

  mTdxEnabled = TdProbe () == PROBE_IS_TD_GUEST;
  mTdxProbed = TRUE;
  return mTdxEnabled;
}
