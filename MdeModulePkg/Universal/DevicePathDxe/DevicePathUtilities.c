/** @file
  Implementation file for Device Path Utilities Protocol

Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DevicePath.h"

/**
  Returns the size of a device path in bytes.

  This function returns the size, in bytes, of the device path data structure specified by
  DevicePath including the end of device path node.  If DevicePath is NULL, then 0 is returned.

  @param  DevicePath                 A pointer to a device path data structure.

  @return The size of a device path in bytes.

**/
UINTN
EFIAPI
GetDevicePathSizeProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  return GetDevicePathSize (DevicePath);
}


/**
  Creates a new device path by appending a second device path to a first device path.

  This function allocates space for a new copy of the device path specified by DevicePath.  If
  DevicePath is NULL, then NULL is returned.  If the memory is successfully allocated, then the
  contents of DevicePath are copied to the newly allocated buffer, and a pointer to that buffer
  is returned.  Otherwise, NULL is returned.

  @param  DevicePath                 A pointer to a device path data structure.

  @return A pointer to the duplicated device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
DuplicateDevicePathProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  return DuplicateDevicePath (DevicePath);
}

/**
  Creates a new device path by appending a second device path to a first device path.

  This function creates a new device path by appending a copy of SecondDevicePath to a copy of
  FirstDevicePath in a newly allocated buffer.  Only the end-of-device-path device node from
  SecondDevicePath is retained. The newly created device path is returned.
  If FirstDevicePath is NULL, then it is ignored, and a duplicate of SecondDevicePath is returned.
  If SecondDevicePath is NULL, then it is ignored, and a duplicate of FirstDevicePath is returned.
  If both FirstDevicePath and SecondDevicePath are NULL, then a copy of an end-of-device-path is
  returned.
  If there is not enough memory for the newly allocated buffer, then NULL is returned.
  The memory for the new device path is allocated from EFI boot services memory. It is the
  responsibility of the caller to free the memory allocated.

  @param  FirstDevicePath            A pointer to a device path data structure.
  @param  SecondDevicePath           A pointer to a device path data structure.

  @return A pointer to the new device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *FirstDevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *SecondDevicePath
  )
{
  return AppendDevicePath (FirstDevicePath, SecondDevicePath);
}

/**
  Creates a new path by appending the device node to the device path.

  This function creates a new device path by appending a copy of the device node specified by
  DevicePathNode to a copy of the device path specified by DevicePath in an allocated buffer.
  The end-of-device-path device node is moved after the end of the appended device node.
  If DevicePathNode is NULL then a copy of DevicePath is returned.
  If DevicePath is NULL then a copy of DevicePathNode, followed by an end-of-device path device
  node is returned.
  If both DevicePathNode and DevicePath are NULL then a copy of an end-of-device-path device node
  is returned.
  If there is not enough memory to allocate space for the new device path, then NULL is returned.
  The memory is allocated from EFI boot services memory. It is the responsibility of the caller to
  free the memory allocated.

  @param  DevicePath                 A pointer to a device path data structure.
  @param  DevicePathNode             A pointer to a single device path node.

  @return A pointer to the new device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDeviceNodeProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePathNode
  )
{
  return AppendDevicePathNode (DevicePath, DevicePathNode);
}

/**
  Creates a new device path by appending the specified device path instance to the specified device
  path.

  This function creates a new device path by appending a copy of the device path instance specified
  by DevicePathInstance to a copy of the device path specified by DevicePath in a allocated buffer.
  The end-of-device-path device node is moved after the end of the appended device path instance
  and a new end-of-device-path-instance node is inserted between.
  If DevicePath is NULL, then a copy if DevicePathInstance is returned.
  If DevicePathInstance is NULL, then NULL is returned.
  If there is not enough memory to allocate space for the new device path, then NULL is returned.
  The memory is allocated from EFI boot services memory. It is the responsibility of the caller to
  free the memory allocated.

  @param  DevicePath                 A pointer to a device path data structure.
  @param  DevicePathInstance         A pointer to a device path instance.

  @return A pointer to the new device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathInstanceProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePathInstance
  )
{
  return AppendDevicePathInstance (DevicePath, DevicePathInstance);
}

/**
  Creates a copy of the current device path instance and returns a pointer to the next device path
  instance.

  This function creates a copy of the current device path instance. It also updates DevicePath to
  point to the next device path instance in the device path (or NULL if no more) and updates Size
  to hold the size of the device path instance copy.
  If DevicePath is NULL, then NULL is returned.
  If there is not enough memory to allocate space for the new device path, then NULL is returned.
  The memory is allocated from EFI boot services memory. It is the responsibility of the caller to
  free the memory allocated.
  If Size is NULL, then ASSERT().

  @param  DevicePath                 On input, this holds the pointer to the current device path
                                     instance. On output, this holds the pointer to the next device
                                     path instance or NULL if there are no more device path
                                     instances in the device path pointer to a device path data
                                     structure.
  @param  Size                       On output, this holds the size of the device path instance, in
                                     bytes or zero, if DevicePath is NULL.

  @return A pointer to the current device path instance.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
GetNextDevicePathInstanceProtocolInterface (
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath,
  OUT UINTN                         *Size
  )
{
  return GetNextDevicePathInstance (DevicePath, Size);
}

/**
  Determines if a device path is single or multi-instance.

  This function returns TRUE if the device path specified by DevicePath is multi-instance.
  Otherwise, FALSE is returned.  If DevicePath is NULL, then FALSE is returned.

  @param  DevicePath                 A pointer to a device path data structure.

  @retval  TRUE                      DevicePath is multi-instance.
  @retval  FALSE                     DevicePath is not multi-instance or DevicePath is NULL.

**/
BOOLEAN
EFIAPI
IsDevicePathMultiInstanceProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
{
  return IsDevicePathMultiInstance (DevicePath);
}

/**
  Creates a copy of the current device path instance and returns a pointer to the next device path
  instance.

  This function creates a new device node in a newly allocated buffer of size NodeLength and
  initializes the device path node header with NodeType and NodeSubType.  The new device path node
  is returned.
  If NodeLength is smaller than a device path header, then NULL is returned.
  If there is not enough memory to allocate space for the new device path, then NULL is returned.
  The memory is allocated from EFI boot services memory. It is the responsibility of the caller to
  free the memory allocated.

  @param  NodeType                   The device node type for the new device node.
  @param  NodeSubType                The device node sub-type for the new device node.
  @param  NodeLength                 The length of the new device node.

  @return The new device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
CreateDeviceNodeProtocolInterface (
  IN UINT8  NodeType,
  IN UINT8  NodeSubType,
  IN UINT16 NodeLength
  )
{
  return CreateDeviceNode (NodeType, NodeSubType, NodeLength);
}
