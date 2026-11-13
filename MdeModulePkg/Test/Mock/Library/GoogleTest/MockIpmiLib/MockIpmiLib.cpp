/** @file MockIpmiLib.cpp
  Google Test mocks for IpmiLib

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockIpmiLib.h>

MOCK_INTERFACE_DEFINITION (MockIpmiLib);
MOCK_FUNCTION_DEFINITION (MockIpmiLib, IpmiSubmitCommand, 6, EFIAPI);
