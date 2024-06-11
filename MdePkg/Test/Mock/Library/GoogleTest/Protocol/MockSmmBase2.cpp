/** @file MockSmmBase2.cpp
  Google Test mocks for SmmBase2 Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSmmBase2.h>

MOCK_INTERFACE_DEFINITION (MockSmmBase2Protocol);
MOCK_FUNCTION_DEFINITION (MockSmmBase2Protocol, InSmm, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmBase2Protocol, GetSmstLocation, 2, EFIAPI);

EFI_SMM_BASE2_PROTOCOL  SMMBASE2_PROTOCOL_INSTANCE = {
  InSmm,           // EFI_SMM_INSIDE_OUT2;
  GetSmstLocation  // EFI_SMM_GET_SMST_LOCATION2;
};

extern "C" {
  EFI_SMM_BASE2_PROTOCOL  *gSmmBase2Protocol = &SMMBASE2_PROTOCOL_INSTANCE;
}
