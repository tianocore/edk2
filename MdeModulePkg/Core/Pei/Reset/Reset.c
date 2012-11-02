/** @file
  Pei Core Reset System Support
  
Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "PeiMain.h"

/**

  Core version of the Reset System


  @param PeiServices                An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.

  @retval EFI_NOT_AVAILABLE_YET     PPI not available yet.
  @retval EFI_DEVICE_ERROR          Did not reset system.
                                    Otherwise, resets the system.

**/
EFI_STATUS
EFIAPI
PeiResetSystem (
  IN CONST EFI_PEI_SERVICES         **PeiServices
  )
{
  EFI_STATUS        Status;
  EFI_PEI_RESET_PPI *ResetPpi;

  Status = PeiServicesLocatePpi (
             &gEfiPeiResetPpiGuid,         
             0,                         
             NULL,                      
             (VOID **)&ResetPpi                  
             );

  //
  // LocatePpi returns EFI_NOT_FOUND on error
  //
  if (!EFI_ERROR (Status)) {
    return ResetPpi->ResetSystem (PeiServices);
  } 
  //
  // Report Status Code that Reset PPI is not available
  //
  REPORT_STATUS_CODE (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    (EFI_SOFTWARE_PEI_CORE | EFI_SW_PS_EC_RESET_NOT_AVAILABLE)
    );
  return  EFI_NOT_AVAILABLE_YET;
}

