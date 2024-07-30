/** @file

  Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeilessSec.h"

/**
  Architecture specific initialization routine.
**/
VOID
ArchInitialize (
  VOID
  )
{
  // Enable program flow prediction, if supported.
  ArmEnableBranchPrediction ();

  if (FixedPcdGet32 (PcdVFPEnabled)) {
    ArmEnableVFP ();
  }
}
