/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

--*/

#ifndef _PLATFORM_FSP_LIB_H
#define _PLATFORM_FSP_LIB_H
#include <Base.h>
#include <Uefi.h>

extern
EFI_STATUS
PlatformHobCreateFromFsp (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  VOID                          *HobList
  );

#endif
