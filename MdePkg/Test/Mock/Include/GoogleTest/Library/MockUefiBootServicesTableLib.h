/** @file
  Google Test mocks for UefiBootServicesTableLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_BOOT_SERVICES_TABLE_LIB_H_
#define MOCK_UEFI_BOOT_SERVICES_TABLE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
}

struct MockUefiBootServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockUefiBootServicesTableLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateProtocol,
    (IN   EFI_GUID  *Protocol,
     IN   VOID      *Registration  OPTIONAL,
     OUT  VOID      **Interface)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateHandleBuffer,
    (IN   EFI_LOCATE_SEARCH_TYPE    SearchType,
     IN   EFI_GUID                  *Protocol       OPTIONAL,
     IN   VOID                      *SearchKey      OPTIONAL,
     OUT  UINTN                     *NoHandles,
     OUT  EFI_HANDLE                **Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_DisconnectController,
    (IN  EFI_HANDLE   ControllerHandle,
     IN  EFI_HANDLE   DriverImageHandle  OPTIONAL,
     IN  EFI_HANDLE   ChildHandle        OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_FreePool,
    (IN  VOID  *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_ConnectController,
    (IN  EFI_HANDLE                 ControllerHandle,
     IN  EFI_HANDLE                 *DriverImageHandle    OPTIONAL,
     IN  EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath  OPTIONAL,
     IN  BOOLEAN                    Recursive)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_HandleProtocol,
    (IN  EFI_HANDLE    Handle,
     IN  EFI_GUID      *Protocol,
     OUT VOID          **Interface)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CopyMem,
    (IN VOID     *Destination,
     IN VOID     *Source,
     IN UINTN    Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_Stall,
    (IN  UINTN  Microseconds)
    );
};

#endif
