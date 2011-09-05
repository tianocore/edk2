/** @file
  Definition for Device Path Utilities driver

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DEVICE_PATH_DRIVER_H_
#define _DEVICE_PATH_DRIVER_H_

#include <Uefi.h>
#include <Protocol/DevicePathUtilities.h>
#include <Protocol/DebugPort.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/DevicePathFromText.h>
#include <Guid/PcAnsi.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>

#define IS_COMMA(a)                ((a) == L',')
#define IS_HYPHEN(a)               ((a) == L'-')
#define IS_DOT(a)                  ((a) == L'.')
#define IS_LEFT_PARENTH(a)         ((a) == L'(')
#define IS_RIGHT_PARENTH(a)        ((a) == L')')
#define IS_SLASH(a)                ((a) == L'/')
#define IS_NULL(a)                 ((a) == L'\0')


#define SET_DEVICE_PATH_INSTANCE_END_NODE(a) {                   \
    (a)->Type       = END_DEVICE_PATH_TYPE;                      \
    (a)->SubType    = END_INSTANCE_DEVICE_PATH_SUBTYPE;          \
    (a)->Length[0]  = (UINT8) sizeof (EFI_DEVICE_PATH_PROTOCOL); \
    (a)->Length[1]  = 0;                                         \
  }

//
// Private Data structure
//
typedef struct {
  CHAR16  *Str;
  UINTN   Length;
  UINTN   Capacity;
} POOL_PRINT;

typedef
EFI_DEVICE_PATH_PROTOCOL  *
(*DUMP_NODE) (
  IN  CHAR16 *DeviceNodeStr
  );

typedef
VOID
(*DEVICE_PATH_TO_TEXT_FUNC) (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  );

typedef struct {
  UINT8                     Type;
  UINT8                     SubType;
  DEVICE_PATH_TO_TEXT_FUNC  Function;
} DEVICE_PATH_TO_TEXT_TABLE;

typedef struct {
  CHAR16                    *DevicePathNodeText;
  DUMP_NODE                 Function;
} DEVICE_PATH_FROM_TEXT_TABLE;

typedef struct {
  BOOLEAN ClassExist;
  UINT8   Class;
  BOOLEAN SubClassExist;
  UINT8   SubClass;
} USB_CLASS_TEXT;

#define USB_CLASS_AUDIO            1
#define USB_CLASS_CDCCONTROL       2
#define USB_CLASS_HID              3
#define USB_CLASS_IMAGE            6
#define USB_CLASS_PRINTER          7
#define USB_CLASS_MASS_STORAGE     8
#define USB_CLASS_HUB              9
#define USB_CLASS_CDCDATA          10
#define USB_CLASS_SMART_CARD       11
#define USB_CLASS_VIDEO            14
#define USB_CLASS_DIAGNOSTIC       220
#define USB_CLASS_WIRELESS         224

#define USB_CLASS_RESERVE          254
#define USB_SUBCLASS_FW_UPDATE     1
#define USB_SUBCLASS_IRDA_BRIDGE   2
#define USB_SUBCLASS_TEST          3

#define RFC_1700_UDP_PROTOCOL      17
#define RFC_1700_TCP_PROTOCOL      6

#pragma pack(1)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEFINED_HARDWARE_DEVICE_PATH;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEFINED_MESSAGING_DEVICE_PATH;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEFINED_MEDIA_DEVICE_PATH;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT32                    Hid;
  UINT32                    Uid;
  UINT32                    Cid;
  CHAR8                     HidUidCidStr[3];
} ACPI_EXTENDED_HID_DEVICE_PATH_WITH_STR;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT16                    NetworkProtocol;
  UINT16                    LoginOption;
  UINT64                    Lun;
  UINT16                    TargetPortalGroupTag;
  CHAR8                     TargetName[1];
} ISCSI_DEVICE_PATH_WITH_NAME;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEVICE_PATH_WITH_DATA;

#pragma pack()

/**
  Converts a device node to its string representation.

  @param DeviceNode        A Pointer to the device node to be converted.
  @param DisplayOnly       If DisplayOnly is TRUE, then the shorter text representation
                           of the display node is used, where applicable. If DisplayOnly
                           is FALSE, then the longer text representation of the display node
                           is used.
  @param AllowShortcuts    If AllowShortcuts is TRUE, then the shortcut forms of text
                           representation for a device node can be used, where applicable.

  @return A pointer to the allocated text representation of the device node or NULL if DeviceNode
          is NULL or there was insufficient memory.

**/
CHAR16 *
EFIAPI
ConvertDeviceNodeToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DeviceNode,
  IN BOOLEAN                         DisplayOnly,
  IN BOOLEAN                         AllowShortcuts
  );

/**
  Converts a device path to its text representation.

  @param DevicePath      A Pointer to the device to be converted.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

  @return A pointer to the allocated text representation of the device path or
          NULL if DeviceNode is NULL or there was insufficient memory.

**/
CHAR16 *
EFIAPI
ConvertDevicePathToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN BOOLEAN                          DisplayOnly,
  IN BOOLEAN                          AllowShortcuts
  );

/**
  Convert text to the binary representation of a device node.

  @param TextDeviceNode  TextDeviceNode points to the text representation of a device
                         node. Conversion starts with the first character and continues
                         until the first non-device node character.

  @return A pointer to the EFI device node or NULL if TextDeviceNode is NULL or there was
          insufficient memory or text unsupported.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
ConvertTextToDeviceNode (
  IN CONST CHAR16 *TextDeviceNode
  );

/**
  Convert text to the binary representation of a device path.


  @param TextDevicePath  TextDevicePath points to the text representation of a device
                         path. Conversion starts with the first character and continues
                         until the first non-device node character.

  @return A pointer to the allocated device path or NULL if TextDeviceNode is NULL or
          there was insufficient memory.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
ConvertTextToDevicePath (
  IN CONST CHAR16 *TextDevicePath
  );

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
DuplicateDevicePathProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  );

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
  );

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
  );

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
GetNextDevicePathInstanceProtocolInterface (
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath,
  OUT UINTN                         *Size
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
IsDevicePathMultiInstanceProtocolInterface (
  IN CONST EFI_DEVICE_PATH_PROTOCOL *DevicePath
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
CreateDeviceNodeProtocolInterface (
  IN UINT8  NodeType,
  IN UINT8  NodeSubType,
  IN UINT16 NodeLength
  );

#endif
