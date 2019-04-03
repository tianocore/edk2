/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_PLATFORM_LIB_H_
#define _FSP_PLATFORM_LIB_H_

/**
  Get system memory from HOB.

  @param[in,out] LowMemoryLength   less than 4G memory length
  @param[in,out] HighMemoryLength  greater than 4G memory length
**/
VOID
EFIAPI
FspGetSystemMemorySize (
  IN OUT UINT64              *LowMemoryLength,
  IN OUT UINT64              *HighMemoryLength
  );

/**
  Migrate BootLoader data before destroying CAR.

**/
VOID
EFIAPI
FspMigrateTemporaryMemory (
  VOID
  );

/**
  Set a new stack frame for the continuation function.

**/
VOID
EFIAPI
FspSetNewStackFrame (
  VOID
  );

/**
  This function transfer control to the ContinuationFunc passed in by the
  BootLoader.

**/
VOID
EFIAPI
FspInitDone (
  VOID
  );

/**
  This function handle NotifyPhase API call from the BootLoader.
  It gives control back to the BootLoader after it is handled. If the
  Notification code is a ReadyToBoot event, this function will return
  and FSP continues the remaining execution until it reaches the DxeIpl.

**/
VOID
EFIAPI
FspWaitForNotify (
  VOID
  );

#endif
