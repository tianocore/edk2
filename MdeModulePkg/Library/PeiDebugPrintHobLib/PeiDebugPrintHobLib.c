/** @file
  NULL Library class that reads Debug Mask variable and if it exists makes a
  HOB that contains the debug mask.

  Copyright (c) 2011, Apple, Inc. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>

#include <Ppi/ReadOnlyVariable2.h>
#include <Guid/DebugMask.h>


/**
  The constructor reads variable and sets HOB
  
  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval EFI_SUCCESS  The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiDebugPrintHobLibConstructor (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *Variable;
  UINTN                           Size;
  UINT64                          GlobalErrorLevel;
  UINT32                          HobErrorLevel;
  
  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&Variable
             );
  if (!EFI_ERROR (Status)) {
    Size = sizeof (GlobalErrorLevel);
    Status = Variable->GetVariable ( 
                         Variable, 
                         DEBUG_MASK_VARIABLE_NAME,
                         &gEfiGenericVariableGuid,
                         NULL,
                         &Size,
                         &GlobalErrorLevel
                         );
    if (!EFI_ERROR (Status)) {
      //
      // Build the GUID'ed HOB for DXE
      //
      HobErrorLevel = (UINT32)GlobalErrorLevel;
      BuildGuidDataHob (
        &gEfiGenericVariableGuid,
        &HobErrorLevel,
        sizeof (HobErrorLevel)
        );
    }
  }

  return EFI_SUCCESS;
}
