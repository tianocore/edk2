/** @file
  SMM Firmware Volume Block Driver for Lakeport Platform.

  Firmware volume block driver for FWH or SPI device.
  It depends on which Flash Device Library to be linked with this driver.

Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

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

  @retval     VOID

**/
VOID
InstallFvbProtocol (
  IN  EFI_FW_VOL_INSTANCE               *FwhInstance,
  IN  UINTN                             InstanceNum
  )
{
  EFI_FW_VOL_BLOCK_DEVICE               *FvbDevice;
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_STATUS                            Status;
  EFI_HANDLE                            FvbHandle;

  FvbDevice = (EFI_FW_VOL_BLOCK_DEVICE *) AllocateRuntimeCopyPool (
                                            sizeof (EFI_FW_VOL_BLOCK_DEVICE),
                                            &mFvbDeviceTemplate
                                            );
  ASSERT (FvbDevice != NULL);

  FvbDevice->Instance = InstanceNum;
  FwVolHeader         = &FwhInstance->VolumeHeader;

  //
  // Set up the devicepath.
  //
  if (FwVolHeader->ExtHeaderOffset == 0) {
    //
    // FV does not contains extension header, then produce MEMMAP_DEVICE_PATH.
    //
    FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_MEMMAP_DEVICE_PATH), &mFvMemmapDevicePathTemplate);
    ((FV_MEMMAP_DEVICE_PATH *) FvbDevice->DevicePath)->MemMapDevPath.StartingAddress = FwhInstance->FvBase;
    ((FV_MEMMAP_DEVICE_PATH *) FvbDevice->DevicePath)->MemMapDevPath.EndingAddress   = FwhInstance->FvBase + FwVolHeader->FvLength - 1;
  } else {
    FvbDevice->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) AllocateRuntimeCopyPool (sizeof (FV_PIWG_DEVICE_PATH), &mFvPIWGDevicePathTemplate);
    CopyGuid (
      &((FV_PIWG_DEVICE_PATH *)FvbDevice->DevicePath)->FvDevPath.FvName,
      (GUID *)(UINTN)(FwhInstance->FvBase + FwVolHeader->ExtHeaderOffset)
      );
  }

  //
  // Install the SMM Firmware Volume Block Protocol and Device Path Protocol.
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
  // Notify the Fvb wrapper driver SMM fvb is ready.
  //
  FvbHandle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &FvbHandle,
                  &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &FvbDevice->FwVolBlockInstance
                  );
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

