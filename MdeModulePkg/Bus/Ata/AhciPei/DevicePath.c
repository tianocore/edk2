/** @file
  The device path help function.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "AhciPei.h"

//
// Template for a SATA Device Path node
//
SATA_DEVICE_PATH  mAhciSataDevicePathNodeTemplate = {
  {        // Header
    MESSAGING_DEVICE_PATH,
    MSG_SATA_DP,
    {
      (UINT8)(sizeof (SATA_DEVICE_PATH)),
      (UINT8)((sizeof (SATA_DEVICE_PATH)) >> 8)
    }
  },
  0x0,     // HBAPortNumber
  0xFFFF,  // PortMultiplierPortNumber
  0x0      // Lun
};

//
// Template for an End of entire Device Path node
//
EFI_DEVICE_PATH_PROTOCOL  mAhciEndDevicePathNodeTemplate = {
  END_DEVICE_PATH_TYPE,
  END_ENTIRE_DEVICE_PATH_SUBTYPE,
  {
    (UINT8)(sizeof (EFI_DEVICE_PATH_PROTOCOL)),
    (UINT8)((sizeof (EFI_DEVICE_PATH_PROTOCOL)) >> 8)
  }
};

/**
  Get the size of the current device path instance.

  @param[in]  DevicePath             A pointer to the EFI_DEVICE_PATH_PROTOCOL
                                     structure.
  @param[out] InstanceSize           The size of the current device path instance.
  @param[out] EntireDevicePathEnd    Indicate whether the instance is the last
                                     one in the device path strucure.

  @retval EFI_SUCCESS    The size of the current device path instance is fetched.
  @retval Others         Fails to get the size of the current device path instance.

**/
EFI_STATUS
GetDevicePathInstanceSize (
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  OUT UINTN                     *InstanceSize,
  OUT BOOLEAN                   *EntireDevicePathEnd
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Walker;

  if ((DevicePath == NULL) || (InstanceSize == NULL) || (EntireDevicePathEnd == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the end of the device path instance
  //
  Walker = DevicePath;
  while (Walker->Type != END_DEVICE_PATH_TYPE) {
    Walker = NextDevicePathNode (Walker);
  }

  //
  // Check if 'Walker' points to the end of an entire device path
  //
  if (Walker->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
    *EntireDevicePathEnd = TRUE;
  } else if (Walker->SubType == END_INSTANCE_DEVICE_PATH_SUBTYPE) {
    *EntireDevicePathEnd = FALSE;
  } else {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Compute the size of the device path instance
  //
  *InstanceSize = ((UINTN)Walker - (UINTN)(DevicePath)) + sizeof (EFI_DEVICE_PATH_PROTOCOL);

  return EFI_SUCCESS;
}

/**
  Check the validity of the device path of a ATA AHCI host controller.

  @param[in] DevicePath          A pointer to the EFI_DEVICE_PATH_PROTOCOL
                                 structure.
  @param[in] DevicePathLength    The length of the device path.

  @retval EFI_SUCCESS              The device path is valid.
  @retval EFI_INVALID_PARAMETER    The device path is invalid.

**/
EFI_STATUS
AhciIsHcDevicePathValid (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINTN                     DevicePathLength
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *Start;
  UINTN                     Size;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Validate the DevicePathLength is big enough to touch the first node.
  //
  if (DevicePathLength < sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
    return EFI_INVALID_PARAMETER;
  }

  Start = DevicePath;
  while (!(DevicePath->Type == END_DEVICE_PATH_TYPE &&
           DevicePath->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE))
  {
    DevicePath = NextDevicePathNode (DevicePath);

    //
    // Prevent overflow and invalid zero in the 'Length' field of a device path
    // node.
    //
    if ((UINTN)DevicePath <= (UINTN)Start) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Prevent touching memory beyond given DevicePathLength.
    //
    if ((UINTN)DevicePath - (UINTN)Start >
        DevicePathLength - sizeof (EFI_DEVICE_PATH_PROTOCOL))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check if the device path and its size match each other.
  //
  Size = ((UINTN)DevicePath - (UINTN)Start) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
  if (Size != DevicePathLength) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Build the device path for an ATA device with given port and port multiplier number.

  @param[in]  Private               A pointer to the PEI_AHCI_CONTROLLER_PRIVATE_DATA
                                    data structure.
  @param[in]  Port                  The given port number.
  @param[in]  PortMultiplierPort    The given port multiplier number.
  @param[out] DevicePathLength      The length of the device path in bytes specified
                                    by DevicePath.
  @param[out] DevicePath            The device path of ATA device.

  @retval EFI_SUCCESS               The operation succeeds.
  @retval EFI_INVALID_PARAMETER     The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES      The operation fails due to lack of resources.

**/
EFI_STATUS
AhciBuildDevicePath (
  IN  PEI_AHCI_CONTROLLER_PRIVATE_DATA  *Private,
  IN  UINT16                            Port,
  IN  UINT16                            PortMultiplierPort,
  OUT UINTN                             *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL          **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathWalker;
  SATA_DEVICE_PATH          *SataDeviceNode;

  if ((DevicePathLength == NULL) || (DevicePath == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *DevicePathLength = Private->DevicePathLength + sizeof (SATA_DEVICE_PATH);
  *DevicePath       = AllocatePool (*DevicePathLength);
  if (*DevicePath == NULL) {
    *DevicePathLength = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Construct the host controller part device nodes
  //
  DevicePathWalker = *DevicePath;
  CopyMem (
    DevicePathWalker,
    Private->DevicePath,
    Private->DevicePathLength - sizeof (EFI_DEVICE_PATH_PROTOCOL)
    );

  //
  // Construct the SATA device node
  //
  DevicePathWalker = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)DevicePathWalker +
                                                  (Private->DevicePathLength - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
  CopyMem (
    DevicePathWalker,
    &mAhciSataDevicePathNodeTemplate,
    sizeof (mAhciSataDevicePathNodeTemplate)
    );
  SataDeviceNode                           = (SATA_DEVICE_PATH *)DevicePathWalker;
  SataDeviceNode->HBAPortNumber            = Port;
  SataDeviceNode->PortMultiplierPortNumber = PortMultiplierPort;

  //
  // Construct the end device node
  //
  DevicePathWalker = (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)DevicePathWalker +
                                                  sizeof (SATA_DEVICE_PATH));
  CopyMem (
    DevicePathWalker,
    &mAhciEndDevicePathNodeTemplate,
    sizeof (mAhciEndDevicePathNodeTemplate)
    );

  return EFI_SUCCESS;
}
