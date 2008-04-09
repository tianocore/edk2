/** @file
  Implementation file for Device Path Utilities Protocol

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DevicePath.h"

UINTN
GetDevicePathSizeProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

  Routine Description:
    Returns the size of the device path, in bytes.

  Arguments:
    DevicePath  -   Points to the start of the EFI device path.

  Returns:
    Size        -   Size of the specified device path, in bytes, including the end-of-path tag.

--*/
{
  return GetDevicePathSize (DevicePath);
}

EFI_DEVICE_PATH_PROTOCOL *
DuplicateDevicePathProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

  Routine Description:
    Create a duplicate of the specified path.

  Arguments:
    DevicePath  -   Points to the source EFI device path.

  Returns:
    Pointer     -   A pointer to the duplicate device path.
    NULL        -   Insufficient memory.

--*/
{
  return DuplicateDevicePath (DevicePath);
}

EFI_DEVICE_PATH_PROTOCOL *
AppendDevicePathProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *Src1,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *Src2
  )
/*++

  Routine Description:
    Create a new path by appending the second device path to the first.

  Arguments:
    Src1      -   Points to the first device path. If NULL, then it is ignored.
    Src2      -   Points to the second device path. If NULL, then it is ignored.

  Returns:
    Pointer   -   A pointer to the newly created device path.
    NULL      -   Memory could not be allocated
                  or either DevicePath or DeviceNode is NULL.

--*/
{
  return AppendDevicePath (Src1, Src2);
}

EFI_DEVICE_PATH_PROTOCOL *
AppendDeviceNodeProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DeviceNode
  )
/*++

  Routine Description:
    Creates a new path by appending the device node to the device path.

  Arguments:
    DevicePath   -   Points to the device path.
    DeviceNode   -   Points to the device node.

  Returns:
    Pointer      -   A pointer to the allocated device node.
    NULL         -   Memory could not be allocated
                     or either DevicePath or DeviceNode is NULL.

--*/
{
  return AppendDevicePathNode (DevicePath, DeviceNode);
}

EFI_DEVICE_PATH_PROTOCOL *
AppendDevicePathInstanceProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePathInstance
  )
/*++

  Routine Description:
    Creates a new path by appending the specified device path instance to the specified device path.

  Arguments:
    DevicePath           -   Points to the device path. If NULL, then ignored.
    DevicePathInstance   -   Points to the device path instance.

  Returns:
    Pointer              -   A pointer to the newly created device path
    NULL                 -   Memory could not be allocated or DevicePathInstance is NULL.

--*/
{
  return AppendDevicePathInstance (DevicePath, DevicePathInstance);
}

EFI_DEVICE_PATH_PROTOCOL *
GetNextDevicePathInstanceProtocolInterface (
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePathInstance,
  OUT UINTN                         *DevicePathInstanceSize
  )
/*++

  Routine Description:
    Creates a copy of the current device path instance and returns a pointer to the next device path instance.

  Arguments:
    DevicePathInstance       -   On input, this holds the pointer to the current device path
                                 instance. On output, this holds the pointer to the next
                                 device path instance or NULL if there are no more device
                                 path instances in the device path.
    DevicePathInstanceSize   -   On output, this holds the size of the device path instance,
                                 in bytes or zero, if DevicePathInstance is zero.

  Returns:
    Pointer                  -   A pointer to the copy of the current device path instance.
    NULL                     -   DevicePathInstace was NULL on entry or there was insufficient memory.

--*/
{
  return GetNextDevicePathInstance (DevicePathInstance, DevicePathInstanceSize);
}

BOOLEAN
IsDevicePathMultiInstanceProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
/*++

  Routine Description:
    Returns whether a device path is multi-instance.

  Arguments:
    DevicePath  -   Points to the device path. If NULL, then ignored.

  Returns:
    TRUE        -   The device path has more than one instance
    FALSE       -   The device path is empty or contains only a single instance.

--*/
{
  return IsDevicePathMultiInstance (DevicePath);
}

EFI_DEVICE_PATH_PROTOCOL *
CreateDeviceNodeProtocolInterface (
  IN UINT8  NodeType,
  IN UINT8  NodeSubType,
  IN UINT16 NodeLength
  )
/*++

  Routine Description:
    Creates a device node

  Arguments:
    NodeType     -    NodeType is the device node type (EFI_DEVICE_PATH.Type) for
                      the new device node.
    NodeSubType  -    NodeSubType is the device node sub-type
                      EFI_DEVICE_PATH.SubType) for the new device node.
    NodeLength   -    NodeLength is the length of the device node
                      (EFI_DEVICE_PATH.Length) for the new device node.

  Returns:
    Pointer      -    A pointer to the newly created device node.
    NULL         -    NodeLength is less than
                      the size of the header or there was insufficient memory.

--*/
{
  return CreateDeviceNode (NodeType, NodeSubType, NodeLength);
}
