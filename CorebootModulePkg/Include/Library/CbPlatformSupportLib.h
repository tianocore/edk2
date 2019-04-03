/** @file
  Coreboot Platform Support library. Platform can provide an implementation of this
  library class to provide hooks that may be required for some type of 
  platform features.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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

