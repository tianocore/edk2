/** @file
  Dectection code for hypervisor debug port.

  Copyright (c) 2020, Citrix Systems, Inc.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DebugLibDetect.h"

/**
  Always return TRUE without detection as the debug I/O port is always
  present.

  @retval TRUE   The debug I/O port is always present.

**/
BOOLEAN
EFIAPI
PlatformDebugLibIoPortDetect (
  VOID
  )
{
  return TRUE;
}
