/** @file
This file includes the function that can be customized by OEM.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CommonHeader.h"

/**
  This function allows the user to force a system recovery

**/
VOID
EFIAPI
OemInitiateRecovery (
  VOID
  )
{
  UINT32          Data32;

  //
  // Set 'B_CFG_STICKY_RW_FORCE_RECOVERY' sticky bit so we know we need to do a recovery following warm reset
  //
  Data32 = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_CFG_STICKY_RW);
  Data32 |= B_CFG_STICKY_RW_FORCE_RECOVERY;
  QNCAltPortWrite (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_CFG_STICKY_RW, Data32);

  //
  // Initialte the warm reset
  //
  ResetWarm ();
}

/**
  This function allows the user to force a system recovery and deadloop.

  Deadloop required since system should not execute beyond this point.
  Deadloop should never happen since OemInitiateRecovery () called within
  this routine should never return since it executes a Warm Reset.

**/
VOID
EFIAPI
OemInitiateRecoveryAndWait (
  VOID
  )
{
  volatile UINTN  Index;

  OemInitiateRecovery ();
  for (Index = 0; Index == 0;);
}
