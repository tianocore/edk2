/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Library/BaseLib.h>
#include "InternalTdxProbe.h"

/**
  TDX only works in X64. So allways return -1 to indicate Non-Td.

  @return 0       TD guest
  @return others  Non-TD guest
**/
UINTN
EFIAPI
TdProbe (
  VOID
  )
{
  return PROBE_NOT_TD_GUEST;
}
