/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  PlatformInfoDxe.c

Abstract:
  Platform Info driver to public platform related HOB data

--*/

#include "PlatformInfoDxe.h"

/**
  Entry point for the driver.

  This routine get the platform HOB data from PEI and publish
  as Platform Info variable that can be accessed during boot service and
  runtime.

  @param ImageHandle    Image Handle.
  @param SystemTable    EFI System Table.

  @retval Status        Function execution status.

**/
EFI_STATUS
EFIAPI
PlatformInfoInit (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_PLATFORM_INFO_HOB       *PlatformInfoHobPtr;
  EFI_PEI_HOB_POINTERS        GuidHob;
  EFI_PLATFORM_INFO_HOB       TmpHob;
  UINTN                       VarSize;
  EFI_OS_SELECTION_HOB        *OsSlectionHobPtr;
  UINT8                       Selection;
  SYSTEM_CONFIGURATION        SystemConfiguration;
  UINT8                       *LpssDataHobPtr;
  UINT8                       *LpssDataVarPtr;
  UINTN                       i;

  VarSize = sizeof(SYSTEM_CONFIGURATION);
  Status = gRT->GetVariable(
                  NORMAL_SETUP_NAME,
                  &gEfiNormalSetupGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );
  
  if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VarSize = sizeof(SYSTEM_CONFIGURATION);
    Status = gRT->GetVariable(
              L"SetupRecovery",
              &gEfiNormalSetupGuid,
              NULL,
              &VarSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }    

  VarSize = sizeof(Selection);
  Status = gRT->GetVariable(
                  L"OsSelection",
                  &gOsSelectionVariableGuid,
                  NULL,
                  &VarSize,
                  &Selection
                  );

  if (EFI_ERROR(Status)) {
    Selection = SystemConfiguration.ReservedO;
    Status = gRT->SetVariable (
                    L"OsSelection",
                    &gOsSelectionVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof(Selection),
                    &Selection
                    );
  }

  GuidHob.Raw = GetHobList ();
  if (GuidHob.Raw != NULL) {
    if ((GuidHob.Raw = GetNextGuidHob (&gOsSelectionVariableGuid, GuidHob.Raw)) != NULL) {
      OsSlectionHobPtr = GET_GUID_HOB_DATA (GuidHob.Guid);

      if (OsSlectionHobPtr->OsSelectionChanged) {
        SystemConfiguration.ReservedO = OsSlectionHobPtr->OsSelection;

        //
        // Load Audio default configuration
        //
        SystemConfiguration.Lpe         = OsSlectionHobPtr->Lpe;
        SystemConfiguration.PchAzalia   = OsSlectionHobPtr->PchAzalia;

        //
        // Load LPSS and SCC default configurations
        //
        LpssDataHobPtr = &OsSlectionHobPtr->LpssData.LpssPciModeEnabled;
        LpssDataVarPtr = &SystemConfiguration.LpssPciModeEnabled;
        for (i = 0; i < sizeof(EFI_PLATFORM_LPSS_DATA); i++) {
          *LpssDataVarPtr = *LpssDataHobPtr;
          LpssDataVarPtr++;
          LpssDataHobPtr++;
        }

        SystemConfiguration.GOPEnable = TRUE;

        Status = gRT->SetVariable (
                        NORMAL_SETUP_NAME,
                        &gEfiNormalSetupGuid,
                        EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                        sizeof(SYSTEM_CONFIGURATION),
                        &SystemConfiguration
                        );
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  GuidHob.Raw = GetHobList ();
  if (GuidHob.Raw == NULL) {
  	return EFI_NOT_FOUND;
  }

  if ((GuidHob.Raw = GetNextGuidHob (&gEfiPlatformInfoGuid, GuidHob.Raw)) != NULL) {
    PlatformInfoHobPtr = GET_GUID_HOB_DATA (GuidHob.Guid);
    VarSize = sizeof(EFI_PLATFORM_INFO_HOB);
    Status = gRT->GetVariable(
                    L"PlatformInfo",
                    &gEfiVlv2VariableGuid,
                    NULL,
                    &VarSize,
                    &TmpHob
                    );

    if (EFI_ERROR(Status) || CompareMem (&TmpHob, PlatformInfoHobPtr, VarSize)) {

      //
      // Write the Platform Info to volatile memory
      //
      Status = gRT->SetVariable(
                      L"PlatformInfo",
                      &gEfiVlv2VariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                      sizeof(EFI_PLATFORM_INFO_HOB),
                      PlatformInfoHobPtr
                      );
      if (EFI_ERROR(Status)) {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

