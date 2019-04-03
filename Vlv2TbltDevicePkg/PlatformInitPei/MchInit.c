/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:


    MchInit.c

Abstract:


--*/


#include "PlatformEarlyInit.h"

#define PSE_PAGE_SIZE 0x400000   // 4MB

extern  BOOLEAN ImageInMemory;


VOID
EfiCommonLibEnablePsePaging (
  IN UINT32   PDBR
  );

VOID
EfiCommonLibDisablePsePaging (
  );

/**

  Initialize the MCH Thermal Sensor

**/
VOID
InitMchThermalSensor()
{
}

/**

  Programs and enables the CRID for MCH and ICH

**/
VOID
ProgramMchCRID(
  IN CONST EFI_PEI_SERVICES            **PeiServices
  )
{
}

/**

  Initialize the GPIO IO selection, GPIO USE selection, and GPIO signal inversion registers

**/
VOID
MchInit (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{

  return;
}


