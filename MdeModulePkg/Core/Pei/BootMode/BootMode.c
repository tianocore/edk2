/** @file

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BootMode.c

Abstract:

  EFI PEI Core Boot Mode services



Revision History

**/

#include <PeiMain.h>

EFI_STATUS
EFIAPI
PeiGetBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  OUT EFI_BOOT_MODE          *BootMode
  )
/*++

Routine Description:

  This service enables PEIMs to ascertain the present value of the boot mode.  

Arguments:

  PeiServices    - The PEI core services table.
  BootMode       - A pointer to contain the value of the boot mode. 

Returns:

  EFI_SUCCESS           - The boot mode was returned successfully.
  EFI_INVALID_PARAMETER - BootMode is NULL.

--*/
{
  PEI_CORE_INSTANCE                    *PrivateData;    
  EFI_HOB_HANDOFF_INFO_TABLE    *HandOffHob;


  if (BootMode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);
  
  HandOffHob  = (PrivateData->HobList.HandoffInformationTable);
  
  *BootMode   = HandOffHob->BootMode;
  

  return EFI_SUCCESS;   
}


EFI_STATUS
EFIAPI
PeiSetBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_BOOT_MODE           BootMode
  )
/*++

Routine Description:

  This service enables PEIMs to update the boot mode variable.    

Arguments:

  PeiServices    - The PEI core services table.
  BootMode       - The value of the boot mode to set.

Returns:

  EFI_SUCCESS    - The value was successfully updated

--*/
{
  PEI_CORE_INSTANCE                    *PrivateData;    
  EFI_HOB_HANDOFF_INFO_TABLE    *HandOffHob;


  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);
  
  HandOffHob  = (PrivateData->HobList.HandoffInformationTable);
  
  HandOffHob->BootMode = BootMode;


  return EFI_SUCCESS;   
}
