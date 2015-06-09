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

#include "PlatformEarlyInit.h"


#define NORMALMODE        0
#define RECOVERYMODE      1
#define SAFEMODE          2
#define MANUFACTURINGMODE 3

#define GPIO_SSUS_OFFSET    0x2000
#define PMU_PWRBTN_B_OFFSET 0x88

EFI_PEI_PPI_DESCRIPTOR  mPpiListRecoveryBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
};

/**
  Return the setting of the Bios configuration jumper

  @param  VOID

  @retval RECOVERYMODE       jumper set to recovery mode
  @retval SAFEMODE           jumper set to config mode
  @retval NORMALMODE         jumper in normal mode

**/
UINTN
GetConfigJumper(
    IN CONST EFI_PEI_SERVICES           **PeiServices,
    IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
 )
{
  //
  // Do the Forced recovery detection based on logic chart above
  //
  if (IsRecoveryJumper(PeiServices, PlatformInfoHob)) {
    return RECOVERYMODE;
  } else {
    return NORMALMODE;
  }
}

BOOLEAN
CheckIfRecoveryMode(
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
 )
{
  if (GetConfigJumper(PeiServices, PlatformInfoHob) == RECOVERYMODE) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN
CheckIfSafeMode(
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
 )
{
  if (GetConfigJumper(PeiServices, PlatformInfoHob) == SAFEMODE) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN
CheckIfManufacturingMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices
 )
{
  EFI_STATUS                  Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *Variable;
  UINT32                      Attributes;
  UINTN                       DataSize;
  CHAR16                      VarName[] = MFGMODE_VARIABLE_NAME;
  UINT8                       MfgMode;

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                             (void **)&Variable
                             );
  ASSERT_EFI_ERROR (Status);

  //
  // Check if SW MMJ mode
  //
  Attributes = (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS);
  DataSize = sizeof (MFG_MODE_VAR);

  Status = Variable->GetVariable (
                       Variable,
                       VarName,
                       &gMfgModeVariableGuid,
                       &Attributes,
                       &DataSize,
                       &MfgMode
                       );
  if (!(EFI_ERROR (Status))) {
    return TRUE;
  }
  return FALSE;
}

EFI_STATUS
UpdateBootMode (
  IN CONST EFI_PEI_SERVICES                       **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB                    *PlatformInfoHob
  )
{
  EFI_STATUS                        Status;
  EFI_BOOT_MODE                     BootMode;
  UINT16                            SleepType;
  CHAR16                            *strBootMode;
  PEI_CAPSULE_PPI                   *Capsule;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI   *Variable;
  SYSTEM_CONFIGURATION              SystemConfiguration;
  UINTN                             VarSize;
  volatile UINT32                   GpioValue;
  BOOLEAN                           IsFirstBoot;
  UINT32                            Data32;

  Status = (*PeiServices)->GetBootMode(
                             PeiServices,
                             &BootMode
                             );
  ASSERT_EFI_ERROR (Status);
  if (BootMode  == BOOT_IN_RECOVERY_MODE){
    return Status;
  }
  GetWakeupEventAndSaveToHob (PeiServices);

  //
  // Let's assume things are OK if not told otherwise
  //
  BootMode = BOOT_WITH_FULL_CONFIGURATION;

  //
  // When this boot is WDT reset, the system needs booting with CrashDump function eanbled.
  //
  Data32 = IoRead32 (ACPI_BASE_ADDRESS + R_PCH_TCO_STS);

  //
  // Check Power Button, click the power button, the system will boot in fast boot mode,
  // if it is pressed and hold for a second, it will boot in FullConfiguration/setup mode.
  //
  GpioValue = MmioRead32 (IO_BASE_ADDRESS + GPIO_SSUS_OFFSET + PMU_PWRBTN_B_OFFSET);    // The value of GPIOS_16 (PMU_PWRBTN_B)
  if (((GpioValue & BIT0) != 0)&&((Data32 & B_PCH_TCO_STS_SECOND_TO) != B_PCH_TCO_STS_SECOND_TO)){
    IsFirstBoot = PcdGetBool(PcdBootState);
    if (!IsFirstBoot){
      VarSize = sizeof (SYSTEM_CONFIGURATION);
      ZeroMem (&SystemConfiguration, sizeof (SYSTEM_CONFIGURATION));

      Status = (*PeiServices)->LocatePpi (
                                 PeiServices,
                                 &gEfiPeiReadOnlyVariable2PpiGuid,
                                 0,
                                 NULL,
                                          (void **)&Variable
                                 );
      ASSERT_EFI_ERROR (Status);

      //
      // Use normal setup default from NVRAM variable,
      // the Platform Mode (manufacturing/safe/normal) is handle in PeiGetVariable.
      //
      VarSize = sizeof(SYSTEM_CONFIGURATION);
      Status = Variable->GetVariable (
                           Variable,
                           L"Setup",
                           &gEfiSetupVariableGuid,
                           NULL,
                           &VarSize,
                           &SystemConfiguration
                           );
      if (EFI_ERROR (Status) || VarSize != sizeof(SYSTEM_CONFIGURATION)) {
        //The setup variable is corrupted
        VarSize = sizeof(SYSTEM_CONFIGURATION);
        Status = Variable->GetVariable(
                  Variable,
                  L"SetupRecovery",
                  &gEfiSetupVariableGuid,
                  NULL,
                  &VarSize,
                  &SystemConfiguration
                  );
        ASSERT_EFI_ERROR (Status);
      }      

      if (SystemConfiguration.FastBoot == 1) {
            BootMode = BOOT_WITH_MINIMAL_CONFIGURATION;
      }
    }
  }

  //
  // Check if we need to boot in forced recovery mode
  //
  if (CheckIfRecoveryMode(PeiServices, PlatformInfoHob)) {
    BootMode  = BOOT_IN_RECOVERY_MODE;
  }

  if (BootMode  == BOOT_IN_RECOVERY_MODE) {
    Status = (*PeiServices)->InstallPpi (
                               PeiServices,
                               &mPpiListRecoveryBootMode
                               );
    ASSERT_EFI_ERROR (Status);
  } else {
    if (GetSleepTypeAfterWakeup (PeiServices, &SleepType)) {
      switch (SleepType) {
        case V_PCH_ACPI_PM1_CNT_S3:
          BootMode = BOOT_ON_S3_RESUME;

          //
          // Determine if we're in capsule update mode
          //
          Status = (*PeiServices)->LocatePpi (
                                     PeiServices,
                                     &gPeiCapsulePpiGuid,
                                     0,
                                     NULL,
                                     (void **)&Capsule
                                     );

          if (Status == EFI_SUCCESS) {
            if (Capsule->CheckCapsuleUpdate ((EFI_PEI_SERVICES**)PeiServices) == EFI_SUCCESS) {
              BootMode = BOOT_ON_FLASH_UPDATE;
            }
          }

          break;

        case V_PCH_ACPI_PM1_CNT_S4:
          BootMode = BOOT_ON_S4_RESUME;
          break;

        case V_PCH_ACPI_PM1_CNT_S5:
          BootMode = BOOT_ON_S5_RESUME;
          break;
      } // switch (SleepType)
    }

    //
    // Check for Safe Mode
    //
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
  Status = (*PeiServices)->SetBootMode(
                             PeiServices,
                             BootMode
                             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Get sleep type after wakeup

  @param PeiServices        Pointer to the PEI Service Table.
  @param SleepType          Sleep type to be returned.

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
    IoWrite16 ((ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_STS), B_PCH_ACPI_PM1_STS_WAK);
   }
  //
  // Get sleep type if a wake event occurred and there is no power failure
  //
  if ((Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP) == V_PCH_ACPI_PM1_CNT_S3) {
    *SleepType = Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP;
    return TRUE;
  } else if ((Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP) == V_PCH_ACPI_PM1_CNT_S4){
    *SleepType = Pm1Cnt & B_PCH_ACPI_PM1_CNT_SLP_TYP;
    return TRUE;
  }
  return FALSE;
}

VOID
SetPlatformBootMode (
  IN CONST EFI_PEI_SERVICES             **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
  )
{
  EFI_PLATFORM_SETUP_ID       PlatformSetupId;

  ZeroMem(&PlatformSetupId, sizeof (EFI_PLATFORM_SETUP_ID));

  CopyMem (&PlatformSetupId.SetupGuid,
           &gEfiNormalSetupGuid,
           sizeof (EFI_GUID));

  if (CheckIfRecoveryMode(PeiServices, PlatformInfoHob)) {
    //
    // Recovery mode
    //
    CopyMem (&PlatformSetupId.SetupName,
             &NORMAL_SETUP_NAME,
             StrSize (NORMAL_SETUP_NAME));    
    PlatformSetupId.PlatformBootMode = PLATFORM_RECOVERY_MODE;
  } else if (CheckIfSafeMode(PeiServices, PlatformInfoHob)) {
    //
    // Safe mode also called config mode or maintenace mode.
    //
    CopyMem (&PlatformSetupId.SetupName,
             &NORMAL_SETUP_NAME,
             StrSize (NORMAL_SETUP_NAME));
    PlatformSetupId.PlatformBootMode = PLATFORM_SAFE_MODE;

  } else if(0) { // else if (CheckIfManufacturingMode(PeiServices)) {
    //
    // Manufacturing mode
    //
    CopyMem (&PlatformSetupId.SetupName,
             MANUFACTURE_SETUP_NAME,
             StrSize (MANUFACTURE_SETUP_NAME));
    PlatformSetupId.PlatformBootMode = PLATFORM_MANUFACTURING_MODE;

  } else {
    //
    // Default to normal mode.
    //
    CopyMem (&PlatformSetupId.SetupName,
             &NORMAL_SETUP_NAME,
             StrSize (NORMAL_SETUP_NAME));
    PlatformSetupId.PlatformBootMode = PLATFORM_NORMAL_MODE;
  }

  BuildGuidDataHob (
    &gEfiPlatformBootModeGuid,
    &PlatformSetupId,
    sizeof (EFI_PLATFORM_SETUP_ID)
    );
  return;
}
