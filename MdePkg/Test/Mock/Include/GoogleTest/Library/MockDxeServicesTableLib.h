/** @file MockDxeServicesTableLib.h
  Google Test mocks for DxeServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_DXE_SERVICES_TABLE_LIB_H_
#define MOCK_DXE_SERVICES_TABLE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiDxeCis.h>
}

//
// Declarations to handle usage of the DxeServicesTableLib by creating mock
//
struct MockDxeServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockDxeServicesTableLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AddMemorySpace,
    (IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
     IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length,
     IN UINT64                Capabilities)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AllocateMemorySpace,
    (IN EFI_GCD_ALLOCATE_TYPE  AllocateType,
     IN EFI_GCD_MEMORY_TYPE    GcdMemoryType,
     IN UINT64                 BaseAddress,
     IN UINT64                 Length,
     IN EFI_GUID               *ImageHandle,
     IN EFI_GUID               *DeviceHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FreeMemorySpace,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RemoveMemorySpace,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetMemorySpaceDescriptor,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetMemorySpaceAttributes,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length,
     IN UINT64                Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetMemorySpaceMap,
    (IN OUT UINTN  *NumberOfDescriptors,
     OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AddIoSpace,
    (IN EFI_GCD_IO_TYPE       GcdIoType,
     IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AllocateIoSpace,
    (IN EFI_GCD_ALLOCATE_TYPE  AllocateType,
     IN EFI_GCD_IO_TYPE        GcdIoType,
     IN UINT64                 BaseAddress,
     IN UINT64                 Length,
     IN EFI_GUID               *ImageHandle,
     IN EFI_GUID               *DeviceHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FreeIoSpace,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RemoveIoSpace,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetIoSpaceDescriptor,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     OUT EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetIoSpaceMap,
    (IN OUT UINTN  *NumberOfDescriptors,
     OUT EFI_GCD_IO_SPACE_DESCRIPTOR  **IoSpaceMap)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Dispatch,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Schedule,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Trust,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ProcessFirmwareVolume,
    (IN CONST VOID                       *FirmwareVolumeHeader,
     IN UINTN                            Size,
     OUT EFI_HANDLE                      *FirmwareVolumeHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetMemorySpaceCapabilities,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length,
     IN UINT64                Capabilities)
    );
};

#endif // MOCK_UEFI_DXE_SERVICES_TABLE_LIB_H_
