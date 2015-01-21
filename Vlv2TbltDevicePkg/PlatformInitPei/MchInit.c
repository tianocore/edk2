/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

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


