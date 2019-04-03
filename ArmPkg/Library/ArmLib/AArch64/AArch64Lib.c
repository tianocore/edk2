/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Chipset/AArch64.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include "AArch64Lib.h"
#include "ArmLibPrivate.h"

VOID
AArch64DataCacheOperation (
  IN  AARCH64_CACHE_OPERATION  DataCacheOperation
  )
{
  UINTN     SavedInterruptState;

  SavedInterruptState = ArmGetInterruptState ();
  ArmDisableInterrupts();

  AArch64AllDataCachesOperation (DataCacheOperation);

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
  AArch64DataCacheOperation (ArmInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  )
{
  ArmDataSynchronizationBarrier ();
  AArch64DataCacheOperation (ArmCleanInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  )
{
  ArmDataSynchronizationBarrier ();
  AArch64DataCacheOperation (ArmCleanDataCacheEntryBySetWay);
}
