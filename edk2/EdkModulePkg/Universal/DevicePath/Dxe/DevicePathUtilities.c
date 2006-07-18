/*++

Copyright (c) 2006, Intel Corporation                                                     
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DevicePathUtilities.c

Abstract:

  Implementation file for Device Path Utilities Protocol

--*/

#include <protocol/DevicePathUtilities.h>
#include <protocol/DevicePath.h>
#include "DevicePath.h"

UINTN
GetDevicePathSize (
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
  CONST EFI_DEVICE_PATH_PROTOCOL  *Start;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Start = (EFI_DEVICE_PATH_PROTOCOL *) DevicePath;
  while (!IsDevicePathEnd (DevicePath)) {
    DevicePath = NextDevicePathNode (DevicePath);
  }

  //
  // Compute the size and add back in the size of the end device path structure
  //
  return ((UINTN) DevicePath - (UINTN) Start) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
}

EFI_DEVICE_PATH_PROTOCOL *
DuplicateDevicePath (
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
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     Size;

  if (DevicePath == NULL) {
    return NULL;
  }

  //
  // Compute the size
  //
  Size = GetDevicePathSize (DevicePath);
  if (Size == 0) {
    return NULL;
  }

  //
  // Allocate space for duplicate device path
  //
  NewDevicePath = AllocateCopyPool (Size, (VOID *) DevicePath);

  return NewDevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
AppendDevicePath (
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
  UINTN                     Size;
  UINTN                     Size1;
  UINTN                     Size2;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *SecondDevicePath;

  //
  // If there's only 1 path, just duplicate it
  //
  if (Src1 == NULL) {
    ASSERT (!IsDevicePathUnpacked (Src2));
    return DuplicateDevicePath (Src2);
  }

  if (Src2 == NULL) {
    ASSERT (!IsDevicePathUnpacked (Src1));
    return DuplicateDevicePath (Src1);
  }

  //
  // Allocate space for the combined device path. It only has one end node of
  // length EFI_DEVICE_PATH_PROTOCOL
  //
  Size1         = GetDevicePathSize (Src1);
  Size2         = GetDevicePathSize (Src2);
  Size          = Size1 + Size2 - sizeof (EFI_DEVICE_PATH_PROTOCOL);

  NewDevicePath = AllocateCopyPool (Size, (VOID *) Src1);

  if (NewDevicePath != NULL) {
    //
    // Over write Src1 EndNode and do the copy
    //
    SecondDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) ((CHAR8 *) NewDevicePath + (Size1 - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
    CopyMem (SecondDevicePath, (VOID *) Src2, Size2);
  }

  return NewDevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
AppendDeviceNode (
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
  EFI_DEVICE_PATH_PROTOCOL  *Temp;
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     NodeLength;

  if ((DevicePath == NULL) || (DeviceNode == NULL)) {
    return NULL;
  }

  //
  // Build a Node that has a terminator on it
  //
  NodeLength  = DevicePathNodeLength (DeviceNode);

  Temp        = AllocateCopyPool (NodeLength + sizeof (EFI_DEVICE_PATH_PROTOCOL), (VOID *) DeviceNode);
  if (Temp == NULL) {
    return NULL;
  }

  //
  // Add and end device path node to convert Node to device path
  //
  NextNode = NextDevicePathNode (Temp);
  SetDevicePathEndNode (NextNode);

  //
  // Append device paths
  //
  NewDevicePath = AppendDevicePath (DevicePath, Temp);
  gBS->FreePool (Temp);
  return NewDevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
AppendDevicePathInstance (
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
  UINT8                     *Ptr;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  UINTN                     SrcSize;
  UINTN                     InstanceSize;

  if (DevicePathInstance == NULL) {
    return NULL;
  }

  if (DevicePath == NULL) {
    return DuplicateDevicePath (DevicePathInstance);
  }

  SrcSize       = GetDevicePathSize (DevicePath);
  InstanceSize  = GetDevicePathSize (DevicePathInstance);

  Ptr           = AllocateCopyPool (SrcSize + InstanceSize, (VOID *) DevicePath);
  if (Ptr != NULL) {

    DevPath = (EFI_DEVICE_PATH_PROTOCOL *) (Ptr + (SrcSize - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
    //
    // Convert the End to an End Instance, since we are
    //  appending another instacne after this one its a good
    //  idea.
    //
    DevPath->SubType  = END_INSTANCE_DEVICE_PATH_SUBTYPE;

    DevPath           = NextDevicePathNode (DevPath);
    CopyMem (DevPath, (VOID *) DevicePathInstance, InstanceSize);
  }

  return (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
}

EFI_DEVICE_PATH_PROTOCOL *
GetNextDevicePathInstance (
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
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *ReturnValue;
  UINT8                     Temp;

  if (*DevicePathInstance == NULL) {
    if (DevicePathInstanceSize != NULL) {
      *DevicePathInstanceSize = 0;
    }

    return NULL;
  }

  //
  // Find the end of the device path instance
  //
  DevPath = *DevicePathInstance;
  while (!IsDevicePathEndType (DevPath)) {
    DevPath = NextDevicePathNode (DevPath);
  }

  //
  // Compute the size of the device path instance
  //
  if (DevicePathInstanceSize != NULL) {
    *DevicePathInstanceSize = ((UINTN) DevPath - (UINTN) (*DevicePathInstance)) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
  }

  //
  // Make a copy and return the device path instance
  //
  Temp              = DevPath->SubType;
  DevPath->SubType  = END_ENTIRE_DEVICE_PATH_SUBTYPE;
  ReturnValue       = DuplicateDevicePath (*DevicePathInstance);
  DevPath->SubType  = Temp;

  //
  // If DevPath is the end of an entire device path, then another instance
  // does not follow, so *DevicePath is set to NULL.
  //
  if (DevicePathSubType (DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
    *DevicePathInstance = NULL;
  } else {
    *DevicePathInstance = NextDevicePathNode (DevPath);
  }

  return ReturnValue;
}

BOOLEAN
IsDevicePathMultiInstance (
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
  CONST EFI_DEVICE_PATH_PROTOCOL  *Node;

  if (DevicePath == NULL) {
    return FALSE;
  }

  Node = DevicePath;
  while (!IsDevicePathEnd (Node)) {
    if (EfiIsDevicePathEndInstance (Node)) {
      return TRUE;
    }

    Node = NextDevicePathNode (Node);
  }

  return FALSE;
}

EFI_DEVICE_PATH_PROTOCOL *
CreateDeviceNode (
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
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  if (NodeLength < sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
    return NULL;
  }

  Node = (EFI_DEVICE_PATH_PROTOCOL *) AllocateZeroPool ((UINTN) NodeLength);
  if (Node != NULL) {
    Node->Type    = NodeType;
    Node->SubType = NodeSubType;
    SetDevicePathNodeLength (Node, NodeLength);
  }

  return Node;
}
