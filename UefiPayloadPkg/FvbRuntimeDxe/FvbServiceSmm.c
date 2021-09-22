/** @file
  SMM Firmware Volume Block Driver.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiSmm.h>
#include <Library/SmmServicesTableLib.h>
#include "FvbSmmCommon.h"
#include "FvbService.h"

/**
  The function installs EFI_SMM_FIRMWARE_VOLUME_BLOCK protocol
  for each FV in the system.

  @param[in]  FwhInstance   The pointer to a FW volume instance structure,
                            which contains the information about one FV.
  @param[in]  InstanceNum   The instance number which can be used as a ID
                            to locate this FwhInstance in other functions.

  @retval     EFI_SUCESS    Installed successfully.
  @retval     Else          Did not install successfully.

**/
EFI_STATUS
InstallFvbProtocol (
  IN  EFI_FW_VOL_INSTANCE               *FwhInstance,
  IN  UINTN                             InstanceNum
  )
{
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_STATUS                            Status;
  EFI_HANDLE                            FvbHandle;
  FV_MEMMAP_DEVICE_PATH                 *FvDevicePath;
  VOID                                  *TempPtr;

  FvbDevice = (EFI_FW_VOL_BLOCK_DEVICE *) AllocateRuntimeCopyPool (
                                            sizeof (EFI_FW_VOL_BLOCK_DEVICE),
                                            &mFvbDeviceTemplate
                                            );
  if (FvbDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  FvbDevice->Instance = InstanceNum;
  FwVolHeader         = &FwhInstance->VolumeHeader;

  //
  // Set up the devicepath
  //
  if (FwVolHeader->ExtHeaderOffset == 0) {
    //
    // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH
    //
    TempPtr = AllocateRuntimeCopyPool (sizeof (FV_MEMMAP_DEVICE_PATH), &mFvMemmapDevicePathTemplate);
    FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
    if (FvbDevice->DevicePath == NULL) {
      ASSERT (FALSE);
      return EFI_OUT_OF_RESOURCES;
    }
    FvDevicePath = (FV_MEMMAP_DEVICE_PATH *) FvbDevice->DevicePath;
    FvDevicePath->MemMapDevPath.StartingAddress = FwhInstance->FvBase;
    FvDevicePath->MemMapDevPath.EndingAddress   = FwhInstance->FvBase + FwVolHeader->FvLength - 1;
  } else {
    TempPtr = AllocateRuntimeCopyPool (sizeof (FV_PIWG_DEVICE_PATH), &mFvPIWGDevicePathTemplate);
    FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) TempPtr;
    if (FvbDevice->DevicePath == NULL) {
      ASSERT (FALSE);
      return EFI_OUT_OF_RESOURCES;
    }

    CopyGuid (
      &((FV_PIWG_DEVICE_PATH *)FvbDevice->DevicePath)->FvDevPath.FvName,
      (GUID *)(UINTN)(FwhInstance->FvBase + FwVolHeader->ExtHeaderOffset)
      );
  }

  //
  // Install the SMM Firmware Volume Block Protocol and Device Path Protocol
  //
  FvbHandle = NULL;
  Status = gSmst->SmmInstallProtocolInterface (
                    &FvbHandle,
                    &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &FvbDevice->FwVolBlockInstance
                    );
  ASSERT_EFI_ERROR (Status);

  Status = gSmst->SmmInstallProtocolInterface (
                    &FvbHandle,
                    &gEfiDevicePathProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    FvbDevice->DevicePath
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Notify the Fvb wrapper driver SMM fvb is ready
  //
  FvbHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &FvbHandle,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FvbDevice->FwVolBlockInstance
                  );

  return Status;
}


/**
  The driver entry point for SMM Firmware Volume Block Driver.

  The function does the necessary initialization work
  Firmware Volume Block Driver.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI system table.

  @retval     EFI_SUCCESS       This funtion always return EFI_SUCCESS.
                                It will ASSERT on errors.

**/
EFI_STATUS
EFIAPI
FvbSmmInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  FvbInitialize ();

  return EFI_SUCCESS;
}
