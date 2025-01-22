/** @file MockHiiDatabase.h
  This file declares a mock of HiiDatabase Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_HII_DATABASE_H
#define MOCK_HII_DATABASE_H

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/HiiDatabase.h>
}

struct MockHiiDatabaseProtocol {
  MOCK_INTERFACE_DECLARATION (MockHiiDatabaseProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    NewPackageList,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL   *This,
     IN CONST  EFI_HII_PACKAGE_LIST_HEADER *PackageList,
     IN        EFI_HANDLE                  DriverHandle  OPTIONAL,
     OUT       EFI_HII_HANDLE               *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RemovePackageList,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
     IN        EFI_HII_HANDLE             Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    UpdatePackageList,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL   *This,
     IN        EFI_HII_HANDLE               Handle,
     IN CONST  EFI_HII_PACKAGE_LIST_HEADER *PackageList
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ListPackageLists,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
     IN        UINT8                     PackageType,
     IN CONST  EFI_GUID                  *PackageGuid,
     IN OUT    UINTN                     *HandleBufferLength,
     OUT       EFI_HII_HANDLE            *Handle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ExportPackageLists,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL      *This,
     IN        EFI_HII_HANDLE                 Handle,
     IN OUT    UINTN                          *BufferSize,
     OUT       EFI_HII_PACKAGE_LIST_HEADER    *Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RegisterPackageNotify,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL     *This,
     IN        UINT8                         PackageType,
     IN CONST  EFI_GUID                      *PackageGuid,
     IN        EFI_HII_DATABASE_NOTIFY       PackageNotifyFn,
     IN        EFI_HII_DATABASE_NOTIFY_TYPE  NotifyType,
     OUT       EFI_HANDLE                    *NotifyHandle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    UnregisterPackageNotify,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
     IN        EFI_HANDLE                NotificationHandle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FindKeyboardLayouts,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
     IN OUT    UINT16                    *KeyGuidBufferLength,
     OUT       EFI_GUID                  *KeyGuidBuffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetKeyboardLayout,
    (IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
     IN CONST  EFI_GUID                  *KeyGuid,
     IN OUT UINT16                       *KeyboardLayoutLength,
     OUT       EFI_HII_KEYBOARD_LAYOUT   *KeyboardLayout)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetKeyboardLayout,
    (IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
     IN CONST  EFI_GUID                  *KeyGuid)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetPackageListHandle,
    (
     IN CONST  EFI_HII_DATABASE_PROTOCOL *This,
     IN        EFI_HII_HANDLE             PackageListHandle,
     OUT       EFI_HANDLE                *DriverHandle
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockHiiDatabaseProtocol);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, NewPackageList, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, RemovePackageList, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, UpdatePackageList, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, ListPackageLists, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, ExportPackageLists, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, RegisterPackageNotify, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, UnregisterPackageNotify, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, FindKeyboardLayouts, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, GetKeyboardLayout, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, SetKeyboardLayout, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockHiiDatabaseProtocol, GetPackageListHandle, 3, EFIAPI);

#define MOCK_EFI_HII_DATABASE_PROTOCOL_INSTANCE(NAME) \
  EFI_HII_DATABASE_PROTOCOL NAME##_INSTANCE = {       \
    NewPackageList,                                   \
    RemovePackageList,                                \
    UpdatePackageList,                                \
    ListPackageLists,                                 \
    ExportPackageLists,                               \
    RegisterPackageNotify,                            \
    UnregisterPackageNotify,                          \
    FindKeyboardLayouts,                              \
    GetKeyboardLayout,                                \
    SetKeyboardLayout,                                \
    GetPackageListHandle };                           \
  EFI_HII_DATABASE_PROTOCOL  *NAME =  &NAME##_INSTANCE;

#endif
