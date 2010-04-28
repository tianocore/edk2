/**@file

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BootMode.c
   
Abstract:

  Tiano PEIM to provide the platform support functionality within Windows

**/



//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/MasterBootMode.h>
#include <Ppi/BootInRecoveryMode.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>


//
// Module globals
//
EFI_PEI_PPI_DESCRIPTOR  mPpiListBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mPpiListRecoveryBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
};

EFI_STATUS
EFIAPI
InitializeBootMode (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
/*++

Routine Description:

  Peform the boot mode determination logic

Arguments:

  FileHandle  - Handle of the file being invoked.
  PeiServices - Describes the list of possible PEI Services.
    
Returns:

  Status -  EFI_SUCCESS if the boot mode could be set

--*/
{
  EFI_STATUS  Status;
  UINTN       BootMode;

  DEBUG ((EFI_D_ERROR, "NT32 Boot Mode PEIM Loaded\n"));

  //
  // Let's assume things are OK if not told otherwise
  // Should we read an environment variable in order to easily change this?
  //
  BootMode  = BOOT_WITH_FULL_CONFIGURATION;

  Status    = (**PeiServices).SetBootMode (PeiServices, (UINT8) BootMode);
  ASSERT_EFI_ERROR (Status);

  Status = (**PeiServices).InstallPpi (PeiServices, &mPpiListBootMode);
  ASSERT_EFI_ERROR (Status);

  if (BootMode == BOOT_IN_RECOVERY_MODE) {
    Status = (**PeiServices).InstallPpi (PeiServices, &mPpiListRecoveryBootMode);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
