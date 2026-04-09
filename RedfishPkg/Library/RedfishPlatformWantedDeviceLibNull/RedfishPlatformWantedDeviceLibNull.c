/** @file
  NULL instace of RedfishPlatformWantedDeviceLib

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/RedfishPlatformWantedDeviceLib.h>

/**
  This is the function to decide if input controller is the device
  that platform want to support. By returning EFI_UNSUPPORTED to
  caller (normally Supported function), caller should ignore this device
  and do not provide Redfish service on this controller.

  @param[in]  ControllerHandle     The handle of the controller to test.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.
                                   This is optional.

  @retval EFI_SUCCESS              This is the device supported by platform.
  @retval EFI_UNSUPPORTED          This device is not supported by platform.
  @retval EFI_INVALID_PARAMETER    ControllerHandle is NULL.

**/
EFI_STATUS
EFIAPI
IsPlatformWantedDevice (
  IN EFI_HANDLE                ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath OPTIONAL
  )
{
  if (ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Always support Redfish on ControllerHandle.
  //
  return EFI_SUCCESS;
}
