/*++

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DevicePathUtilities.h

Abstract:

--*/

#ifndef _DEVICE_PATH_UTILITIES_PROTOCOL_H_
#define _DEVICE_PATH_UTILITIES_PROTOCOL_H_

//
// Device Path Utilities protocol
//
#define EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID \
  { \
    0x379be4e, 0xd706, 0x437d, {0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4}  \
  }

typedef
UINTN
(EFIAPI *EFI_DEVICE_PATH_UTILS_GET_DEVICE_PATH_SIZE) (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath
  )
/*++

  Routine Description:
    Returns the size of the device path, in bytes.

  Arguments:
    DevicePath  -   Points to the start of the EFI device path.

  Returns:
    Size        -   Size of the specified device path, in bytes, including the end-of-path tag.

--*/
;

typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *EFI_DEVICE_PATH_UTILS_DUP_DEVICE_PATH) (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath
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
;

typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *EFI_DEVICE_PATH_UTILS_APPEND_PATH) (
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
;

typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *EFI_DEVICE_PATH_UTILS_APPEND_NODE) (
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
;

typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *EFI_DEVICE_PATH_UTILS_APPEND_INSTANCE) (
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
;

typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *EFI_DEVICE_PATH_UTILS_GET_NEXT_INSTANCE) (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePathInstance,
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
;

typedef
BOOLEAN
(EFIAPI *EFI_DEVICE_PATH_UTILS_IS_MULTI_INSTANCE) (
  IN CONST EFI_DEVICE_PATH_PROTOCOL         *DevicePath
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
;

typedef
EFI_DEVICE_PATH_PROTOCOL*
(EFIAPI *EFI_DEVICE_PATH_UTILS_CREATE_NODE) (
  IN UINT8                          NodeType,
  IN UINT8                          NodeSubType,
  IN UINT16                         NodeLength
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
;

typedef struct {
  EFI_DEVICE_PATH_UTILS_GET_DEVICE_PATH_SIZE GetDevicePathSize;
  EFI_DEVICE_PATH_UTILS_DUP_DEVICE_PATH      DuplicateDevicePath;
  EFI_DEVICE_PATH_UTILS_APPEND_PATH          AppendDevicePath;
  EFI_DEVICE_PATH_UTILS_APPEND_NODE          AppendDeviceNode;
  EFI_DEVICE_PATH_UTILS_APPEND_INSTANCE      AppendDevicePathInstance;
  EFI_DEVICE_PATH_UTILS_GET_NEXT_INSTANCE    GetNextDevicePathInstance;
  EFI_DEVICE_PATH_UTILS_IS_MULTI_INSTANCE    IsDevicePathMultiInstance;
  EFI_DEVICE_PATH_UTILS_CREATE_NODE          CreateDeviceNode;
} EFI_DEVICE_PATH_UTILITIES_PROTOCOL;

extern EFI_GUID gEfiDevicePathUtilitiesProtocolGuid;

#endif
