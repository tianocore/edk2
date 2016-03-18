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

/**
  Resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of WatchdogData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                            the data buffer starts with a Null-terminated string, optionally
                            followed by additional binary data. The string is a description
                            that the caller may use to further indicate the reason for the
                            system reset. ResetData is only valid if ResetStatus is something
                            other than EFI_SUCCESS unless the ResetType is EfiResetPlatformSpecific
                            where a minimum amount of ResetData is always required.

**/
VOID
EFIAPI
PeiResetSystem2 (
  IN EFI_RESET_TYPE     ResetType,
  IN EFI_STATUS         ResetStatus,
  IN UINTN              DataSize,
  IN VOID               *ResetData OPTIONAL
  )
{
  EFI_STATUS            Status;
  EFI_PEI_RESET2_PPI    *Reset2Ppi;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReset2PpiGuid,
             0,
             NULL,
             (VOID **)&Reset2Ppi
             );

  if (!EFI_ERROR (Status)) {
    Reset2Ppi->ResetSystem (ResetType, ResetStatus, DataSize, ResetData);
    return;
  }

  //
  // Report Status Code that Reset2 PPI is not available.
  //
  REPORT_STATUS_CODE (
    EFI_ERROR_CODE | EFI_ERROR_MINOR,
    (EFI_SOFTWARE_PEI_CORE | EFI_SW_PS_EC_RESET_NOT_AVAILABLE)
    );
}

