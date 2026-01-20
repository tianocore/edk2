/** @file
  IA32/X64 specific Unit Test Host functions.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c), Microsoft Corporation.
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
/// Structure of hook functions for BaseLib functions that can not be used from
/// a host application.  A simple emulation of these function is provided by
/// default.  A specific unit test can provide its own implementation for any
/// of these functions.
///
UNIT_TEST_HOST_BASE_LIB  gUnitTestHostBaseLib = {
  &mUnitTestHostBaseLibCommon
};
