/** @file
  Entry point to a DXE Boot Services Driver

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DevicePathLib.h

**/

#ifndef __DEVICE_PATH_LIB_H__
#define __DEVICE_PATH_LIB_H__

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
;

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
;

/**
  This function appends the device path SecondDevicePath
  to every device path instance in FirstDevicePath. 

  @param  FirstDevicePath A pointer to a device path data structure.
  
  @param  SecondDevicePath A pointer to a device path data structure.

  @return
  A pointer to the new device path is returned.
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
;

/**
  This function appends the device path node SecondDevicePath
  to every device path instance in FirstDevicePath.

  @param  DevicePath A pointer to a device path data structure.
  
  @param  DevicePathNode A pointer to a single device path node.

  @return A pointer to the new device path.
  If there is not enough temporary pool memory available to complete this function,
  then NULL is returned.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathNode (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode
  )
;

/**
  This function appends the device path instance Instance to the device path Source.
  If Source is NULL, then a new device path with one instance is created.  

  @param  Source A pointer to a device path data structure.
  @param  Instance A pointer to a device path instance.

  @return
  A pointer to the new device path.
  If there is not enough temporary pool memory available to complete this function,
  then NULL is returned.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
AppendDevicePathInstance (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Source,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *Instance
  )
;

/**
  Function retrieves the next device path instance from a device path data structure.

  @param  DevicePath A pointer to a device path data structure.
  
  @param  Size A pointer to the size of a device path instance in bytes.

  @return
  This function returns a pointer to the current device path instance.
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
;

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
;

/**
  This function retrieves the device path protocol from a handle.

  @param  Handle The handle from which to retrieve the device path protocol.

  @return
  This function returns the device path protocol from the handle specified by Handle.
  If Handle is NULL or Handle does not contain a device path protocol, then NULL is returned.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
DevicePathFromHandle (
  IN EFI_HANDLE  Handle
  )
;

/**
  This function allocates a device path for a file and appends it to an existing device path.

  @param  Device A pointer to a device handle.  This parameter is optional and may be NULL.
  @param  FileName A pointer to a Null-terminated Unicode string.

  @return
  If Device is a valid device handle that contains a device path protocol,
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
;

#endif
