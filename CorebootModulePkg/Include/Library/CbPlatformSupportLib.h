/** @file
  Coreboot Platform Support library. Platform can provide an implementation of this
  library class to provide hooks that may be required for some type of 
  platform features.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CB_PLATFORM_SUPPORT_LIB__
#define __CB_PLATFORM_SUPPORT_LIB__

/**
  Parse platform specific information from coreboot. 

  @retval RETURN_SUCCESS       The platform specific coreboot support succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific coreboot support could not be completed.
 
**/
EFI_STATUS
EFIAPI
CbParsePlatformInfo (
  VOID
  );

#endif // __CB_PLATFORM_SUPPORT_LIB__

