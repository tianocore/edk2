/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  BootMode.c

Abstract:

  EFI PEIM to provide the platform support functionality on the Thurley.


--*/
#include "CommonHeader.h"
#include "Platform.h"
#include "PlatformBaseAddresses.h"
#include "PchAccess.h"
#include "PlatformBootMode.h"
#include <Guid/SetupVariable.h>

#include <Guid/BootState.h>

//
// Priority of our boot modes, highest priority first
//
EFI_BOOT_MODE mBootModePriority[] = {
  BOOT_IN_RECOVERY_MODE,
  BOOT_WITH_DEFAULT_SETTINGS,
  BOOT_ON_FLASH_UPDATE,
  BOOT_ON_S2_RESUME,
  BOOT_ON_S3_RESUME,
  BOOT_ON_S4_RESUME,
  BOOT_WITH_MINIMAL_CONFIGURATION,
  BOOT_ASSUMING_NO_CONFIGURATION_CHANGES,
  BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS,
  BOOT_WITH_FULL_CONFIGURATION,
  BOOT_ON_S5_RESUME
};

EFI_PEI_NOTIFY_DESCRIPTOR mCapsuleNotifyList[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gPeiCapsulePpiGuid,
    CapsulePpiNotifyCallback
  }
};

BOOLEAN
GetSleepTypeAfterWakeup (
  IN  CONST EFI_PEI_SERVICES          **PeiServices,
  OUT UINT16                    *SleepType
  );

EFI_STATUS
EFIAPI
CapsulePpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS      Status;
  EFI_BOOT_MODE   BootMode;
  PEI_CAPSULE_PPI *Capsule;

  Status = (*PeiServices)->GetBootMode((const EFI_PEI_SERVICES **)PeiServices, &BootMode);
  ASSERT_EFI_ERROR (Status);

  if (BootMode == BOOT_ON_S3_RESUME) {
    //
    // Determine if we're in capsule update mode
    //
    Status = (*PeiServices)->LocatePpi ((const EFI_PEI_SERVICES **)PeiServices,
                                        &gPeiCapsulePpiGuid,
                                        0,
                                        NULL,
                                        (VOID **)&Capsule
                                        );

    if (Status == EFI_SUCCESS) {
      if (Capsule->CheckCapsuleUpdate ((EFI_PEI_SERVICES**)PeiServices) == EFI_SUCCESS) {
        BootMode = BOOT_ON_FLASH_UPDATE;
        Status = (*PeiServices)->SetBootMode((const EFI_PEI_SERVICES **)PeiServices, BootMode);
        ASSERT_EFI_ERROR (Status);
      }
    }
  }

  return Status;
}

/**
  Check CMOS register bit to determine if previous boot was successful

  @param PeiServices    pointer to the PEI Service Table

  @retval TRUE          - Previous Boot was success
  @retval FALSE         - Previous Boot wasn't success

**/
BOOLEAN
IsPreviousBootSuccessful(
  IN CONST EFI_PEI_SERVICES   **PeiServices

  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         BootState;
  UINTN                           DataSize;
  CHAR16                          VarName[] = BOOT_STATE_VARIABLE_NAME;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *PeiVar;

  Status = (**PeiServices).LocatePpi (
                             PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                                (void **)&PeiVar
                             );
  ASSERT_EFI_ERROR (Status);

  //
  // Get last Boot State Variable to confirm that it is not a first boot .
  //

  DataSize = sizeof (BOOLEAN);
  Status = PeiVar->GetVariable (
                     PeiVar,
                     VarName,
                     &gEfiBootStateGuid,
                     NULL,
                     &DataSize,
                     &BootState
                     );
  if (EFI_ERROR (Status) || (BootState == TRUE)) {
    return FALSE;
  }

  DEBUG ((EFI_D_INFO, "Previous boot cycle successfully completed handover to OS\n"));
  return TRUE;
}
#ifdef NOCS_S3_SUPPORT
EFI_STATUS
UpdateBootMode (
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS      Status;
  EFI_BOOT_MODE   BootMode;
  UINT16          SleepType;
  CHAR16          *strBootMode;

  Status = (*PeiServices)->GetBootMode(PeiServices, &BootMode);
  ASSERT_EFI_ERROR (Status);
  if (BootMode  == BOOT_IN_RECOVERY_MODE){
    return Status;
  }

  //
  // Let's assume things are OK if not told otherwise
  //
  BootMode = BOOT_WITH_FULL_CONFIGURATION;

  if (GetSleepTypeAfterWakeup (PeiServices, &SleepType)) {
    switch (SleepType) {
      case V_PCH_ACPI_PM1_CNT_S3:
        BootMode = BOOT_ON_S3_RESUME;
        Status = (*PeiServices)->NotifyPpi (PeiServices, &mCapsuleNotifyList[0]);
        ASSERT_EFI_ERROR (Status);
        break;

      case V_PCH_ACPI_PM1_CNT_S4:
        BootMode = BOOT_ON_S4_RESUME;
        break;

      case V_PCH_ACPI_PM1_CNT_S5:
        BootMode = BOOT_ON_S5_RESUME;
        break;
    } // switch (SleepType)
  }

  if (IsFastBootEnabled (PeiServices) && IsPreviousBootSuccessful (PeiServices)) {
    DEBUG ((EFI_D_INFO, "Prioritizing Boot mode to BOOT_WITH_MINIMAL_CONFIGURATION\n"));
    PrioritizeBootMode (&BootMode, BOOT_WITH_MINIMAL_CONFIGURATION);
  }

  switch (BootMode) {
    case BOOT_WITH_FULL_CONFIGURATION:
      strBootMode = L"BOOT_WITH_FULL_CONFIGURATION";
      break;
    case BOOT_WITH_MINIMAL_CONFIGURATION:
      strBootMode = L"BOOT_WITH_MINIMAL_CONFIGURATION";
      break;
    case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
      strBootMode = L"BOOT_ASSUMING_NO_CONFIGURATION_CHANGES";
      break;
    case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
      strBootMode = L"BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS";
      break;
    case BOOT_WITH_DEFAULT_SETTINGS:
      strBootMode = L"BOOT_WITH_DEFAULT_SETTINGS";
      break;
    case BOOT_ON_S4_RESUME:
      strBootMode = L"BOOT_ON_S4_RESUME";
      break;
    case BOOT_ON_S5_RESUME:
      strBootMode = L"BOOT_ON_S5_RESUME";
      break;
    case BOOT_ON_S2_RESUME:
      strBootMode = L"BOOT_ON_S2_RESUME";
      break;
    case BOOT_ON_S3_RESUME:
      strBootMode = L"BOOT_ON_S3_RESUME";

      break;
    case BOOT_ON_FLASH_UPDATE:
      strBootMode = L"BOOT_ON_FLASH_UPDATE";
      break;
    case BOOT_IN_RECOVERY_MODE:
      strBootMode = L"BOOT_IN_RECOVERY_MODE";
      break;
    default:
      strBootMode = L"Unknown boot mode";
  } // switch (BootMode)

  DEBUG ((EFI_D_ERROR, "Setting BootMode to %s\n", strBootMode));
  Status = (*PeiServices)->SetBootMode(PeiServices, BootMode);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
#endif

/**
  Get sleep type after wakeup

  @param PeiServices       Pointer to the PEI Service Table.
  @param SleepType         Sleep type to be returned.

  @retval TRUE              A wake event occured without power failure.
  @retval FALSE             Power failure occured or not a wakeup.

**/
BOOLEAN
GetSleepTypeAfterWakeup (
  IN  CONST EFI_PEI_SERVICES          **PeiServices,
  OUT UINT16                    *SleepType
  )
{
  UINT16  Pm1Sts;
  UINT16  Pm1Cnt;
  UINT16  GenPmCon1;
  //
  // VLV BIOS Specification 0.6.2 - Section 18.4, "Power Failure Consideration"
  //
  // When the SUS_PWR_FLR bit is set, it indicates the SUS well power is lost.
  // This bit is in the SUS Well and defaults to 1’b1 based on RSMRST# assertion (not cleared by any type of reset).
  // System BIOS should follow cold boot path if SUS_PWR_FLR (PBASE + 0x20[14]),
  // GEN_RST_STS (PBASE + 0x20[9]) or PWRBTNOR_STS (ABASE + 0x00[11]) is set to 1’b1
  // regardless of the value in the SLP_TYP (ABASE + 0x04[12:10]) field.
  //
  GenPmCon1 = MmioRead16 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1);

  //
  // Read the ACPI registers
  //
  Pm1Sts  = IoRead16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_STS);
  Pm1Cnt  = IoRead16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_CNT);

  if ((GenPmCon1 & (B_PCH_PMC_GEN_PMCON_SUS_PWR_FLR | B_PCH_PMC_GEN_PMCON_GEN_RST_STS)) ||
     (Pm1Sts & B_PCH_ACPI_PM1_STS_PRBTNOR)) {
	  //
    // If power failure indicator, then don't attempt s3 resume.
    // Clear PM1_CNT of S3 and set it to S5 as we just had a power failure, and memory has
    // lost already.  This is to make sure no one will use PM1_CNT to check for S3 after
    // power failure.
	  //
    if ((Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP) == V_PCH_ACPI_PM1_CNT_S3) {
      Pm1Cnt = ((Pm1Cnt & ~B_PCH_ACPI_PM1_CNT_SLP_TYP) | V_PCH_ACPI_PM1_CNT_S5);
      IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_CNT, Pm1Cnt);
    }
	  //
    // Clear Wake Status (WAK_STS)
    //
  }
  //
  // Get sleep type if a wake event occurred and there is no power failure
  //
  if ((Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP) == V_PCH_ACPI_PM1_CNT_S3) {
    *SleepType = Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP;
    return TRUE;
  } else if ((Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP) == V_PCH_ACPI_PM1_CNT_S4) {
    *SleepType = Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP;
    return TRUE;
  }
  return FALSE;
}

BOOLEAN
EFIAPI
IsFastBootEnabled (
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *PeiReadOnlyVarPpi;
  UINTN                           VarSize;
  SYSTEM_CONFIGURATION            SystemConfiguration;
  BOOLEAN                         FastBootEnabledStatus;

  FastBootEnabledStatus = FALSE;
  Status = (**PeiServices).LocatePpi (
                             PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                             (void **)&PeiReadOnlyVarPpi
                             );
  if (Status == EFI_SUCCESS) {
    VarSize = sizeof (SYSTEM_CONFIGURATION);
    Status = PeiReadOnlyVarPpi->GetVariable (
                                  PeiReadOnlyVarPpi,
                                  PLATFORM_SETUP_VARIABLE_NAME,
                                  &gEfiSetupVariableGuid,
                                  NULL,
                                  &VarSize,
                                  &SystemConfiguration
                                  );
    if (Status == EFI_SUCCESS) {
      if (SystemConfiguration.FastBoot != 0) {
        FastBootEnabledStatus = TRUE;
      }
    }
  }

  return FastBootEnabledStatus;
}

/**
  Given the current boot mode, and a proposed new boot mode, determine
  which has priority. If the new boot mode has higher priority, then
  make it the current boot mode.

  @param CurrentBootMode    pointer to current planned boot mode
  @param NewBootMode        proposed boot mode

  @retval EFI_NOT_FOUND      if either boot mode is not recognized
  @retval EFI_SUCCESS        if both boot mode values were recognized and
                             were processed.
**/
EFI_STATUS
PrioritizeBootMode (
  IN OUT EFI_BOOT_MODE    *CurrentBootMode,
  IN EFI_BOOT_MODE        NewBootMode
  )
{
  UINT32 CurrentIndex;
  UINT32 NewIndex;

  //
  // Find the position of the current boot mode in our priority array
  //
  for ( CurrentIndex = 0;
        CurrentIndex < sizeof (mBootModePriority) / sizeof (mBootModePriority[0]);
        CurrentIndex++) {
    if (mBootModePriority[CurrentIndex] == *CurrentBootMode) {
      break;
    }
  }
  if (CurrentIndex >= sizeof (mBootModePriority) / sizeof (mBootModePriority[0])) {
    return EFI_NOT_FOUND;
  }

  //
  // Find the position of the new boot mode in our priority array
  //
  for ( NewIndex = 0;
        NewIndex < sizeof (mBootModePriority) / sizeof (mBootModePriority[0]);
        NewIndex++) {
    if (mBootModePriority[NewIndex] == NewBootMode) {
      //
      // If this new boot mode occurs before the current boot mode in the
      // priority array, then take it.
      //
      if (NewIndex < CurrentIndex) {
        *CurrentBootMode = NewBootMode;
      }
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}
