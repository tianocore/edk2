/** @file
  TdxProbeLib definitions

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _TDX_PROBE_LIB_H_
#define _TDX_PROBE_LIB_H_

#include <Library/BaseLib.h>

/**
  Whether Intel TDX is enabled.

  @return TRUE    TDX enabled
  @return FALSE   TDX not enabled
**/
BOOLEAN
EFIAPI
TdxIsEnabled (
  VOID);

#endif
