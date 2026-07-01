/** @file
  AArch64-specific UnitTestHostBaseLib globals.

  Copyright (c) 2026 NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "../UnitTestHost.h"

STATIC UNIT_TEST_HOST_BASE_LIB_COMMON  mUnitTestHostBaseLibCommon = {
  UnitTestHostBaseLibEnableInterrupts,
  UnitTestHostBaseLibDisableInterrupts,
  UnitTestHostBaseLibEnableDisableInterrupts,
  UnitTestHostBaseLibGetInterruptState,
};

UNIT_TEST_HOST_BASE_LIB  gUnitTestHostBaseLib = {
  &mUnitTestHostBaseLibCommon,
};
