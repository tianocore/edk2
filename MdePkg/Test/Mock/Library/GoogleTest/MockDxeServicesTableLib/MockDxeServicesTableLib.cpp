/** @file MockDxeServicesTableLib.cpp
  Google Test mocks for DxeServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockDxeServicesTableLib.h>

MOCK_INTERFACE_DEFINITION (MockDxeServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, AddMemorySpace, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, AllocateMemorySpace, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, FreeMemorySpace, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, RemoveMemorySpace, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, GetMemorySpaceDescriptor, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, SetMemorySpaceAttributes, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, GetMemorySpaceMap, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, AddIoSpace, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, AllocateIoSpace, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, FreeIoSpace, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, RemoveIoSpace, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, GetIoSpaceDescriptor, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, GetIoSpaceMap, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, Dispatch, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, Schedule, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, Trust, 0, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, ProcessFirmwareVolume, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDxeServicesTableLib, SetMemorySpaceCapabilities, 3, EFIAPI);

static EFI_DXE_SERVICES  DxeServicesMock = {
  { 0, 0, 0, 0, 0 }, // EFI_TABLE_HEADER
  (EFI_ADD_MEMORY_SPACE)AddMemorySpace,
  (EFI_ALLOCATE_MEMORY_SPACE)AllocateMemorySpace,
  (EFI_FREE_MEMORY_SPACE)FreeMemorySpace,
  (EFI_REMOVE_MEMORY_SPACE)RemoveMemorySpace,
  (EFI_GET_MEMORY_SPACE_DESCRIPTOR)GetMemorySpaceDescriptor,
  (EFI_SET_MEMORY_SPACE_ATTRIBUTES)SetMemorySpaceAttributes,
  (EFI_GET_MEMORY_SPACE_MAP)GetMemorySpaceMap,
  (EFI_ADD_IO_SPACE)AddIoSpace,
  (EFI_ALLOCATE_IO_SPACE)AllocateIoSpace,
  (EFI_FREE_IO_SPACE)FreeIoSpace,
  (EFI_REMOVE_IO_SPACE)RemoveIoSpace,
  (EFI_GET_IO_SPACE_DESCRIPTOR)GetIoSpaceDescriptor,
  (EFI_GET_IO_SPACE_MAP)GetIoSpaceMap,
  (EFI_DISPATCH)Dispatch,
  (EFI_SCHEDULE)Schedule,
  (EFI_TRUST)Trust,
  (EFI_PROCESS_FIRMWARE_VOLUME)ProcessFirmwareVolume,
  (EFI_SET_MEMORY_SPACE_CAPABILITIES)SetMemorySpaceCapabilities
};

extern "C" {
  EFI_DXE_SERVICES  *gDxeServicesMock = &DxeServicesMock;
}
