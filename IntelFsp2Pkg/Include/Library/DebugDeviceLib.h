/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DEBUG_DEVICE_LIB_H__
#define __DEBUG_DEVICE_LIB_H__

/**
  Returns the debug print device enable state.

  @return  Debug print device enable state.

**/
UINT8
EFIAPI
GetDebugPrintDeviceEnable (
  VOID
  );

#endif
