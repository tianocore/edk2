/** @file
  Google Test mocks for DevicePathLib

  Copyright (c) 2025, Yandex. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_DEVICE_PATH_LIB_LIB_H_
#define MOCK_DEVICE_PATH_LIB_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/DevicePathLib.h>
}

struct MockDevicePathLib {
  MOCK_INTERFACE_DECLARATION (MockDevicePathLib);

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsDevicePathValid,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
     IN UINTN                           MaxSize)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    DevicePathType,
    (IN CONST VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT8,
    DevicePathSubType,
    (IN CONST VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    DevicePathNodeLength,
    (IN CONST VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    NextDevicePathNode,
    (IN CONST VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsDevicePathEndType,
    (IN CONST VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsDevicePathEnd,
    (IN CONST VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsDevicePathEndInstance,
    (IN CONST VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    UINT16,
    SetDevicePathNodeLength,
    (IN OUT VOID  *Node,
     IN UINTN     Length)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    SetDevicePathEndNode,
    (OUT VOID  *Node)
    );

  MOCK_FUNCTION_DECLARATION (
    UINTN,
    GetDevicePathSize,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    DuplicateDevicePath,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    AppendDevicePath,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *FirstDevicePath   OPTIONAL,
     IN CONST EFI_DEVICE_PATH_PROTOCOL  *SecondDevicePath  OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    AppendDevicePathNode,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath      OPTIONAL,
     IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathNode  OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    AppendDevicePathInstance,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath         OPTIONAL,
     IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePathInstance OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    GetNextDevicePathInstance,
    (IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
     OUT UINTN                        *Size)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    CreateDeviceNode,
    (IN UINT8   NodeType,
     IN UINT8   NodeSubType,
     IN UINT16  NodeLength)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    IsDevicePathMultiInstance,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    DevicePathFromHandle,
    (IN EFI_HANDLE  Handle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    FileDevicePath,
    (IN EFI_HANDLE    Device      OPTIONAL,
     IN CONST CHAR16  *FileName)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ConvertDevicePathToText,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
     IN BOOLEAN                         DisplayOnly,
     IN BOOLEAN                         AllowShortcuts)
    );

  MOCK_FUNCTION_DECLARATION (
    CHAR16 *,
    ConvertDeviceNodeToText,
    (IN CONST EFI_DEVICE_PATH_PROTOCOL  *DeviceNode,
     IN BOOLEAN                         DisplayOnly,
     IN BOOLEAN                         AllowShortcuts)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    ConvertTextToDeviceNode,
    (IN CONST CHAR16  *TextDeviceNode)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    ConvertTextToDevicePath,
    (IN CONST CHAR16  *TextDevicePath)
    );
};

#endif // MOCK_DEVICE_PATH_LIB_H_
