/** @file MockPlatformHookLib.cpp
  Mock instance of the PCI Host Bridge Library.

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockPlatformHookLib.h>

MOCK_INTERFACE_DEFINITION (MockPlatformHookLib);

MOCK_FUNCTION_DEFINITION (MockPlatformHookLib, PlatformHookSerialPortInitialize, 0, EFIAPI);
