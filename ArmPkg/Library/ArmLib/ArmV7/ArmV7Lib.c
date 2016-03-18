/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Chipset/ArmV7.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include "ArmV7Lib.h"
#include "ArmLibPrivate.h"

VOID
ArmV7DataCacheOperation (
  IN  ARM_V7_CACHE_OPERATION  DataCacheOperation
  )
{
  UINTN     SavedInterruptState;

  SavedInterruptState = ArmGetInterruptState ();
  ArmDisableInterrupts ();

  ArmV7AllDataCachesOperation (DataCacheOperation);

  ArmDataSynchronizationBarrier ();

  if (SavedInterruptState) {
    ArmEnableInterrupts ();
  }
}

VOID
EFIAPI
ArmInvalidateDataCache (
  VOID
  )
{
  ArmDataSynchronizationBarrier ();
  ArmV7DataCacheOperation (ArmInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  )
{
  ArmDataSynchronizationBarrier ();
  ArmV7DataCacheOperation (ArmCleanInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  )
{
  ArmDataSynchronizationBarrier ();
  ArmV7DataCacheOperation (ArmCleanDataCacheEntryBySetWay);
}
