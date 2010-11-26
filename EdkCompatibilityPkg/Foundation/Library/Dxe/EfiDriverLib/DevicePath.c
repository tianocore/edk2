/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DevicePath.c

Abstract:

  Device Path services. The thing to remember is device paths are built out of
  nodes. The device path is terminated by an end node that is length
  sizeof(EFI_DEVICE_PATH_PROTOCOL). That would be why there is sizeof(EFI_DEVICE_PATH_PROTOCOL)
  all over this file.

  The only place where multi-instance device paths are supported is in
  environment varibles. Multi-instance device paths should never be placed
  on a Handle.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"
#include EFI_PROTOCOL_DEFINITION (DevicePath)

EFI_DEVICE_PATH_PROTOCOL *
EfiDevicePathInstance (
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath,
  OUT UINTN                         *Size
  )
/*++

Routine Description:
  Function retrieves the next device path instance from a device path data structure.

Arguments:
  DevicePath           - A pointer to a device path data structure.

  Size                 - A pointer to the size of a device path instance in bytes.

Returns:

  This function returns a pointer to the current device path instance.
  In addition, it returns the size in bytes of the current device path instance in Size,
  and a pointer to the next device path instance in DevicePath.
  If there are no more device path instances in DevicePath, then DevicePath will be set to NULL.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *ReturnValue;
  UINT8                     Temp;

  if (*DevicePath == NULL) {
    if (Size != NULL) {
      *Size = 0;
    }

    return NULL;
  }

  //
  // Find the end of the device path instance
  //
  DevPath = *DevicePath;
  while (!IsDevicePathEndType (DevPath)) {
    DevPath = NextDevicePathNode (DevPath);
  }

  //
  // Compute the size of the device path instance
  //
  if (Size != NULL) {
    *Size = ((UINTN) DevPath - (UINTN) (*DevicePath)) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
  }

  //
  // Make a copy and return the device path instance
  //
  Temp              = DevPath->SubType;
  DevPath->SubType  = END_ENTIRE_DEVICE_PATH_SUBTYPE;
  ReturnValue       = EfiDuplicateDevicePath (*DevicePath);
  DevPath->SubType  = Temp;

  //
  // If DevPath is the end of an entire device path, then another instance
  // does not follow, so *DevicePath is set to NULL.
  //
  if (DevicePathSubType (DevPath) == END_ENTIRE_DEVICE_PATH_SUBTYPE) {
    *DevicePath = NULL;
  } else {
    *DevicePath = NextDevicePathNode (DevPath);
  }

  return ReturnValue;
}

BOOLEAN
EfiIsDevicePathMultiInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:
  Return TRUE is this is a multi instance device path.

Arguments:
  DevicePath  - A pointer to a device path data structure.


Returns:
  TRUE - If DevicePath is multi instance. FALSE - If DevicePath is not multi
  instance.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *Node;

  if (DevicePath == NULL) {
    return FALSE;
  }

  Node = DevicePath;
  while (!EfiIsDevicePathEnd (Node)) {
    if (EfiIsDevicePathEndInstance (Node)) {
      return TRUE;
    }

    Node = EfiNextDevicePathNode (Node);
  }

  return FALSE;
}

UINTN
EfiDevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:

  Calculate the space size of a device path.

Arguments:

  DevicePath  - A specified device path

Returns:

  The size.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *Start;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Start = DevicePath;
  while (!EfiIsDevicePathEnd (DevicePath)) {
    DevicePath = EfiNextDevicePathNode (DevicePath);
  }

  //
  // Compute the size and add back in the size of the end device path structure
  //
  return ((UINTN) DevicePath - (UINTN) Start) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
}

EFI_DEVICE_PATH_PROTOCOL *
EfiDevicePathFromHandle (
  IN EFI_HANDLE  Handle
  )
/*++

Routine Description:

  Get the device path protocol interface installed on a specified handle.

Arguments:

  Handle  - a specified handle

Returns:

  The device path protocol interface installed on that handle.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath = NULL;
  gBS->HandleProtocol (
        Handle,
        &gEfiDevicePathProtocolGuid,
        (VOID *) &DevicePath
        );
  return DevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
EfiDuplicateDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
/*++

Routine Description:

  Duplicate a device path structure.

Arguments:

  DevicePath  - The device path to duplicated.

Returns:

  The duplicated device path.

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
  Size = EfiDevicePathSize (DevicePath);
  if (Size == 0) {
    return NULL;
  }

  //
  // Allocate space for duplicate device path
  //
  NewDevicePath = EfiLibAllocateCopyPool (Size, DevicePath);

  return NewDevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
EfiAppendDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Src2
  )
/*++

Routine Description:
  Function is used to append a Src1 and Src2 together.

Arguments:
  Src1  - A pointer to a device path data structure.

  Src2  - A pointer to a device path data structure.

Returns:

  A pointer to the new device path is returned.
  NULL is returned if space for the new device path could not be allocated from pool.
  It is up to the caller to free the memory used by Src1 and Src2 if they are no longer needed.

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
  if (!Src1) {
    ASSERT (!IsDevicePathUnpacked (Src2));
    return EfiDuplicateDevicePath (Src2);
  }

  if (!Src2) {
    ASSERT (!IsDevicePathUnpacked (Src1));
    return EfiDuplicateDevicePath (Src1);
  }

  //
  // Allocate space for the combined device path. It only has one end node of
  // length EFI_DEVICE_PATH_PROTOCOL
  //
  Size1         = EfiDevicePathSize (Src1);
  Size2         = EfiDevicePathSize (Src2);
  Size          = Size1 + Size2 - sizeof (EFI_DEVICE_PATH_PROTOCOL);

  NewDevicePath = EfiLibAllocateCopyPool (Size, Src1);

  if (NewDevicePath != NULL) {

    //
    // Over write Src1 EndNode and do the copy
    //
    SecondDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) ((CHAR8 *) NewDevicePath + (Size1 - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
    EfiCopyMem (SecondDevicePath, Src2, Size2);
  }

  return NewDevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
EfiAppendDevicePathNode (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Node
  )
/*++

Routine Description:
  Function is used to append a device path node to the end of another device path.

Arguments:
  Src1  - A pointer to a device path data structure.

  Node - A pointer to a device path data structure.

Returns:
  This function returns a pointer to the new device path.
  If there is not enough temporary pool memory available to complete this function,
  then NULL is returned.


--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *Temp;
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     NodeLength;

  //
  // Build a Node that has a terminator on it
  //
  NodeLength  = DevicePathNodeLength (Node);

  Temp        = EfiLibAllocateCopyPool (NodeLength + sizeof (EFI_DEVICE_PATH_PROTOCOL), Node);
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
  NewDevicePath = EfiAppendDevicePath (Src1, Temp);
  gBS->FreePool (Temp);
  return NewDevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
EfiFileDevicePath (
  IN EFI_HANDLE               Device  OPTIONAL,
  IN CHAR16                   *FileName
  )
/*++

Routine Description:

  This function allocates a device path for a file and appends it to an existiong
  device path.

Arguments:
  Device     - A pointer to a device handle.

  FileName   - A pointer to a Null-terminated Unicodestring.

Returns:
  A device path contain the file name.

--*/
{
  UINTN                     Size;
  FILEPATH_DEVICE_PATH      *FilePath;
  EFI_DEVICE_PATH_PROTOCOL  *Eop;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  for (Size = 0; FileName[Size] != 0; Size++)
    ;
  Size        = (Size + 1) * 2;

  FilePath    = EfiLibAllocateZeroPool (Size + SIZE_OF_FILEPATH_DEVICE_PATH + sizeof (EFI_DEVICE_PATH_PROTOCOL));

  DevicePath  = NULL;

  if (FilePath != NULL) {

    //
    // Build a file path
    //
    FilePath->Header.Type     = MEDIA_DEVICE_PATH;
    FilePath->Header.SubType  = MEDIA_FILEPATH_DP;
    SetDevicePathNodeLength (&FilePath->Header, Size + SIZE_OF_FILEPATH_DEVICE_PATH);
    EfiCopyMem (FilePath->PathName, FileName, Size);
    Eop = NextDevicePathNode (&FilePath->Header);
    SetDevicePathEndNode (Eop);

    //
    // Append file path to device's device path
    //

    DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) FilePath;
    if (Device != NULL) {
      DevicePath = EfiAppendDevicePath (
                    EfiDevicePathFromHandle (Device),
                    DevicePath
                    );

      gBS->FreePool (FilePath);
    }
  }

  return DevicePath;
}

EFI_DEVICE_PATH_PROTOCOL *
EfiAppendDevicePathInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src,
  IN EFI_DEVICE_PATH_PROTOCOL  *Instance
  )
/*++

Routine Description:

  Append a device path instance to another.

Arguments:

  Src       - The device path instance to be appended with.
  Instance  - The device path instance appending the other.

Returns:

  The contaction of these two.

--*/
{
  UINT8                     *Ptr;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  UINTN                     SrcSize;
  UINTN                     InstanceSize;

  if (Src == NULL) {
    return EfiDuplicateDevicePath (Instance);
  }

  SrcSize       = EfiDevicePathSize (Src);
  InstanceSize  = EfiDevicePathSize (Instance);

  Ptr           = EfiLibAllocateCopyPool (SrcSize + InstanceSize, Src);
  if (Ptr != NULL) {

    DevPath = (EFI_DEVICE_PATH_PROTOCOL *) Ptr;

    while (!IsDevicePathEnd (DevPath)) {
      DevPath = NextDevicePathNode (DevPath);
    }
    //
    // Convert the End to an End Instance, since we are
    //  appending another instacne after this one its a good
    //  idea.
    //
    DevPath->SubType  = END_INSTANCE_DEVICE_PATH_SUBTYPE;

    DevPath           = NextDevicePathNode (DevPath);
    EfiCopyMem (DevPath, Instance, InstanceSize);
  }

  return (EFI_DEVICE_PATH_PROTOCOL *) Ptr;
}

VOID
EFIAPI
EfiInitializeFwVolDevicepathNode (
  IN  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *FvDevicePathNode,
  IN EFI_GUID                               *NameGuid
  )
/*++

Routine Description:

  Initialize a Firmware Volume (FV) Media Device Path node.
  
Arguments:

  FvDevicePathNode  - Pointer to a FV device path node to initialize
  NameGuid          - FV file name to use in FvDevicePathNode

Returns:

  None

--*/
{
  FvDevicePathNode->Header.Type     = MEDIA_DEVICE_PATH;
  FvDevicePathNode->Header.SubType  = MEDIA_FV_FILEPATH_DP;
  SetDevicePathNodeLength (&FvDevicePathNode->Header, sizeof (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH));

  EfiCopyMem (&FvDevicePathNode->NameGuid, NameGuid, sizeof(EFI_GUID));
}

EFI_GUID *
EFIAPI
EfiGetNameGuidFromFwVolDevicePathNode (
  IN  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH   *FvDevicePathNode
  )
/*++

Routine Description:

  Check to see if the Firmware Volume (FV) Media Device Path is valid.
  
Arguments:

  FvDevicePathNode  - Pointer to FV device path to check

Returns:

  NULL              - FvDevicePathNode is not valid.
  Other             - FvDevicePathNode is valid and pointer to NameGuid was returned.

--*/
{
  if (DevicePathType (&FvDevicePathNode->Header) == MEDIA_DEVICE_PATH &&
      DevicePathSubType (&FvDevicePathNode->Header) == MEDIA_FV_FILEPATH_DP) {
    return &FvDevicePathNode->NameGuid;
  }

  return NULL;
}
