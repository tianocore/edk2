/** @file
  Google Test mocks for UefiBootManagerLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.
  Copyright (C) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/UefiBootManagerLib.h>
}

struct MockUefiBootManagerLib {
  MOCK_INTERFACE_DECLARATION (MockUefiBootManagerLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_BOOT_MANAGER_LOAD_OPTION *,
    EfiBootManagerGetLoadOptions,
    (OUT UINTN                             *LoadOptionCount,
     IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  LoadOptionType)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerFreeLoadOptions,
    (IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOptions,
     IN  UINTN                         LoadOptionCount)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerInitializeLoadOption,
    (IN OUT EFI_BOOT_MANAGER_LOAD_OPTION    *Option,
     IN  UINTN                              OptionNumber,
     IN  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType,
     IN  UINT32                             Attributes,
     IN  CHAR16                             *Description,
     IN  EFI_DEVICE_PATH_PROTOCOL           *FilePath,
     IN  UINT8                              *OptionalData,
     IN  UINT32                             OptionalDataSize)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerFreeLoadOption,
    (IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerVariableToLoadOption,
    (IN CHAR16                            *VariableName,
     IN OUT EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerAddLoadOptionVariable,
    (IN OUT EFI_BOOT_MANAGER_LOAD_OPTION  *Option,
     IN     UINTN                         Position)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerDeleteLoadOptionVariable,
    (IN UINTN                              OptionNumber,
     IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerSortLoadOptionVariable,
    (IN EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  OptionType,
     IN SORT_COMPARE                       CompareFunction)
    );

  MOCK_FUNCTION_DECLARATION (
    INTN,
    EfiBootManagerFindLoadOption,
    (IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Key,
     IN CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Array,
     IN UINTN                               Count)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerStartHotkeyService,
    (IN EFI_EVENT  *HotkeyTriggered)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerHotkeyBoot,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerRefreshAllBootOption,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerBoot,
    (IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerGetBootManagerMenu,
    (IN  EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    EfiBootManagerGetNextLoadOptionDevicePath,
    (IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
     IN  EFI_DEVICE_PATH_PROTOCOL  *FullPath)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    EfiBootManagerGetLoadOptionBuffer,
    (IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath,
     OUT EFI_DEVICE_PATH_PROTOCOL  **FullPath,
     OUT UINTN                     *FileSize)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerRegisterLegacyBootSupport,
    (EFI_BOOT_MANAGER_REFRESH_LEGACY_BOOT_OPTION  RefreshLegacyBootOption,
     EFI_BOOT_MANAGER_LEGACY_BOOT                 LegacyBoot)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerRegisterBootDescriptionHandler,
    (IN EFI_BOOT_MANAGER_BOOT_DESCRIPTION_HANDLER  Handler)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerConnectAll,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerConnectDevicePath,
    (EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect,
     EFI_HANDLE                *MatchingHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    EfiBootManagerDisconnectAll,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerConnectAllDefaultConsoles,
    ()
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerUpdateConsoleVariable,
    (IN  CONSOLE_TYPE              ConsoleType,
     IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
     IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerConnectConsoleVariable,
    (IN  CONSOLE_TYPE  ConsoleType)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_DEVICE_PATH_PROTOCOL *,
    EfiBootManagerGetGopDevicePath,
    (IN  EFI_HANDLE  VideoController)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerConnectVideoController,
    (EFI_HANDLE  VideoController  OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO *,
    EfiBootManagerGetDriverHealthInfo,
    (UINTN  *Count)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerFreeDriverHealthInfo,
    (EFI_BOOT_MANAGER_DRIVER_HEALTH_INFO  *DriverHealthInfo,
     UINTN                                Count)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerProcessLoadOption,
    (EFI_BOOT_MANAGER_LOAD_OPTION  *LoadOption)
    );

  MOCK_FUNCTION_DECLARATION (
    BOOLEAN,
    EfiBootManagerIsValidLoadOptionVariableName,
    (IN CHAR16                              *VariableName,
     OUT EFI_BOOT_MANAGER_LOAD_OPTION_TYPE  *OptionType   OPTIONAL,
     OUT UINT16                             *OptionNumber OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiBootManagerDispatchDeferredImages,
    ()
    );
};
