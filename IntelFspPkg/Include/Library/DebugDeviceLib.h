/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
