/** @file MockSmmAccess2.cpp
  Declare mock SMM Access2 Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSmmAccess2.h>

MOCK_INTERFACE_DEFINITION (MockSmmAccess2Protocol);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Open, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Close, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, Lock, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmAccess2Protocol, GetCapabilities, 3, EFIAPI);

EFI_SMM_ACCESS2_PROTOCOL  SMM_ACCESS2_PROTOCOL_INSTANCE = {
  Open,
  Close,
  Lock,
  GetCapabilities,
  FALSE,
  FALSE
};

extern "C" {
  EFI_SMM_ACCESS2_PROTOCOL  *gSmmAccess2Protocol = &SMM_ACCESS2_PROTOCOL_INSTANCE;
}
