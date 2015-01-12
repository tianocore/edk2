/** @file
  Jumper setting for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include <BoardJumpers.h>

BOOLEAN
IsRecoveryJumper (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
)
{
  return FALSE;
}

BOOLEAN
IsManufacturingMode(
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
)
{
  return FALSE;
}
