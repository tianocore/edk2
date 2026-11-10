/** @file
  LoongArch64 specific Unit Test Host functions.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation.
  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UnitTestHost.h"

///
/// Common services
///
STATIC UNIT_TEST_HOST_BASE_LIB_COMMON  mUnitTestHostBaseLibCommon = {
  UnitTestHostBaseLibEnableInterrupts,
  UnitTestHostBaseLibDisableInterrupts,
  UnitTestHostBaseLibEnableDisableInterrupts,
  UnitTestHostBaseLibGetInterruptState,
};

///
/// Host applications emulate interrupt state instead of accessing privileged
/// CPU registers. Unit tests can override these hooks.
///
UNIT_TEST_HOST_BASE_LIB  gUnitTestHostBaseLib = {
  &mUnitTestHostBaseLibCommon
};
