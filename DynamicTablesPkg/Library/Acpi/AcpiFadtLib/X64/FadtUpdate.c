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
                       (1 << EArchObjFadtArmBootArch) |
                       (1 << EArchObjFadtHypervisorVendorId)
                       );
}
