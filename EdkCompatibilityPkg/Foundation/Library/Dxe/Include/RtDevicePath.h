/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  RtDevicePath.h

Abstract:

  Device Path services. The thing to remember is device paths are built out of
  nodes. The device path is terminated by an end node that is length
  sizeof(EFI_DEVICE_PATH_PROTOCOL). That would be why there is sizeof(EFI_DEVICE_PATH_PROTOCOL)
  all over this file.

  The only place where multi-instance device paths are supported is in
  environment varibles. Multi-instance device paths should never be placed
  on a Handle.

--*/
#ifndef _RT_DEVICE_PATH_H_
#define _RT_DEVICE_PATH_H_

BOOLEAN
RtEfiIsDevicePathMultiInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:
  Return TRUE is this is a multi instance device path.

Arguments:
  DevicePath  - A pointer to a device path data structure.


Returns:
  TRUE - If DevicePath is multi instance. 
  FALSE - If DevicePath is not multi instance.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
RtEfiDevicePathInstance (
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
;

UINTN
RtEfiDevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

Routine Description:

  Calculate the size of a whole device path.    
    
Arguments:

  DevPath - The pointer to the device path data.
    
Returns:

  Size of device path data structure..

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
RtEfiAppendDevicePath (
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
;

EFI_DEVICE_PATH_PROTOCOL        *
RtEfiDevicePathFromHandle (
  IN EFI_HANDLE       Handle
  )
/*++

Routine Description:

  Locate device path protocol interface on a device handle.

Arguments:

  Handle  - The device handle

Returns:

  Device path protocol interface located.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
RtEfiDuplicateDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

Routine Description:
  Duplicate a new device path data structure from the old one.

Arguments:
  DevPath  - A pointer to a device path data structure.

Returns:
  A pointer to the new allocated device path data.
  Caller must free the memory used by DevicePath if it is no longer needed.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
RtEfiAppendDevicePathNode (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Src2
  )
/*++

Routine Description:
  Function is used to append a device path node to the end of another device path.

Arguments:
  Src1  - A pointer to a device path data structure.

  Src2 - A pointer to a device path data structure.

Returns:
  This function returns a pointer to the new device path.
  If there is not enough temporary pool memory available to complete this function,
  then NULL is returned.


--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
RtEfiFileDevicePath (
  IN EFI_HANDLE               Device  OPTIONAL,
  IN CHAR16                   *FileName
  )
/*++

Routine Description:
  Create a device path that appends a MEDIA_DEVICE_PATH with
  FileNameGuid to the device path of DeviceHandle.

Arguments:
  Device   - Optional Device Handle to use as Root of the Device Path

  FileName - FileName

Returns:
  EFI_DEVICE_PATH_PROTOCOL that was allocated from dynamic memory
  or NULL pointer.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
RtEfiAppendDevicePathInstance (
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
;

#endif
