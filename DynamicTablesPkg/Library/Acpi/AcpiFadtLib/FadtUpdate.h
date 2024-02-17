/** @file
  FADT Update header file
  Defines architecture specific function headers.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FADT_UPDATE_H_
#define FADT_UPDATE_H_

/** Gets Architecture specific bitmask for EObjNameSpaceArch namespace.

  @param [out] EArchObjFadtMask   Pointer to the Mask bit variable.

  @retval None
**/
VOID
EFIAPI
GetPlatformNameSpaceMask (
  OUT UINT64  *EArchObjFadtMask
  );

#endif // FADT_UPDATE_H_
