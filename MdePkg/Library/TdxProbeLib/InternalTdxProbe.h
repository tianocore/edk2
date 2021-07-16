/** @file
  Copyright (c) 2020 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _INTERNAL_TDX_PROBE_H_
#define _INTERNAL_TDX_PROBE_H_

/**
  The internal Td Probe implementation.

  @return 0       TD guest
  @return others  Non-TD guest
**/
UINTN
EFIAPI
TdProbe (
  VOID
  );

#endif
