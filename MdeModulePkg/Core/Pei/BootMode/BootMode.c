/** @file
  This module provide function for ascertaining and updating the boot mode:
  GetBootMode()
  SetBootMode()
  See PI Specification volume I, chapter 9 Boot Paths for additional information
  on the boot mode.
  
Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "PeiMain.h"

/**
  This service enables PEIMs to ascertain the present value of the boot mode.

  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param BootMode               A pointer to contain the value of the boot mode.

  @retval EFI_SUCCESS           The boot mode was returned successfully.
  @retval EFI_INVALID_PARAMETER BootMode is NULL.

**/
EFI_STATUS
EFIAPI
PeiGetBootMode (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  OUT   EFI_BOOT_MODE     *BootMode
  )
{
  PEI_CORE_INSTANCE             *PrivateData;    
  EFI_HOB_HANDOFF_INFO_TABLE    *HandOffHob;


  if (BootMode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);
  
  HandOffHob  = (PrivateData->HobList.HandoffInformationTable);
  
  *BootMode   = HandOffHob->BootMode;
  

  return EFI_SUCCESS;   
}


/**
  This service enables PEIMs to update the boot mode variable.


  @param PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param BootMode         The value of the boot mode to set.

  @return EFI_SUCCESS     The value was successfully updated

**/
EFI_STATUS
EFIAPI
PeiSetBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_BOOT_MODE           BootMode
  )
{
  PEI_CORE_INSTANCE                    *PrivateData;    
  EFI_HOB_HANDOFF_INFO_TABLE    *HandOffHob;


  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);
  
  HandOffHob  = (PrivateData->HobList.HandoffInformationTable);
  
  HandOffHob->BootMode = BootMode;


  return EFI_SUCCESS;   
}
