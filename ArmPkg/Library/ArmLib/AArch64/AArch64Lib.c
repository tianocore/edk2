/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Portions copyright (c) 2011 - 2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
