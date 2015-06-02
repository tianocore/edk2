/** @file
  MicroSecondDelay implementation of ACPI Timer.
  
  Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                               
--*/

#include "PiPei.h"
#include "I2CAccess.h"
#include "I2CDelayPei.h"
#include <Library/DebugLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/Stall.h>

/**
  Stalls the CPU for at least the given number of microseconds.
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{

  EFI_PEI_STALL_PPI              *StallPpi;
  EFI_STATUS                     Status;
  CONST EFI_PEI_SERVICES         **PeiServices;
  
  PeiServices = GetPeiServicesTablePointer();


  Status = (**PeiServices).LocatePpi (PeiServices, &gEfiPeiStallPpiGuid, 0, NULL, &StallPpi);
  ASSERT(!EFI_ERROR(Status));

  StallPpi->Stall (PeiServices, StallPpi, MicroSeconds);

  return EFI_SUCCESS;
}
