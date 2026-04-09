/** @file
  Boot Discovery Policy UI for Boot Maintenance menu.

  Copyright (c) 2021, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2021, Semihalf All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/BootDiscoveryPolicy.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Include/Library/PcdLib.h>

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

extern UINT8  BootDiscoveryPolicyUiLibVfrBin[];

EFI_HII_HANDLE  mBPHiiHandle    = NULL;
EFI_HANDLE      mBPDriverHandle = NULL;

STATIC HII_VENDOR_DEVICE_PATH  mVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    BOOT_DISCOVERY_POLICY_MGR_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/**

  Initialize Boot Maintenance Menu library.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCCESS  Install Boot manager menu success.
  @retval  Other        Return error status.gBPDisplayLibGuid

**/
EFI_STATUS
EFIAPI
BootDiscoveryPolicyUiLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  UINT32      BootDiscoveryPolicy;

  Size   = sizeof (UINT32);
  Status = gRT->GetVariable (
                  BOOT_DISCOVERY_POLICY_VAR,
                  &gBootDiscoveryPolicyMgrFormsetGuid,
                  NULL,
                  &Size,
                  &BootDiscoveryPolicy
                  );
  if (EFI_ERROR (Status)) {
    Status = PcdSet32S (PcdBootDiscoveryPolicy, PcdGet32 (PcdBootDiscoveryPolicy));
    ASSERT_EFI_ERROR (Status);
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mBPDriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mVendorDevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Publish our HII data
  //
  mBPHiiHandle = HiiAddPackages (
                   &gBootDiscoveryPolicyMgrFormsetGuid,
                   mBPDriverHandle,
                   BootDiscoveryPolicyUiLibVfrBin,
                   BootDiscoveryPolicyUiLibStrings,
                   NULL
                   );
  if (mBPHiiHandle == NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           mBPDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mVendorDevicePath,
           NULL
           );

    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Destructor of Boot Maintenance menu library.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Other value   The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
BootDiscoveryPolicyUiLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mBPDriverHandle != NULL) {
    gBS->UninstallProtocolInterface (
           mBPDriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mVendorDevicePath
           );
    mBPDriverHandle = NULL;
  }

  if (mBPHiiHandle != NULL) {
    HiiRemovePackages (mBPHiiHandle);
    mBPHiiHandle = NULL;
  }

  return EFI_SUCCESS;
}
