/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

**/

#ifndef __MMIO_CONFIG_LIB_H__
#define __MMIO_CONFIG_LIB_H__

#include <Protocol/MmioDevice.h>

///
/// Declare the memory mapped I/O devices assocaited with the
/// board.
///
extern CONST EFI_MMIO_DEVICE_PROTOCOL gMmioDeviceList [ ];
extern CONST UINTN gMmioDeviceCount;

#endif  //  __MMIO_CONFIG_LIB_H__
