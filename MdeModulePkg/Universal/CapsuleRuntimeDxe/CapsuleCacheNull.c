/** @file
  Null function version of cache function.

  Copyright (c) 2018, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleService.h"

#include <Library/CacheMaintenanceLib.h>

/**
  Writes Back a range of data cache lines covering a set of capsules in memory.

  Writes Back the data cache lines specified by ScatterGatherList.

  Null version, do nothing.

  @param  ScatterGatherList Physical address of the data structure that
                            describes a set of capsules in memory

**/
VOID
CapsuleCacheWriteBack (
  IN  EFI_PHYSICAL_ADDRESS    ScatterGatherList
  )
{
}

