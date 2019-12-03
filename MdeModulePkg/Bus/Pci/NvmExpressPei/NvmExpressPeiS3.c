/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpressPei.h"

#include <Guid/S3StorageDeviceInitList.h>

#include <Library/LockBoxLib.h>

/**
  Determine if a specific NVM Express controller can be skipped for S3 phase.

  @param[in]  HcDevicePath          Device path of the controller.
  @param[in]  HcDevicePathLength    Length of the device path specified by
                                    HcDevicePath.

  @retval    The number of ports that need to be enumerated.

**/
BOOLEAN
NvmeS3SkipThisController (
  IN  EFI_DEVICE_PATH_PROTOCOL    *HcDevicePath,
  IN  UINTN                       HcDevicePathLength
  )
{
  EFI_STATUS                  Status;
  UINT8                       DummyData;
  UINTN                       S3InitDevicesLength;
  EFI_DEVICE_PATH_PROTOCOL    *S3InitDevices;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePathInst;
  UINTN                       DevicePathInstLength;
  BOOLEAN                     EntireEnd;
  BOOLEAN                     Skip;

  //
  // From the LockBox, get the list of device paths for devices need to be
  // initialized in S3.
  //
  S3InitDevices       = NULL;
  S3InitDevicesLength = sizeof (DummyData);
  EntireEnd           = FALSE;
  Skip                = TRUE;
  Status = RestoreLockBox (&gS3StorageDeviceInitListGuid, &DummyData, &S3InitDevicesLength);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Skip;
  } else {
    S3InitDevices = AllocatePool (S3InitDevicesLength);
    if (S3InitDevices == NULL) {
      return Skip;
    }

    Status = RestoreLockBox (&gS3StorageDeviceInitListGuid, S3InitDevices, &S3InitDevicesLength);
    if (EFI_ERROR (Status)) {
      return Skip;
    }
  }

  if (S3InitDevices == NULL) {
    return Skip;
  }

  //
  // Only need to initialize the controllers that exist in the device list.
  //
  do {
    //
    // Fetch the size of current device path instance.
    //
    Status = GetDevicePathInstanceSize (
               S3InitDevices,
               &DevicePathInstLength,
               &EntireEnd
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    DevicePathInst = S3InitDevices;
    S3InitDevices  = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN) S3InitDevices + DevicePathInstLength);

    if (HcDevicePathLength >= DevicePathInstLength) {
      continue;
    }

    //
    // Compare the device paths to determine if the device is managed by this
    // controller.
    //
    if (CompareMem (
          DevicePathInst,
          HcDevicePath,
          HcDevicePathLength - sizeof (EFI_DEVICE_PATH_PROTOCOL)
          ) == 0) {
      Skip = FALSE;
      break;
    }
  } while (!EntireEnd);

  return Skip;
}
