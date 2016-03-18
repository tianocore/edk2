/** @file
  Debug device library instance that retrieves the current enabling state for
  the platform debug output device.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
