/** @file
  Device Path services. The thing to remember is device paths are built out of
  nodes. The device path is terminated by an end node that is length
  sizeof(EFI_DEVICE_PATH_PROTOCOL). That would be why there is sizeof(EFI_DEVICE_PATH_PROTOCOL)
  all over this file.

  The only place where multi-instance device paths are supported is in
  environment varibles. Multi-instance device paths should never be placed
  on a Handle.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  UefiDevicePathLib.c

**/

/**
  This function returns the size, in bytes, 
  of the device path data structure specified by DevicePath.
  If DevicePath is NULL, then 0 is returned.

  @param  DevicePath A pointer to a device path data structure.

  @return The size of a device path in bytes.

**/
UINTN
EFIAPI
GetDevicePathSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Start;

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

/**
  This function allocates space for a new copy of the device path
  specified by DevicePath.

  @param  DevicePath A pointer to a device path data structure.

  @return The duplicated device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
DuplicateDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     Size;

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
  NewDevicePath = AllocateCopyPool (Size, DevicePath);

  return NewDevicePath;
}

/**
  This function appends the device path SecondDevicePath
  to every device path instance in FirstDevicePath. 

  @param  FirstDevicePath A pointer to a device path data structure.
  
  @param  SecondDevicePath A pointer to a device path data structure.

  @return A pointer to the new device path is returned.
  NULL is returned if space for the new device path could not be allocated from pool.
  It is up to the caller to free the memory used by FirstDevicePath and SecondDevicePath
  if they are no longer needed.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *FirstDevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *SecondDevicePath
  )
{
  UINTN                     Size;
  UINTN                     Size1;
  UINTN                     Size2;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath2;

  //
  // If there's only 1 path, just duplicate it
  //
  if (FirstDevicePath == NULL) {
    return DuplicateDevicePath (SecondDevicePath);
  }

  if (SecondDevicePath == NULL) {
    return DuplicateDevicePath (FirstDevicePath);
  }

  //
  // Allocate space for the combined device path. It only has one end node of
  // length EFI_DEVICE_PATH_PROTOCOL
  //
  Size1         = GetDevicePathSize (FirstDevicePath);
  Size2         = GetDevicePathSize (SecondDevicePath);
  Size          = Size1 + Size2 - sizeof (EFI_DEVICE_PATH_PROTOCOL);

  NewDevicePath = AllocatePool (Size);

  if (NewDevicePath != NULL) {
    NewDevicePath = CopyMem (NewDevicePath, FirstDevicePath, Size1);
    //
    // Over write Src1 EndNode and do the copy
    //
    DevicePath2 = (EFI_DEVICE_PATH_PROTOCOL *) ((CHAR8 *) NewDevicePath + (Size1 - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
    CopyMem (DevicePath2, SecondDevicePath, Size2);
  }

  return NewDevicePath;
}

/**
  This function appends the device path node SecondDevicePath
  to every device path instance in FirstDevicePath.

  @param  FirstDevicePath A pointer to a device path data structure.
  
  @param  SecondDevicePath A pointer to a single device path node.

  @return A pointer to the new device path.
  If there is not enough temporary pool memory available to complete this function,
  then NULL is returned.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathNode (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *FirstDevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *SecondDevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *NextNode;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  UINTN                     NodeLength;
  UINTN                     Size1;

  //
  // Build a Node that has a terminator on it
  //
  NodeLength  = DevicePathNodeLength (SecondDevicePath);
  Size1       = GetDevicePathSize (FirstDevicePath);
  
  NewDevicePath = AllocatePool (NodeLength + Size1);
  if (NewDevicePath != NULL) {
    //
    // Copy the first device path to the new device path
    //
    NewDevicePath = CopyMem (NewDevicePath, FirstDevicePath, Size1);

    //
    // Copy the device path node to the new device path
    //
    NextNode      = (EFI_DEVICE_PATH_PROTOCOL *) ((CHAR8 *) NewDevicePath + (Size1 - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
    NextNode      = CopyMem (NextNode, SecondDevicePath, NodeLength);

    //
    // Terminate the whole device path
    //
    NextNode      = NextDevicePathNode (NextNode);
    SetDevicePathEndNode (NextNode);
  }
  return NewDevicePath;
}

/**
  This function appends the device path instance Instance to the device path Source.
  If Source is NULL, then a new device path with one instance is created.  

  @param  Source A pointer to a device path data structure.
  @param  Instance A pointer to a device path instance.

  @return A pointer to the new device path.
  If there is not enough temporary pool memory available to complete this function,
  then NULL is returned.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathInstance (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Source,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Instance
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     SrcSize;
  UINTN                     InstanceSize;

  if (Source == NULL) {
    return DuplicateDevicePath (Instance);
  }

  SrcSize       = GetDevicePathSize (Source);
  InstanceSize  = GetDevicePathSize (Instance);

  NewDevicePath = AllocatePool (SrcSize + InstanceSize);
  if (NewDevicePath != NULL) {
    
    DevicePath = CopyMem (NewDevicePath, Source, SrcSize);;
 
    while (!IsDevicePathEnd (DevicePath)) {
      DevicePath = NextDevicePathNode (DevicePath);
    }
    
    DevicePath->SubType  = END_INSTANCE_DEVICE_PATH_SUBTYPE;

    DevicePath           = NextDevicePathNode (DevicePath);
    CopyMem (DevicePath, Instance, InstanceSize);
  }

  return NewDevicePath;
}

/**
  Function retrieves the next device path instance from a device path data structure.

  @param  DevicePath A pointer to a device path data structure.
  
  @param  Size A pointer to the size of a device path instance in bytes.

  @return This function returns a pointer to the current device path instance.
  In addition, it returns the size in bytes of the current device path instance in Size,
  and a pointer to the next device path instance in DevicePath.
  If there are no more device path instances in DevicePath, then DevicePath will be set to NULL.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
GetNextDevicePathInstance (
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
     OUT UINTN                     *Size
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_DEVICE_PATH_PROTOCOL  *ReturnValue;
  UINT8                     Temp;

  ASSERT (DevicePath != NULL);
  ASSERT (Size != NULL);
  if (*DevicePath == NULL) {
    *Size = 0;
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
  *Size = ((UINTN) DevPath - (UINTN) (*DevicePath)) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
 
  //
  // Make a copy and return the device path instance
  //
  Temp              = DevPath->SubType;
  DevPath->SubType  = END_ENTIRE_DEVICE_PATH_SUBTYPE;
  ReturnValue       = DuplicateDevicePath (*DevicePath);
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

/**
  Return TRUE is this is a multi instance device path.

  @param  DevicePath A pointer to a device path data structure.

  @retval  TRUE If DevicePath is multi-instance.
  @retval  FALSE If DevicePath is not multi-instance or DevicePath is NULL.

**/
BOOLEAN
EFIAPI
IsDevicePathMultiInstance (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Node;

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

/**
  This function retrieves the device path protocol from a handle.

  @param  Handle The handle from which to retrieve the device path protocol.

  @return This function returns the device path protocol from the handle specified by Handle.
  If Handle is NULL or Handle does not contain a device path protocol, then NULL is returned.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
DevicePathFromHandle (
  IN EFI_HANDLE  Handle
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_STATUS                Status;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID *) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    DevicePath = NULL;
  }
  return DevicePath;
}

/**
  This function allocates a device path for a file and appends it to an existing device path.

  @param  Device A pointer to a device handle.  This parameter is optional and may be NULL.
  @param  FileName A pointer to a Null-terminated Unicode string.

  @return If Device is a valid device handle that contains a device path protocol,
  then a device path for the file specified by FileName is allocated
  and appended to the device path associated with the handle Device. The allocated device path is returned.
  If Device is NULL or Device is a handle that does not support the device path protocol,
  then a device path containing a single device path node for the file specified by FileName
  is allocated and returned.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
FileDevicePath (
  IN EFI_HANDLE    Device,     OPTIONAL
  IN CONST CHAR16  *FileName
  )
{
  UINTN                     FileNameSize;
  UINTN                     FilePathNodeSize;
  FILEPATH_DEVICE_PATH      *FilePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  DevicePath        = NULL;

  FileNameSize      = StrSize (FileName);
  FilePathNodeSize  = FileNameSize + SIZE_OF_FILEPATH_DEVICE_PATH;
  FilePathNode      = AllocatePool (FilePathNodeSize);
  if (FilePathNode != NULL) {
    //
    // Build a file path node
    //
    FilePathNode->Header.Type     = MEDIA_DEVICE_PATH;
    FilePathNode->Header.SubType  = MEDIA_FILEPATH_DP;
    SetDevicePathNodeLength (&FilePathNode->Header, FilePathNodeSize);
    CopyMem (FilePathNode->PathName, FileName, FileNameSize);
 
    //
    // Append file path node to device's device path
    //
    if (Device != NULL) {
      DevicePath = DevicePathFromHandle (Device);
    }
    DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL *) FilePathNode);
    FreePool (FilePathNode);
  }
  return DevicePath;
}

