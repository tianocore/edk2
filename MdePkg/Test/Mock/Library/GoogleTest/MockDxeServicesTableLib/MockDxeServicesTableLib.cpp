/** @file MockDxeServicesTableLib.cpp
  Google Test mocks for DxeServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockDxeServicesTableLib.h>

MOCK_INTERFACE_DEFINITION (MockDxeServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, gDS_Dispatch, 0, EFIAPI);

static EFI_DXE_SERVICES  LocalDs = {
  { 0, 0, 0, 0, 0 },                                                                   // EFI_TABLE_HEADER
  NULL,                                                                                // EFI_ADD_MEMORY_SPACE
  NULL,                                                                                // EFI_ALLOCATE_MEMORY_SPACE
  NULL,                                                                                // EFI_FREE_MEMORY_SPACE
  NULL,                                                                                // EFI_REMOVE_MEMORY_SPACE
  NULL,                                                                                // EFI_GET_MEMORY_SPACE_DESCRIPTOR
  NULL,                                                                                // EFI_SET_MEMORY_SPACE_ATTRIBUTES
  NULL,                                                                                // EFI_GET_MEMORY_SPACE_MAP
  NULL,                                                                                // EFI_ADD_IO_SPACE
  NULL,                                                                                // EFI_ALLOCATE_IO_SPACE
  NULL,                                                                                // EFI_FREE_IO_SPACE
  NULL,                                                                                // EFI_REMOVE_IO_SPACE
  NULL,                                                                                // EFI_GET_IO_SPACE_DESCRIPTOR
  NULL,                                                                                // EFI_GET_IO_SPACE_MAP
  gDS_Dispatch,                                                                        // EFI_DISPATCH
  NULL,                                                                                // EFI_SCHEDULE
  NULL,                                                                                // EFI_TRUST
  NULL,                                                                                // EFI_PROCESS_FIRMWARE_VOLUME
  NULL                                                                                 // EFI_SET_MEMORY_SPACE_CAPABILITIES
};

extern "C" {
  EFI_DXE_SERVICES  *gDS = &LocalDs;
}
