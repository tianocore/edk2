/** @file
This library includes the recovery function that can be customized by OEM,
including how to select the recovery capsule if more than one capsule found,
and security check.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __RECOVERY_OEM_HOOK_LIB_H__
#define __RECOVERY_OEM_HOOK_LIB_H__

/**
  This function allows the user to force a system recovery

**/
VOID
EFIAPI
OemInitiateRecovery (
  VOID
  );

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
  );

#endif
