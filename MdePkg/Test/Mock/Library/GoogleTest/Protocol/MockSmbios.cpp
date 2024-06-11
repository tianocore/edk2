/** @file MockSmbios.cpp
  Google Test mock for Service Binding Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSmbios.h>

MOCK_INTERFACE_DEFINITION (MockSmbiosProtocol);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, Add, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, UpdateString, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, Remove, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, GetNext, 5, EFIAPI);

EFI_SMBIOS_PROTOCOL  SMBIOS_PROTOCOL_INSTANCE = {
  Add,          // EFI_SMBIOS_ADD              Add;
  UpdateString, // EFI_SMBIOS_UPDATE_STRING    UpdateString;
  Remove,       // EFI_SMBIOS_REMOVE           Remove;
  GetNext,      // EFI_SMBIOS_GET_NEXT         GetNext;
  0,            // UINT8                       MajorVersion;
  0             // UINT8                       MinorVersion;
};//

extern "C" {
  EFI_SMBIOS_PROTOCOL  *gSmbiosProtocol = &SMBIOS_PROTOCOL_INSTANCE;
}
