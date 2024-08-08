/** @file
  Google Test mocks for UefiLib

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_UEFI_LIB_H_
#define MOCK_UEFI_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/UefiLib.h>
}

struct MockUefiLib {
  MOCK_INTERFACE_DECLARATION (MockUefiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVariable2,
    (IN CONST CHAR16    *Name,
     IN CONST EFI_GUID  *Guid,
     OUT VOID           **Value,
     OUT UINTN          *Size OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetEfiGlobalVariable2,
    (IN CONST CHAR16  *Name,
     OUT VOID         **Value,
     OUT UINTN        *Size OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_EVENT,
    EfiCreateProtocolNotifyEvent,
    (IN EFI_GUID          *ProtocolGuid,
     IN EFI_TPL           NotifyTpl,
     IN EFI_EVENT_NOTIFY  NotifyFunction,
     IN VOID              *NotifyContext OPTIONAL,
     OUT VOID             **Registration)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiTestManagedDevice,
    (IN CONST EFI_HANDLE ControllerHandle,
     IN CONST EFI_HANDLE DriverBindingHandle,
     IN CONST EFI_GUID    *ProtocolGuid)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    LookupUnicodeString2,
    (IN CONST CHAR8                     *Language,
     IN CONST CHAR8                     *SupportedLanguages,
     IN CONST EFI_UNICODE_STRING_TABLE  *UnicodeStringTable,
     OUT CHAR16                         **UnicodeString,
     IN BOOLEAN Iso639Language)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    AddUnicodeString2,
    (IN CONST CHAR8               *Language,
     IN CONST CHAR8               *SupportedLanguages,
     IN OUT EFI_UNICODE_STRING_TABLE  **UnicodeStringTable,
     IN CONST CHAR16              *UnicodeString,
     IN BOOLEAN Iso639Language)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FreeUnicodeStringTable,
    (IN EFI_UNICODE_STRING_TABLE  *UnicodeStringTable)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EfiLibInstallDriverBindingComponentName2,
    (IN CONST EFI_HANDLE                    ImageHandle,
     IN CONST EFI_SYSTEM_TABLE              *SystemTable,
     IN EFI_DRIVER_BINDING_PROTOCOL         *DriverBinding,
     IN EFI_HANDLE                          DriverBindingHandle,
     IN CONST EFI_COMPONENT_NAME_PROTOCOL   *ComponentName        OPTIONAL,
     IN CONST EFI_COMPONENT_NAME2_PROTOCOL  *ComponentName2       OPTIONAL)
    );
};

#endif
