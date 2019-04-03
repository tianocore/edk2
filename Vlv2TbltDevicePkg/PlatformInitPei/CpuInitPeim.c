/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

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
