/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

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
