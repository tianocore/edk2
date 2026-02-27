/** @file
  Set the level of support for Hardware Error Record Persistence that is
  implemented by the platform.

Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include "Bds.h"

/**
  Set the HwErrRecSupport variable contains a binary UINT16 that supplies the
  level of support for Hardware Error Record Persistence that is implemented
  by the platform.

**/
VOID
InitializeHwErrRecSupport (
  VOID
  );
