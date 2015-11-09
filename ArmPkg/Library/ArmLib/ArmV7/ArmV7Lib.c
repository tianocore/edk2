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

UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (0) & 7;

  // * 4 converts to bytes
  return (1 << (CCSIDR + 2)) * 4;
}

UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  )
{
  UINT32 CCSIDR = ReadCCSIDR (1) & 7;

  // * 4 converts to bytes
  return (1 << (CCSIDR + 2)) * 4;

//  return 64;
}


VOID
ArmV7DataCacheOperation (
  IN  ARM_V7_CACHE_OPERATION  DataCacheOperation
  )
{
  UINTN     SavedInterruptState;

  SavedInterruptState = ArmGetInterruptState ();
  ArmDisableInterrupts ();

  ArmV7AllDataCachesOperation (DataCacheOperation);

  ArmDrainWriteBuffer ();

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
  ArmDrainWriteBuffer ();
  ArmV7DataCacheOperation (ArmInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanInvalidateDataCache (
  VOID
  )
{
  ArmDrainWriteBuffer ();
  ArmV7DataCacheOperation (ArmCleanInvalidateDataCacheEntryBySetWay);
}

VOID
EFIAPI
ArmCleanDataCache (
  VOID
  )
{
  ArmDrainWriteBuffer ();
  ArmV7DataCacheOperation (ArmCleanDataCacheEntryBySetWay);
}
