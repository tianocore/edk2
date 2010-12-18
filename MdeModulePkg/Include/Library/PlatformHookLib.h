/** @file
  Platform hook library. Platform can provide an implementation of this
  library class to provide hooks that may be required for some type of 
  platform initialization.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PLATFORM_HOOK_LIB__
#define __PLATFORM_HOOK_LIB__

/**
  Performs platform specific initialization required for the CPU to access
  the hardware associated with a SerialPortLib instance.  This function does
  not intiailzie the serial port hardware itself.  Instead, it initializes 
  hardware devices that are required for the CPU to access the serial port 
  hardware.  This function may be called more than once.

  @retval RETURN_SUCCESS       The platform specific initialization succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific initialization could not be completed.
 
**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  );

#endif // __PLATFORM_HOOK_LIB__

