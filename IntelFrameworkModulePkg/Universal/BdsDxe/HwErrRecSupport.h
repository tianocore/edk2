/** @file
  Set the level of support for Hardware Error Record Persistence that is
  implemented by the platform.

Copyright (c) 2007 - 2008, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HW_ERR_REC_SUPPORT_H_
#define _HW_ERR_REC_SUPPORT_H_

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

#endif
