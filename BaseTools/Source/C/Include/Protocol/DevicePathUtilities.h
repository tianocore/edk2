/** @file
  EFI_DEVICE_PATH_UTILITIES_PROTOCOL as defined in UEFI 2.0.
  Use to create and manipulate device paths and device nodes.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __DEVICE_PATH_UTILITIES_H__
#define __DEVICE_PATH_UTILITIES_H__

///
/// Device Path Utilities protocol
///
#define EFI_DEVICE_PATH_UTILITIES_GUID \
  { \
    0x379be4e, 0xd706, 0x437d, {0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4 } \
  }

/**
  Returns the size of the device path, in bytes.

  @param  DevicePath Points to the start of the EFI device path.

  @return Size  Size of the specified device path, in bytes, including the end-of-path tag.
  @retval 0     DevicePath is NULL

**/
typedef
UINTN
( *EFI_DEVICE_PATH_UTILS_GET_DEVICE_PATH_SIZE)(
   CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath
  );


/**
  Create a duplicate of the specified path.

  @param  DevicePath Points to the source EFI device path.

  @retval Pointer    A pointer to the duplicate device path.
  @retval NULL       insufficient memory or DevicePath is NULL

**/
typedef
EFI_DEVICE_PATH_PROTOCOL*
( *EFI_DEVICE_PATH_UTILS_DUP_DEVICE_PATH)(
   CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath
  );

/**
  Create a new path by appending the second device path to the first.
  If Src1 is NULL and Src2 is non-NULL, then a duplicate of Src2 is returned.
  If Src1 is non-NULL and Src2 is NULL, then a duplicate of Src1 is returned.
  If Src1 and Src2 are both NULL, then a copy of an end-of-device-path is returned.

  @param  Src1 Points to the first device path.
  @param  Src2 Points to the second device path.

  @retval Pointer  A pointer to the newly created device path.
  @retval NULL     Memory could not be allocated

**/
typedef
EFI_DEVICE_PATH_PROTOCOL*
( *EFI_DEVICE_PATH_UTILS_APPEND_PATH)(
   CONST EFI_DEVICE_PATH_PROTOCOL *Src1,
   CONST EFI_DEVICE_PATH_PROTOCOL *Src2
  );

/**
  Creates a new path by appending the device node to the device path.
  If DeviceNode is NULL then a copy of DevicePath is returned.
  If DevicePath is NULL then a copy of DeviceNode, followed by an end-of-device path device node is returned.
  If both DeviceNode and DevicePath are NULL then a copy of an end-of-device-path device node is returned.

  @param  DevicePath Points to the device path.
  @param  DeviceNode Points to the device node.

  @retval Pointer    A pointer to the allocated device node.
  @retval NULL       There was insufficient memory.

**/
typedef
EFI_DEVICE_PATH_PROTOCOL*
( *EFI_DEVICE_PATH_UTILS_APPEND_NODE)(
   CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
   CONST EFI_DEVICE_PATH_PROTOCOL *DeviceNode
  );

/**
  Creates a new path by appending the specified device path instance to the specified device path.

  @param  DevicePath         Points to the device path. If NULL, then ignored.
  @param  DevicePathInstance Points to the device path instance.

  @retval Pointer            A pointer to the newly created device path
  @retval NULL               Memory could not be allocated or DevicePathInstance is NULL.

**/
typedef
EFI_DEVICE_PATH_PROTOCOL*
( *EFI_DEVICE_PATH_UTILS_APPEND_INSTANCE)(
   CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
   CONST EFI_DEVICE_PATH_PROTOCOL *DevicePathInstance
  );

/**
  Creates a copy of the current device path instance and returns a pointer to the next device path
  instance.

  @param  DevicePathInstance     On input, this holds the pointer to the current device path
                                 instance. On output, this holds the pointer to the next
                                 device path instance or NULL if there are no more device
                                 path instances in the device path.
  @param  DevicePathInstanceSize On output, this holds the size of the device path instance,
                                 in bytes or zero, if DevicePathInstance is NULL.
                                 If NULL, then the instance size is not output.

  @retval Pointer                A pointer to the copy of the current device path instance.
  @retval NULL                   DevicePathInstance was NULL on entry or there was insufficient memory.

**/
typedef
EFI_DEVICE_PATH_PROTOCOL*
( *EFI_DEVICE_PATH_UTILS_GET_NEXT_INSTANCE)(
     EFI_DEVICE_PATH_PROTOCOL  **DevicePathInstance,
   UINTN                         *DevicePathInstanceSize
  );

/**
  Creates a device node

  @param  NodeType    NodeType is the device node type (EFI_DEVICE_PATH.Type) for
                      the new device node.
  @param  NodeSubType NodeSubType is the device node sub-type
                      EFI_DEVICE_PATH.SubType) for the new device node.
  @param  NodeLength  NodeLength is the length of the device node
                      (EFI_DEVICE_PATH.Length) for the new device node.

  @retval Pointer     A pointer to the newly created device node.
  @retval NULL        NodeLength is less than
                      the size of the header or there was insufficient memory.

**/
typedef
EFI_DEVICE_PATH_PROTOCOL*
( *EFI_DEVICE_PATH_UTILS_CREATE_NODE)(
   UINT8                          NodeType,
   UINT8                          NodeSubType,
   UINT16                         NodeLength
);

/**
  Returns whether a device path is multi-instance.

  @param  DevicePath Points to the device path. If NULL, then ignored.

  @retval TRUE       The device path has more than one instance
  @retval FALSE      The device path is empty or contains only a single instance.

**/
typedef
BOOLEAN
( *EFI_DEVICE_PATH_UTILS_IS_MULTI_INSTANCE)(
   CONST EFI_DEVICE_PATH_PROTOCOL         *DevicePath
  );

///
/// This protocol is used to creates and manipulates device paths and device nodes.
///
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

VOID
SetDevicePathEndNode (
   VOID  *Node
  );

BOOLEAN
IsDevicePathValid (
   CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath,
   UINTN                    MaxSize
  );

UINT8
DevicePathType (
   CONST VOID  *Node
  );

UINT8
DevicePathSubType (
   CONST VOID  *Node
  );

UINTN
DevicePathNodeLength (
   CONST VOID  *Node
  );

EFI_DEVICE_PATH_PROTOCOL *
NextDevicePathNode (
   CONST VOID  *Node
  );

BOOLEAN
IsDevicePathEndType (
   CONST VOID  *Node
  );

BOOLEAN
IsDevicePathEnd (
   CONST VOID  *Node
  );
BOOLEAN
IsDevicePathEndInstance (
   CONST VOID  *Node
  );

UINT16
SetDevicePathNodeLength (
    VOID  *Node,
   UINTN     Length
  );

VOID
SetDevicePathEndNode (
   VOID  *Node
  );

UINTN
UefiDevicePathLibGetDevicePathSize (
   CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

EFI_DEVICE_PATH_PROTOCOL *
UefiDevicePathLibDuplicateDevicePath (
   CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

EFI_DEVICE_PATH_PROTOCOL *
UefiDevicePathLibAppendDevicePath (
   CONST EFI_DEVICE_PATH_PROTOCOL  *FirstDevicePath,
   CONST EFI_DEVICE_PATH_PROTOCOL  *SecondDevicePath
  );

EFI_DEVICE_PATH_PROTOCOL *
UefiDevicePathLibAppendDevicePathNode (
   CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
   CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode
  );

EFI_DEVICE_PATH_PROTOCOL *
UefiDevicePathLibAppendDevicePathInstance (
   CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
   CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathInstance
  );

EFI_DEVICE_PATH_PROTOCOL *
UefiDevicePathLibGetNextDevicePathInstance (
    EFI_DEVICE_PATH_PROTOCOL    **DevicePath,
   UINTN                          *Size
  );

EFI_DEVICE_PATH_PROTOCOL *
UefiDevicePathLibCreateDeviceNode (
   UINT8                           NodeType,
   UINT8                           NodeSubType,
   UINT16                          NodeLength
  );

BOOLEAN
UefiDevicePathLibIsDevicePathMultiInstance (
   CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

#endif
