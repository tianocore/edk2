/** @file
  ARM implementation of architecture specific routines related to
  PersistAcrossReset capsules

  Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CapsuleService.h"

#include <Library/CacheMaintenanceLib.h>

/**
  Whether the platform supports capsules that persist across reset. Note that
  some platforms only support such capsules at boot time.

  @return TRUE  if a PersistAcrossReset capsule may be passed to UpdateCapsule()
                at this time
          FALSE otherwise
**/
BOOLEAN
IsPersistAcrossResetCapsuleSupported (
  VOID
  )
{
  //
  // ARM requires the capsule payload to be cleaned to the point of coherency
  // (PoC), but only permits doing so using cache maintenance instructions that
  // operate on virtual addresses. Since at runtime, we don't know the virtual
  // addresses of the data structures that make up the scatter/gather list, we
  // cannot perform the maintenance, and all we can do is give up.
  //
  return FeaturePcdGet (PcdSupportUpdateCapsuleReset) && !EfiAtRuntime ();
}

/**
  Writes Back a range of data cache lines covering a set of capsules in memory.

  Writes Back the data cache lines specified by ScatterGatherList.

  @param  ScatterGatherList Physical address of the data structure that
                            describes a set of capsules in memory

**/
VOID
CapsuleCacheWriteBack (
  IN  EFI_PHYSICAL_ADDRESS    ScatterGatherList
  )
{
  EFI_CAPSULE_BLOCK_DESCRIPTOR    *Desc;

  Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)ScatterGatherList;
  do {
    WriteBackDataCacheRange (Desc, sizeof *Desc);

    if (Desc->Length > 0) {
      WriteBackDataCacheRange ((VOID *)(UINTN)Desc->Union.DataBlock,
                               Desc->Length
                               );
      Desc++;
    } else if (Desc->Union.ContinuationPointer > 0) {
      Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)Desc->Union.ContinuationPointer;
    }
  } while (Desc->Length > 0 || Desc->Union.ContinuationPointer > 0);

  WriteBackDataCacheRange (Desc, sizeof *Desc);
}
