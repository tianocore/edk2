/** @file
Platform Initialization Driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CommonHeader.h"

#include "SetupPlatform.h"
#include <Library/HobLib.h>

EFI_HANDLE            mImageHandle = NULL;

EFI_HII_DATABASE_PROTOCOL        *mHiiDataBase = NULL;
EFI_HII_CONFIG_ROUTING_PROTOCOL  *mHiiConfigRouting = NULL;

UINT8                    mSmbusRsvdAddresses[PLATFORM_NUM_SMBUS_RSVD_ADDRESSES] = {
  SMBUS_ADDR_CH_A_1,
  SMBUS_ADDR_CK505,
  SMBUS_ADDR_THERMAL_SENSOR1,
  SMBUS_ADDR_THERMAL_SENSOR2
};

EFI_PLATFORM_POLICY_PROTOCOL    mPlatformPolicyData = {
  PLATFORM_NUM_SMBUS_RSVD_ADDRESSES,
  mSmbusRsvdAddresses
};

EFI_STATUS
DxePlatformDriverEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

  Routine Description:
    This is the standard EFI driver point for the D845GRgPlatform Driver. This
    driver is responsible for setting up any platform specific policy or
    initialization information.

  Arguments:
    ImageHandle     - Handle for the image of this driver
    SystemTable     - Pointer to the EFI System Table

  Returns:
    EFI_SUCCESS     - Policy decisions set

--*/
{
  EFI_STATUS                  Status;
  EFI_HANDLE                  Handle;

  S3BootScriptSaveInformationAsciiString (
    "SetupDxeEntryBegin"
    );

  mImageHandle = ImageHandle;

  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID**)&mHiiDataBase);
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID**)&mHiiConfigRouting);
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize keyboard layout
  //
  Status = InitKeyboardLayout ();

  //
  // Initialize ICH registers
  //
  PlatformInitQNCRegs();

  ProducePlatformCpuData ();

  //
  // Install protocol to to allow access to this Policy.
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiPlatformPolicyProtocolGuid, &mPlatformPolicyData,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  S3BootScriptSaveInformationAsciiString (
    "SetupDxeEntryEnd"
    );

  return EFI_SUCCESS;
}

