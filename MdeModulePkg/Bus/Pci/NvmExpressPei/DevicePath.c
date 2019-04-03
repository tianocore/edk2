/** @file
  The device path help function.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NvmExpressPei.h"

//
// Template for an Nvm Express Device Path node
//
NVME_NAMESPACE_DEVICE_PATH  mNvmeDevicePathNodeTemplate = {
  {        // Header
    MESSAGING_DEVICE_PATH,
    MSG_NVME_NAMESPACE_DP,
    {
      (UINT8) (sizeof (NVME_NAMESPACE_DEVICE_PATH)),
      (UINT8) ((sizeof (NVME_NAMESPACE_DEVICE_PATH)) >> 8)
    }
  },
  0x0,     // NamespaceId
  0x0      // NamespaceUuid
};

//
// Template for an End of entire Device Path node
//
EFI_DEVICE_PATH_PROTOCOL  mNvmeEndDevicePathNodeTemplate = {
  END_DEVICE_PATH_TYPE,
  END_ENTIRE_DEVICE_PATH_SUBTYPE,
  {
    (UINT8) (sizeof (EFI_DEVICE_PATH_PROTOCOL)),
    (UINT8) ((sizeof (EFI_DEVICE_PATH_PROTOCOL)) >> 8)
  }
};

/**
  Returns the 16-bit Length field of a device path node.

  Returns the 16-bit Length field of the device path node specified by Node.
  Node is not required to be aligned on a 16-bit boundary, so it is recommended
  that a function such as ReadUnaligned16() be used to extract the contents of
  the Length field.

  If Node is NULL, then ASSERT().

  @param  Node      A pointer to a device path node data structure.

  @return The 16-bit Length field of the device path node specified by Node.

**/
UINTN
DevicePathNodeLength (
  IN CONST VOID  *Node
  )
{
  ASSERT (Node != NULL);
  return ReadUnaligned16 ((UINT16 *)&((EFI_DEVICE_PATH_PROTOCOL *)(Node))->Length[0]);
}

/**
  Returns a pointer to the next node in a device path.

  If Node is NULL, then ASSERT().

  @param  Node    A pointer to a device path node data structure.

  @return a pointer to the device path node that follows the device path node
  specified by Node.

**/
EFI_DEVICE_PATH_PROTOCOL *
NextDevicePathNode (
  IN CONST VOID  *Node
  )
{
  ASSERT (Node != NULL);
  return (EFI_DEVICE_PATH_PROTOCOL *)((UINT8 *)(Node) + DevicePathNodeLength(Node));
}

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
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  OUT UINTN                       *InstanceSize,
  OUT BOOLEAN                     *EntireDevicePathEnd
  )
{
  EFI_DEVICE_PATH_PROTOCOL    *Walker;

  if (DevicePath == NULL || InstanceSize == NULL || EntireDevicePathEnd == NULL) {
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
  *InstanceSize = ((UINTN) Walker - (UINTN) (DevicePath)) + sizeof (EFI_DEVICE_PATH_PROTOCOL);

  return EFI_SUCCESS;
}

/**
  Check the validity of the device path of a NVM Express host controller.

  @param[in] DevicePath          A pointer to the EFI_DEVICE_PATH_PROTOCOL
                                 structure.
  @param[in] DevicePathLength    The length of the device path.

  @retval EFI_SUCCESS              The device path is valid.
  @retval EFI_INVALID_PARAMETER    The device path is invalid.

**/
EFI_STATUS
NvmeIsHcDevicePathValid (
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINTN                       DevicePathLength
  )
{
  EFI_DEVICE_PATH_PROTOCOL    *Start;
  UINTN                       Size;

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
           DevicePath->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)) {
    DevicePath = NextDevicePathNode (DevicePath);

    //
    // Prevent overflow and invalid zero in the 'Length' field of a device path
    // node.
    //
    if ((UINTN) DevicePath <= (UINTN) Start) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Prevent touching memory beyond given DevicePathLength.
    //
    if ((UINTN) DevicePath - (UINTN) Start >
        DevicePathLength - sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Check if the device path and its size match exactly with each other.
  //
  Size = ((UINTN) DevicePath - (UINTN) Start) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
  if (Size != DevicePathLength) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Build the device path for an Nvm Express device with given namespace identifier
  and namespace extended unique identifier.

  @param[in]  Private              A pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA
                                   data structure.
  @param[in]  NamespaceId          The given namespace identifier.
  @param[in]  NamespaceUuid        The given namespace extended unique identifier.
  @param[out] DevicePathLength     The length of the device path in bytes specified
                                   by DevicePath.
  @param[out] DevicePath           The device path of Nvm Express device.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     The operation fails due to lack of resources.

**/
EFI_STATUS
NvmeBuildDevicePath (
  IN  PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN  UINT32                              NamespaceId,
  IN  UINT64                              NamespaceUuid,
  OUT UINTN                               *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL            **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *DevicePathWalker;
  NVME_NAMESPACE_DEVICE_PATH    *NvmeDeviceNode;

  if (DevicePathLength == NULL || DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *DevicePathLength = Private->DevicePathLength + sizeof (NVME_NAMESPACE_DEVICE_PATH);
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
  // Construct the Nvm Express device node
  //
  DevicePathWalker = (EFI_DEVICE_PATH_PROTOCOL *) ((UINT8 *)DevicePathWalker +
                     (Private->DevicePathLength - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
  CopyMem (
    DevicePathWalker,
    &mNvmeDevicePathNodeTemplate,
    sizeof (mNvmeDevicePathNodeTemplate)
    );
  NvmeDeviceNode                = (NVME_NAMESPACE_DEVICE_PATH *)DevicePathWalker;
  NvmeDeviceNode->NamespaceId   = NamespaceId;
  NvmeDeviceNode->NamespaceUuid = NamespaceUuid;

  //
  // Construct the end device node
  //
  DevicePathWalker = (EFI_DEVICE_PATH_PROTOCOL *) ((UINT8 *)DevicePathWalker +
                     sizeof (NVME_NAMESPACE_DEVICE_PATH));
  CopyMem (
    DevicePathWalker,
    &mNvmeEndDevicePathNodeTemplate,
    sizeof (mNvmeEndDevicePathNodeTemplate)
    );

  return EFI_SUCCESS;
}
