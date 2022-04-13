/** @file

  Null stub of TdProbeLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/TdProbeLib.h>

/**
  Probe if it is Tdx guest.

  @return TD_PROBE_TDX if it is Tdx guest. Otherwise return TD_PROBE_NON.

**/
UINTN
EFIAPI
TdProbe (
  VOID
  )
{
  return TD_PROBE_NON;
}
