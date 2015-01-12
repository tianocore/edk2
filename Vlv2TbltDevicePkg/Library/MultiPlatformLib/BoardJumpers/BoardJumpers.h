/**@file
  Jumper setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   
**/

#ifndef _BOARDJUMPERS_H_
#define _BOARDJUMPERS_H_

#include <PiPei.h>
#include "PchAccess.h"
#include "PlatformBaseAddresses.h"

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Guid/PlatformInfo.h>

BOOLEAN
IsRecoveryJumper (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
);

#endif
