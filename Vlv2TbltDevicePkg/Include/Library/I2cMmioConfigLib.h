/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

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
