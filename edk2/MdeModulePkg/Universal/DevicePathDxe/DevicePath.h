/** @file
  Definition for Device Path Utilities driver

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DEVICE_PATH_DRIVER_H
#define _DEVICE_PATH_DRIVER_H

#include <PiDxe.h>
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

extern const EFI_GUID mEfiDevicePathMessagingUartFlowControlGuid;
extern const EFI_GUID mEfiDevicePathMessagingSASGuid;

#define MAX_CHAR                   480

#define MIN_ALIGNMENT_SIZE         sizeof(UINTN)
#define ALIGN_SIZE(a)              ((a % MIN_ALIGNMENT_SIZE) ? MIN_ALIGNMENT_SIZE - (a % MIN_ALIGNMENT_SIZE) : 0)

#define IS_COMMA(a)                ((a) == L',')
#define IS_HYPHEN(a)               ((a) == L'-')
#define IS_DOT(a)                  ((a) == L'.')
#define IS_LEFT_PARENTH(a)         ((a) == L'(')
#define IS_RIGHT_PARENTH(a)        ((a) == L')')
#define IS_SLASH(a)                ((a) == L'/')
#define IS_NULL(a)                 ((a) == L'\0')

#define DEVICE_NODE_END            1
#define DEVICE_PATH_INSTANCE_END   2
#define DEVICE_PATH_END            3

#define SetDevicePathInstanceEndNode(a) {                \
    (a)->Type       = END_DEVICE_PATH_TYPE;              \
    (a)->SubType    = END_INSTANCE_DEVICE_PATH_SUBTYPE;  \
    (a)->Length[0]  = sizeof (EFI_DEVICE_PATH_PROTOCOL); \
    (a)->Length[1]  = 0;                                 \
  }

//
// Private Data structure
//
typedef struct {
  CHAR16  *Str;
  UINTN   Len;
  UINTN   MaxLen;
} POOL_PRINT;

typedef struct {
  UINT8   Type;
  UINT8   SubType;
  VOID    (*Function) (POOL_PRINT *, VOID *, BOOLEAN, BOOLEAN);
} DEVICE_PATH_TO_TEXT_TABLE;

typedef struct {
  CHAR16                    *DevicePathNodeText;
  EFI_DEVICE_PATH_PROTOCOL  * (*Function) (CHAR16 *);
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
  UINT32                    HID;
  UINT32                    UID;
  UINT32                    CID;
  CHAR8                     HidUidCidStr[3];
} ACPI_EXTENDED_HID_DEVICE_PATH_WITH_STR;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT16                    NetworkProtocol;
  UINT16                    LoginOption;
  UINT64                    Lun;
  UINT16                    TargetPortalGroupTag;
  CHAR8                     iSCSITargetName[1];
} ISCSI_DEVICE_PATH_WITH_NAME;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEVICE_PATH_WITH_DATA;

#pragma pack()

CHAR16 *
ConvertDeviceNodeToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DeviceNode,
  IN BOOLEAN                         DisplayOnly,
  IN BOOLEAN                         AllowShortcuts
  )
/*++

  Routine Description:
    Convert a device node to its text representation.

  Arguments:
    DeviceNode       -   Points to the device node to be converted.
    DisplayOnly      -   If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
    AllowShortcuts   -   If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

  Returns:
    A pointer        -   a pointer to the allocated text representation of the device node.
    NULL             -   if DeviceNode is NULL or there was insufficient memory.

--*/
;

CHAR16 *
ConvertDevicePathToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DeviceNode,
  IN BOOLEAN                         DisplayOnly,
  IN BOOLEAN                         AllowShortcuts
  )
/*++

  Routine Description:
    Convert a device path to its text representation.

  Arguments:
    DeviceNode       -   Points to the device path to be converted.
    DisplayOnly      -   If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
    AllowShortcuts   -   If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

  Returns:
    A pointer        -   a pointer to the allocated text representation of the device path.
    NULL             -   if DeviceNode is NULL or there was insufficient memory.

--*/
;

EFI_DEVICE_PATH_PROTOCOL *
ConvertTextToDeviceNode (
  IN CONST CHAR16 *TextDeviceNode
  )
/*++

  Routine Description:
    Convert text to the binary representation of a device node.

  Arguments:
    TextDeviceNode   -   TextDeviceNode points to the text representation of a device
                         node. Conversion starts with the first character and continues
                         until the first non-device node character.

  Returns:
    A pointer        -   Pointer to the EFI device node.
    NULL             -   if TextDeviceNode is NULL or there was insufficient memory.

--*/
;

EFI_DEVICE_PATH_PROTOCOL *
ConvertTextToDevicePath (
  IN CONST CHAR16 *TextDevicePath
  )
/*++

  Routine Description:
    Convert text to the binary representation of a device path.

  Arguments:
    TextDevicePath   -   TextDevicePath points to the text representation of a device
                         path. Conversion starts with the first character and continues
                         until the first non-device node character.

  Returns:
    A pointer        -   Pointer to the allocated device path.
    NULL             -   if TextDeviceNode is NULL or there was insufficient memory.

--*/
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;

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
;

#endif
