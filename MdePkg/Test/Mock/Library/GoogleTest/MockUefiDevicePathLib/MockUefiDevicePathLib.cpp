/** @file MockUefiDevicePathLib.cpp
  Google Test mocks for UefiDevicePathLib

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiDevicePathLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiDevicePathLib);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, DuplicateDevicePath, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, IsDevicePathValid, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, DevicePathType, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, DevicePathSubType, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, DevicePathNodeLength, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, NextDevicePathNode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, IsDevicePathEndType, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, IsDevicePathEnd, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, IsDevicePathEndInstance, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, SetDevicePathNodeLength, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, SetDevicePathEndNode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, GetDevicePathSize, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, AppendDevicePath, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, AppendDevicePathNode, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, AppendDevicePathInstance, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, GetNextDevicePathInstance, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, CreateDeviceNode, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, IsDevicePathMultiInstance, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, DevicePathFromHandle, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, FileDevicePath, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, ConvertDevicePathToText, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, ConvertDeviceNodeToText, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, ConvertTextToDeviceNode, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiDevicePathLib, ConvertTextToDevicePath, 1, EFIAPI);
