/** @file
  Internal policy helpers for SMMSTORE-backed firmware updates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FmpDeviceSmmUpdatePolicy.h"

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

BOOLEAN
FmpDeviceShouldDisableVariableServices (
  IN BOOLEAN  VariableStorePreserved
  )
{
  return !VariableStorePreserved;
}
