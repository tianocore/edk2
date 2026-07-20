/** @file
  Internal policy helpers for SMMSTORE-backed firmware updates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Base.h>

BOOLEAN
FmpDeviceFlashRangesOverlap (
  IN UINTN  FirstOffset,
  IN UINTN  FirstSize,
  IN UINTN  SecondOffset,
  IN UINTN  SecondSize
  );

BOOLEAN
FmpDeviceShouldDisableVariableServices (
  IN BOOLEAN  VariableStorePreserved
  );
