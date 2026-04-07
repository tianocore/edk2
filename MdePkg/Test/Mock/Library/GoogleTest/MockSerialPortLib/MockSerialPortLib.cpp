/** @file MockSerialPortLib.cpp
  Google Test mocks for SerialPortLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockSerialPortLib.h>

MOCK_INTERFACE_DEFINITION (MockSerialPortLib);
MOCK_FUNCTION_DEFINITION (MockSerialPortLib, SerialPortInitialize, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSerialPortLib, SerialPortWrite, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSerialPortLib, SerialPortRead, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSerialPortLib, SerialPortPoll, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSerialPortLib, SerialPortSetControl, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSerialPortLib, SerialPortGetControl, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSerialPortLib, SerialPortSetAttributes, 6, EFIAPI);
