/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <PiPei.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Ppi/ArmGlobalVariable.h>
#include <Ppi/MasterBootMode.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Ppi/GuidedSectionExtraction.h>
//
// The Library classes this module consumes
//
#include <Library/ArmPlatformLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>

#include <Guid/ArmGlobalVariableHob.h>

EFI_STATUS
EFIAPI
InitializePlatformPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  );

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

VOID
EFIAPI
BuildGlobalVariableHob (
  IN EFI_PHYSICAL_ADDRESS         GlobalVariableBase,
  IN UINT32                       GlobalVariableSize
  )
{
  EFI_STATUS                Status;
  ARM_HOB_GLOBAL_VARIABLE   *Hob;

  Status = PeiServicesCreateHob (EFI_HOB_TYPE_GUID_EXTENSION, sizeof (ARM_HOB_GLOBAL_VARIABLE), (VOID**)&Hob);
  if (!EFI_ERROR(Status)) {
    CopyGuid (&(Hob->Header.Name), &gArmGlobalVariableGuid);
    Hob->GlobalVariableBase = GlobalVariableBase;
    Hob->GlobalVariableSize = GlobalVariableSize;
  }
}

/*++

Routine Description:



Arguments:

  FileHandle  - Handle of the file being invoked.
  PeiServices - Describes the list of possible PEI Services.

Returns:

  Status -  EFI_SUCCESS if the boot mode could be set

--*/
EFI_STATUS
EFIAPI
InitializePlatformPeim (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                    Status;
  UINTN                         BootMode;
  ARM_GLOBAL_VARIABLE_PPI       *ArmGlobalVariablePpi;
  EFI_PHYSICAL_ADDRESS          GlobalVariableBase;

  DEBUG ((EFI_D_LOAD | EFI_D_INFO, "Platform PEIM Loaded\n"));

  PlatformPeim ();

  Status = PeiServicesLocatePpi (&gArmGlobalVariablePpiGuid, 0, NULL, (VOID**)&ArmGlobalVariablePpi);
  if (!EFI_ERROR(Status)) {
    Status = ArmGlobalVariablePpi->GetGlobalVariableMemory (&GlobalVariableBase);

    if (!EFI_ERROR(Status)) {
      // Declare the Global Variable HOB
      BuildGlobalVariableHob (GlobalVariableBase, FixedPcdGet32 (PcdPeiGlobalVariableSize));
    }
  }

  BootMode  = ArmPlatformGetBootMode ();
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
