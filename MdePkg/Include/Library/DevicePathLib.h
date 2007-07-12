/** @file
  Entry point to a DXE Boot Services Driver

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __DEVICE_PATH_LIB_H__
#define __DEVICE_PATH_LIB_H__

/**
  Returns the size of a device path in bytes.

  This function returns the size, in bytes, of the device path data structure specified by
  DevicePath including the end of device path node.  If DevicePath is NULL, then 0 is returned.

  @param  DevicePath                 A pointer to a device path data structure.

  @return The size of a device path in bytes.

**/
UINTN
EFIAPI
GetDevicePathSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

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
DuplicateDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Creates a new device path by appending a second device path to a first device path.

  This function creates a new device path by appending a copy of SecondDevicePath to a copy of
  FirstDevicePath in a newly allocated buffer.  Only the end-of-device-path device node from
  SecondDevicePath is retained. The newly created device path is returned.  
  If FirstDevicePath is NULL, then it is ignored, and a duplicate of SecondDevicePath is returned.  
  If SecondDevicePath is NULL, then it is ignored, and a duplicate of FirstDevicePath is returned.  
  If both FirstDevicePath and SecondDevicePath are NULL, then NULL is returned.  
  If there is not enough memory for the newly allocated buffer, then NULL is returned.
  The memory for the new device path is allocated from EFI boot services memory. It is the
  responsibility of the caller to free the memory allocated.

  @param  FirstDevicePath            A pointer to a device path data structure.
  @param  SecondDevicePath           A pointer to a device path data structure.

  @return A pointer to the new device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *FirstDevicePath,  OPTIONAL
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *SecondDevicePath  OPTIONAL
  );

/**
  Creates a new path by appending the device node to the device path.

  This function creates a new device path by appending a copy of the device node specified by
  DevicePathNode to a copy of the device path specified by DevicePath in an allocated buffer.
  The end-of-device-path device node is moved after the end of the appended device node.
  If DevicePath is NULL, then NULL is returned.
  If DevicePathNode is NULL, then NULL is returned.
  If there is not enough memory to allocate space for the new device path, then NULL is returned.  
  The memory is allocated from EFI boot services memory. It is the responsibility of the caller to
  free the memory allocated.

  @param  DevicePath                 A pointer to a device path data structure.
  @param  DevicePathNode             A pointer to a single device path node.

  @return A pointer to the new device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathNode (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,     OPTIONAL
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode  OPTIONAL
  );

/**
  Creates a new device path by appending the specified device path instance to the specified device
  path.
 
  This function creates a new device path by appending a copy of the device path instance specified
  by DevicePathInstance to a copy of the device path secified by DevicePath in a allocated buffer.
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
AppendDevicePathInstance (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,        OPTIONAL
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathInstance OPTIONAL
  );

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
GetNextDevicePathInstance (
  IN OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath,
  OUT UINTN                          *Size
  );

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
CreateDeviceNode (
  IN UINT8                           NodeType,
  IN UINT8                           NodeSubType,
  IN UINT16                          NodeLength
  );

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
IsDevicePathMultiInstance (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

/**
  Retrieves the device path protocol from a handle.

  This function returns the device path protocol from the handle specified by Handle.  If Handle is
  NULL or Handle does not contain a device path protocol, then NULL is returned.
 
  @param  Handle                     The handle from which to retrieve the device path protocol.

  @return The device path protocol from the handle specified by Handle.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
DevicePathFromHandle (
  IN EFI_HANDLE                      Handle
  );

/**
  Allocates a device path for a file and appends it to an existing device path.

  If Device is a valid device handle that contains a device path protocol, then a device path for
  the file specified by FileName  is allocated and appended to the device path associated with the
  handle Device.  The allocated device path is returned.  If Device is NULL or Device is a handle
  that does not support the device path protocol, then a device path containing a single device
  path node for the file specified by FileName is allocated and returned.
  If FileName is NULL, then ASSERT().

  @param  Device                     A pointer to a device handle.  This parameter is optional and
                                     may be NULL.
  @param  FileName                   A pointer to a Null-terminated Unicode string.

  @return The allocated device path.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
FileDevicePath (
  IN EFI_HANDLE                      Device,     OPTIONAL
  IN CONST CHAR16                    *FileName
  );

#endif
