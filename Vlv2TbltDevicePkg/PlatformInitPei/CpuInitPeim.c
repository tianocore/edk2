/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


    CpuInitPeim.c

Abstract:

    Functions for LpcSio initilization
    It is needed for early onboard LAN controller disable/enable in platform setup.

--*/

#include "PlatformEarlyInit.h"


EFI_STATUS
PlatformCpuInit (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  IN EFI_PLATFORM_CPU_INFO       *PlatformCpuInfo
  )
{
  BOOLEAN                     ResetRequired;

  //
  // Variable initialization
  //
  ResetRequired = FALSE;


  if (ResetRequired) {
    CpuOnlyReset(PeiServices);
  }

  return EFI_SUCCESS;
}
