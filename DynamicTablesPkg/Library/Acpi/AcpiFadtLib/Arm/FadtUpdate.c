/** @file
  FADT Update header file
  Defines architecture specific function headers.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include  "../FadtUpdate.h"
#include <ArchNameSpaceObjects.h>

/** Gets Architecture specific bitmask for EObjNameSpaceArch namespace.

  @param [out] EArchObjFadtMask   Pointer to the Mask bit variable.

**/
VOID
EFIAPI
GetPlatformNameSpaceMask (
  OUT UINT64  *EArchObjFadtMask
  )
{
  if (EArchObjFadtMask == NULL) {
    return;
  }

  *EArchObjFadtMask = ((*EArchObjFadtMask) |
                       (1 << EArchObjFadtSciInterrupt) |
                       (1 << EArchObjFadtSciCmdInfo) |
                       (1 << EArchObjFadtPmBlockInfo) |
                       (1 << EArchObjFadtGpeBlockInfo) |
                       (1 << EArchObjFadtXpmBlockInfo) |
                       (1 << EArchObjFadtXgpeBlockInfo) |
                       (1 << EArchObjFadtSleepBlockInfo) |
                       (1 << EArchObjFadtResetBlockInfo) |
                       (1 << EArchObjFadtMiscInfo)
                       );
}
