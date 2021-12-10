/** @file
  The AhciPei driver is used to manage ATA hard disk device working under AHCI
  mode at PEI phase.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AhciPei.h"

#include <Guid/S3StorageDeviceInitList.h>

#include <Library/LockBoxLib.h>

/**
  Collect the ports that need to be enumerated on a controller for S3 phase.

  @param[in]  HcDevicePath          Device path of the controller.
  @param[in]  HcDevicePathLength    Length of the device path specified by
                                    HcDevicePath.
  @param[out] PortBitMap            Bitmap that indicates the ports that need
                                    to be enumerated on the controller.

  @retval    The number of ports that need to be enumerated.

**/
UINT8
AhciS3GetEumeratePorts (
  IN  EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath,
  IN  UINTN                     HcDevicePathLength,
  OUT UINT32                    *PortBitMap
  )
{
  EFI_STATUS                Status;
  UINT8                     DummyData;
  UINTN                     S3InitDevicesLength;
  EFI_DEVICE_PATH_PROTOCOL  *S3InitDevices;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  UINTN                     DevicePathInstLength;
  BOOLEAN                   EntireEnd;
  SATA_DEVICE_PATH          *SataDeviceNode;

  *PortBitMap = 0;

  //
  // From the LockBox, get the list of device paths for devices need to be
  // initialized in S3.
  //
  S3InitDevices       = NULL;
  S3InitDevicesLength = sizeof (DummyData);
  EntireEnd           = FALSE;
  Status              = RestoreLockBox (&gS3StorageDeviceInitListGuid, &DummyData, &S3InitDevicesLength);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return 0;
  } else {
    S3InitDevices = AllocatePool (S3InitDevicesLength);
    if (S3InitDevices == NULL) {
      return 0;
    }

    Status = RestoreLockBox (&gS3StorageDeviceInitListGuid, S3InitDevices, &S3InitDevicesLength);
    if (EFI_ERROR (Status)) {
      return 0;
    }
  }

  if (S3InitDevices == NULL) {
    return 0;
  }

  //
  // Only enumerate the ports that exist in the device list.
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
    S3InitDevices  = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)S3InitDevices + DevicePathInstLength);

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
          ) == 0)
    {
      //
      // Get the port number.
      //
      while (DevicePathInst->Type != END_DEVICE_PATH_TYPE) {
        if ((DevicePathInst->Type == MESSAGING_DEVICE_PATH) &&
            (DevicePathInst->SubType == MSG_SATA_DP))
        {
          SataDeviceNode = (SATA_DEVICE_PATH *)DevicePathInst;
          //
          // For now, the driver only support upto AHCI_MAX_PORTS ports and
          // devices directly connected to a HBA.
          //
          if ((SataDeviceNode->HBAPortNumber >= AHCI_MAX_PORTS) ||
              (SataDeviceNode->PortMultiplierPortNumber != 0xFFFF))
          {
            break;
          }

          *PortBitMap |= (UINT32)BIT0 << SataDeviceNode->HBAPortNumber;
          break;
        }

        DevicePathInst = NextDevicePathNode (DevicePathInst);
      }
    }
  } while (!EntireEnd);

  //
  // Return the number of ports need to be enumerated on this controller.
  //
  return AhciGetNumberOfPortsFromMap (*PortBitMap);
}
