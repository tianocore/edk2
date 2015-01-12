/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  PlatformSsaInitPeim.c

Abstract:


--*/

#include "PlatformEarlyInit.h"

/**
  Perform SSA related platform initialization.

**/
VOID
PlatformSsaInit (
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{

  DEBUG ((EFI_D_ERROR, "PlatformSsaInit() - Start\n"));
  DEBUG ((EFI_D_ERROR, "PlatformSsaInit() - SystemConfiguration->ISPDevSel 0x%x\n",SystemConfiguration->ISPDevSel));
  if(SystemConfiguration->ISPDevSel == 0x02)
  {
    //
    // Device 3 Interrupt Route
    //
    MmioWrite16 (
      (ILB_BASE_ADDRESS + R_PCH_ILB_D3IR),
      V_PCH_ILB_DXXIR_IAR_PIRQH   // For IUNIT
    );
    MmioRead16(ILB_BASE_ADDRESS + R_PCH_ILB_D3IR); // Read Posted Writes Register
    DEBUG ((EFI_D_ERROR, "PlatformSsaInit() - Device 3 Interrupt Route Done\n"));
  }

  //
  // Device 2 Interrupt Route
  //
  MmioWrite16 (
    (ILB_BASE_ADDRESS + R_PCH_ILB_D2IR),
    V_PCH_ILB_DXXIR_IAR_PIRQA   // For IGD
  );
  MmioRead16(ILB_BASE_ADDRESS + R_PCH_ILB_D2IR); // Read Posted Writes Register
  DEBUG ((EFI_D_ERROR, "PlatformSsaInit() - Device 2 Interrupt Route Done\n"));

  return;
}
