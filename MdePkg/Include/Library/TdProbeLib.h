/** @file

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TD_PROBE_LIB_H_
#define TD_PROBE_LIB_H_

#define TD_PROBE_NON  0
#define TD_PROBE_TDX  1

/**
  Probe if it is Tdx guest.

  @return TD_PROBE_TDX if it is Tdx guest. Otherwise return TD_PROBE_NON.

**/
UINTN
EFIAPI
TdProbe (
  VOID
  );

#endif
