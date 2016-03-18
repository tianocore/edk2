/** @file
*
*  Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "PrePi.h"

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

