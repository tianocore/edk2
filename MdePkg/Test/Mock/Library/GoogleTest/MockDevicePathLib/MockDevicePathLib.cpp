/** @file
  Google Test mocks for DevicePathLib

  Copyright (c) 2025, Yandex. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockDevicePathLib.h>

MOCK_INTERFACE_DEFINITION (MockDevicePathLib);

MOCK_FUNCTION_DEFINITION (MockDevicePathLib, IsDevicePathValid, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, DevicePathType, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, DevicePathSubType, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, DevicePathNodeLength, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, NextDevicePathNode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, IsDevicePathEndType, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, IsDevicePathEnd, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, IsDevicePathEndInstance, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, SetDevicePathNodeLength, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, SetDevicePathEndNode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, GetDevicePathSize, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, DuplicateDevicePath, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, AppendDevicePath, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, AppendDevicePathNode, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, AppendDevicePathInstance, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, GetNextDevicePathInstance, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, CreateDeviceNode, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, IsDevicePathMultiInstance, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, DevicePathFromHandle, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, FileDevicePath, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, ConvertDevicePathToText, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, ConvertDeviceNodeToText, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, ConvertTextToDeviceNode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDevicePathLib, ConvertTextToDevicePath, 1, EFIAPI);
