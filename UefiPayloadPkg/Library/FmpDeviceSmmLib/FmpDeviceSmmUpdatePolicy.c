/** @file
  Internal policy helpers for SMMSTORE-backed firmware updates.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FmpDeviceSmmUpdatePolicy.h"

/**
  Check whether two flash ranges overlap or are malformed.

  @param[in] FirstOffset   First range offset.
  @param[in] FirstSize     First range size.
  @param[in] SecondOffset  Second range offset.
  @param[in] SecondSize    Second range size.

  @retval TRUE   The ranges overlap or are malformed.
  @retval FALSE  The ranges are valid and separate.
**/
BOOLEAN
FmpDeviceFlashRangesOverlap (
  IN UINTN  FirstOffset,
  IN UINTN  FirstSize,
  IN UINTN  SecondOffset,
  IN UINTN  SecondSize
  )
{
  if ((FirstSize == 0) || (SecondSize == 0) ||
      (FirstOffset > (MAX_UINTN - FirstSize)) ||
      (SecondOffset > (MAX_UINTN - SecondSize)))
  {
    return TRUE;
  }

  return (FirstOffset < (SecondOffset + SecondSize)) &&
         (SecondOffset < (FirstOffset + FirstSize));
}

/**
  Determine whether variable services must be disabled after an update.

  @param[in] VariableStorePreserved  Whether the variable store was preserved.

  @retval TRUE   Disable variable services.
  @retval FALSE  Keep variable services enabled.
**/
BOOLEAN
FmpDeviceShouldDisableVariableServices (
  IN BOOLEAN  VariableStorePreserved
  )
{
  return !VariableStorePreserved;
}
