/** @file MockDxeServicesTableLib.h
  Google Test mocks for DxeServicesTableLib

  Copyright (c) Microsoft Corporation.
  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

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
    gDS_Dispatch,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gDS_AddMemorySpace,
    (IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
     IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length,
     IN UINT64                Capabilities)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gDS_AllocateMemorySpace,
    (IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
     IN     EFI_GCD_MEMORY_TYPE    GcdMemoryType,
     IN     UINTN                 Alignment,
     IN     UINT64                Length,
     IN OUT EFI_PHYSICAL_ADDRESS  *BaseAddress,
     IN     EFI_HANDLE            ImageHandle,
     IN     EFI_HANDLE            DeviceHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gDS_FreeMemorySpace,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gDS_RemoveMemorySpace,
    (IN EFI_PHYSICAL_ADDRESS  BaseAddress,
     IN UINT64                Length)
    );
};
