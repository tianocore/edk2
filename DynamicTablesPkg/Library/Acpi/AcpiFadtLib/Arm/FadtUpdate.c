/** @file
  FADT Update header file
  Defines architecture specific function headers.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include  "../FadtUpdate.h"
#include <ArchNameSpaceObjects.h>

/** Gets Architecture specific bitmask for EObjNameSpaceArch namespace.

  @param [out] ArchObjMask   Pointer to the Mask bit variable.

**/
VOID
EFIAPI
GetPlatformNameSpaceMask (
  OUT UINT64  *ArchObjMask
  )
{
  if (ArchObjMask == NULL) {
    return;
  }

  *ArchObjMask = ((*ArchObjMask) |
                  (1 << ArchObjSciInterrupt) |
                  (1 << ArchObjSciCmdInfo) |
                  (1 << ArchObjPmBlockInfo) |
                  (1 << ArchObjGpeBlockInfo) |
                  (1 << ArchObjXpmBlockInfo) |
                  (1 << ArchObjXgpeBlockInfo) |
                  (1 << ArchObjSleepBlockInfo) |
                  (1 << ArchObjResetBlockInfo) |
                  (1 << ArchObjFadtMiscInfo)
                  );
}
