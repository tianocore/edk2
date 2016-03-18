/** @file
This file provide the function to detect boot mode

Copyright (c) 2013 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "CommonHeader.h"
#include <Pi/PiFirmwareVolume.h>

EFI_PEI_PPI_DESCRIPTOR mPpiListRecoveryBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiBootInRecoveryModePpiGuid,
  NULL
};

/**
  If the box was opened, it's boot with full config.
  If the box is closed, then
    1. If it's first time to boot, it's boot with full config .
    2. If the ChassisIntrution is selected, force to be a boot with full config
    3. Otherwise it's boot with no change.

  @param  PeiServices General purpose services available to every PEIM.

  @retval TRUE  If it's boot with no change.

  @retval FALSE If boot with no change.
**/
BOOLEAN
IsBootWithNoChange (
  IN EFI_PEI_SERVICES   **PeiServices
  )
{
  BOOLEAN IsFirstBoot = FALSE;

  BOOLEAN EnableFastBoot = FALSE;
  IsFirstBoot = PcdGetBool(PcdBootState);
  EnableFastBoot = PcdGetBool (PcdEnableFastBoot);

  DEBUG ((EFI_D_INFO, "IsFirstBoot = %x , EnableFastBoot= %x. \n", IsFirstBoot, EnableFastBoot));

  if ((!IsFirstBoot) && EnableFastBoot) {
    return TRUE;
  } else {
    return FALSE;
  }
}


/**

Routine Description:

  This function is used to verify if the FV header is validate.

  @param  FwVolHeader - The FV header that to be verified.

  @retval EFI_SUCCESS   - The Fv header is valid.
  @retval EFI_NOT_FOUND - The Fv header is invalid.

**/
EFI_STATUS
ValidateFvHeader (
  EFI_BOOT_MODE      *BootMode
  )
{
  UINT16  *Ptr;
  UINT16  HeaderLength;
  UINT16  Checksum;

  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;

  if (BOOT_IN_RECOVERY_MODE == *BootMode) {
    DEBUG ((EFI_D_INFO, "Boot mode recovery\n"));
    return EFI_SUCCESS;
  }
  //
  // Let's check whether FvMain header is valid, if not enter into recovery mode
  //
  //
  // Verify the header revision, header signature, length
  // Length of FvBlock cannot be 2**64-1
  // HeaderLength cannot be an odd number
  //
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32(PcdFlashFvMainBase);
  if ((FwVolHeader->Revision != EFI_FVH_REVISION)||
      (FwVolHeader->Signature != EFI_FVH_SIGNATURE) ||
      (FwVolHeader->FvLength == ((UINT64) -1)) ||
      ((FwVolHeader->HeaderLength & 0x01) != 0)
      ) {
    return EFI_NOT_FOUND;
  }
  //
  // Verify the header checksum
  //
  HeaderLength  = (UINT16) (FwVolHeader->HeaderLength / 2);
  Ptr           = (UINT16 *) FwVolHeader;
  Checksum      = 0;
  while (HeaderLength > 0) {
    Checksum = Checksum +*Ptr;
    Ptr++;
    HeaderLength--;
  }

  if (Checksum != 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**

  Check whether go to recovery path
  @retval TRUE  Go to recovery path
  @retval FALSE Go to normal path

**/
BOOLEAN
OemRecoveryBootMode ()
{
  return PlatformIsBootWithRecoveryStage1 ();
}

/**
  Peform the boot mode determination logic
  If the box is closed, then
    1. If it's first time to boot, it's boot with full config .
    2. If the ChassisIntrution is selected, force to be a boot with full config
    3. Otherwise it's boot with no change.

  @param  PeiServices General purpose services available to every PEIM.

  @param  BootMode The detected boot mode.

  @retval EFI_SUCCESS if the boot mode could be set
**/
EFI_STATUS
UpdateBootMode (
  IN  EFI_PEI_SERVICES     **PeiServices,
  OUT EFI_BOOT_MODE        *BootMode
  )
{
  EFI_STATUS          Status;
  EFI_BOOT_MODE       NewBootMode;
  PEI_CAPSULE_PPI     *Capsule;
  CHAR8               UserSelection;
  UINT32              Straps32;

  //
  // Read Straps. Used later if recovery boot.
  //
  Straps32 = QNCAltPortRead (QUARK_SCSS_SOC_UNIT_SB_PORT_ID, QUARK_SCSS_SOC_UNIT_STPDDRCFG);

  //
  // Check if we need to boot in recovery mode
  //
  if ((ValidateFvHeader (BootMode) != EFI_SUCCESS)) {
    DEBUG ((EFI_D_INFO, "Force Boot mode recovery\n"));
    NewBootMode = BOOT_IN_RECOVERY_MODE;
    Status = PeiServicesInstallPpi (&mPpiListRecoveryBootMode);
    ASSERT_EFI_ERROR (Status);
    if (OemRecoveryBootMode () == FALSE) {
      DEBUG ((EFI_D_INFO, "Recovery stage1 not Active, reboot to activate recovery stage1 image\n"));
      OemInitiateRecoveryAndWait ();
    }
  } else if (OemRecoveryBootMode ()) {
    DEBUG ((EFI_D_INFO, "Boot mode recovery\n"));
    NewBootMode = BOOT_IN_RECOVERY_MODE;
    Status = PeiServicesInstallPpi (&mPpiListRecoveryBootMode);
    ASSERT_EFI_ERROR (Status);
  } else if (QNCCheckS3AndClearState ()) {
    //
    // Determine if we're in capsule update mode
    //
    Status = PeiServicesLocatePpi (
               &gPeiCapsulePpiGuid,
               0,
               NULL,
               (VOID **)&Capsule
               );
    if (Status == EFI_SUCCESS) {
      Status = Capsule->CheckCapsuleUpdate (PeiServices);
      if (Status == EFI_SUCCESS) {
        DEBUG ((EFI_D_INFO, "Boot mode Flash Update\n"));
        NewBootMode = BOOT_ON_FLASH_UPDATE;
      } else {
        DEBUG ((EFI_D_INFO, "Boot mode S3 resume\n"));
        NewBootMode = BOOT_ON_S3_RESUME;
      }
    } else {
      DEBUG ((EFI_D_INFO, "Boot mode S3 resume\n"));
      NewBootMode = BOOT_ON_S3_RESUME;
    }
  } else {
    //
    // Check if this is a power on reset
    //
    if (QNCCheckPowerOnResetAndClearState ()) {
      DEBUG ((EFI_D_INFO, "Power On Reset\n"));
    }
    if (IsBootWithNoChange (PeiServices)) {
      DEBUG ((EFI_D_INFO, "Boot with Minimum cfg\n"));
      NewBootMode = BOOT_ASSUMING_NO_CONFIGURATION_CHANGES;
    } else {
      DEBUG ((EFI_D_INFO, "Boot with Full cfg\n"));
      NewBootMode = BOOT_WITH_FULL_CONFIGURATION;
    }
  }
  *BootMode = NewBootMode;
  Status = PeiServicesSetBootMode (NewBootMode);
  ASSERT_EFI_ERROR (Status);

  //
  // If Recovery Boot then prompt the user to insert a USB key with recovery nodule and
  // continue with the recovery. Also give the user a chance to retry a normal boot.
  //
  if (NewBootMode == BOOT_IN_RECOVERY_MODE) {
    if ((Straps32 & B_STPDDRCFG_FORCE_RECOVERY) == 0) {
      DEBUG ((EFI_D_ERROR, "*****************************************************************\n"));
      DEBUG ((EFI_D_ERROR, "*****           Force Recovery Jumper Detected.             *****\n"));
      DEBUG ((EFI_D_ERROR, "*****      Attempting auto recovery of system flash.        *****\n"));
      DEBUG ((EFI_D_ERROR, "*****   Expecting USB key with recovery module connected.   *****\n"));
      DEBUG ((EFI_D_ERROR, "*****         PLEASE REMOVE FORCE RECOVERY JUMPER.          *****\n"));
      DEBUG ((EFI_D_ERROR, "*****************************************************************\n"));
    } else {
      DEBUG ((EFI_D_ERROR, "*****************************************************************\n"));
      DEBUG ((EFI_D_ERROR, "*****           ERROR: System boot failure!!!!!!!           *****\n"));
      DEBUG ((EFI_D_ERROR, "***** - Press 'R' if you wish to force system recovery      *****\n"));
      DEBUG ((EFI_D_ERROR, "*****     (connect USB key with recovery module first)      *****\n"));
      DEBUG ((EFI_D_ERROR, "***** - Press any other key to attempt another boot         *****\n"));
      DEBUG ((EFI_D_ERROR, "*****************************************************************\n"));

      UserSelection = PlatformDebugPortGetChar8 ();
      if ((UserSelection != 'R') && (UserSelection != 'r')) {
        DEBUG ((EFI_D_ERROR, "New boot attempt selected........\n"));
        //
        // Initialte the cold reset
        //
        ResetCold ();
      }
      DEBUG ((EFI_D_ERROR, "Recovery boot selected..........\n"));
    }
  }

  return EFI_SUCCESS;
}
