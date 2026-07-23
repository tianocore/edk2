/** @file
  Internal policy helpers for SMMSTORE-backed firmware updates.

  Copyright (c) 2026, Star Labs Systems. All rights reserved.<BR>
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
