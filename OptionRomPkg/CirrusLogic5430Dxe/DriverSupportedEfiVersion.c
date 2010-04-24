/** @file
  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DriverSupportEfiVersion.c

**/
#include "CirrusLogic5430.h"

EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL gCirrusLogic5430DriverSupportedEfiVersion = {
  sizeof (EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL), // Size of Protocol structure.
  0                                                   // Version number to be filled at start up.
};

