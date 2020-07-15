/** @file
  Flush the cache is required for most architectures while do capsule
  update. It is not support at Runtime.

  Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleService.h"

#include <Library/CacheMaintenanceLib.h>

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

  if (!EfiAtRuntime ()) {
    Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)ScatterGatherList;
    do {
      WriteBackDataCacheRange (
        (VOID *)(UINTN)Desc,
        (UINTN)sizeof (*Desc)
        );

      if (Desc->Length > 0) {
        WriteBackDataCacheRange (
          (VOID *)(UINTN)Desc->Union.DataBlock,
          (UINTN)Desc->Length
          );
        Desc++;
      } else if (Desc->Union.ContinuationPointer > 0) {
        Desc = (EFI_CAPSULE_BLOCK_DESCRIPTOR *)(UINTN)Desc->Union.ContinuationPointer;
      }
    } while (Desc->Length > 0 || Desc->Union.ContinuationPointer > 0);

    WriteBackDataCacheRange (
      (VOID *)(UINTN)Desc,
      (UINTN)sizeof (*Desc)
      );
  }
}

