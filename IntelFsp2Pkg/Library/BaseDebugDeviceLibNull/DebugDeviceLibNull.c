/** @file
  Debug device library instance that retrieves the current enabling state for
  the platform debug output device.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>

/**
  Returns the debug print device enable state.

  @return  Debug print device enable state.

**/
UINT8
EFIAPI
GetDebugPrintDeviceEnable (
  VOID
  )
{
  return 1;
}
