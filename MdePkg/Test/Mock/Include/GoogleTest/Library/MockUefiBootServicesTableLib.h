/** @file
  Google Test mocks for UefiBootServicesTableLib

  Copyright (c) Microsoft Corporation.
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

//
// Declarations to handle usage of the UefiBootServiceTableLib by creating mock
//
struct MockUefiBootServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockUefiBootServicesTableLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_GetMemoryMap,
    (IN OUT UINTN                 *MemoryMapSize,
     OUT    EFI_MEMORY_DESCRIPTOR *MemoryMap,
     OUT    UINTN                 *MapKey,
     OUT    UINTN                 *DescriptorSize,
     OUT    UINT32                *DescriptorVersion)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CreateEvent,
    (IN  UINT32           Type,
     IN  EFI_TPL          NotifyTpl,
     IN  EFI_EVENT_NOTIFY NotifyFunction,
     IN  VOID             *NotifyContext,
     OUT EFI_EVENT        *Event)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CloseEvent,
    (IN EFI_EVENT Event)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_InstallProtocolInterface,
    (IN OUT EFI_HANDLE      *UserHandle,
     IN EFI_GUID            *Protocol,
     IN EFI_INTERFACE_TYPE  InterfaceType,
     IN VOID                *Interface)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_UninstallProtocolInterface,
    (IN EFI_HANDLE               Handle,
     IN EFI_GUID                 *Protocol,
     IN VOID                     *Interface)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_HandleProtocol,
    (IN  EFI_HANDLE Handle,
     IN  EFI_GUID   *Protocol,
     OUT VOID       **Interface)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateHandleBuffer,
    (
     IN     EFI_LOCATE_SEARCH_TYPE       SearchType,
     IN     EFI_GUID                     *Protocol       OPTIONAL,
     IN     VOID                         *SearchKey      OPTIONAL,
     OUT    UINTN                        *NoHandles,
     OUT    EFI_HANDLE                   **Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateProtocol,
    (IN  EFI_GUID *Protocol,
     IN  VOID      *Registration  OPTIONAL,
     OUT VOID      **Interface)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CreateEventEx,
    (IN UINT32            Type,
     IN EFI_TPL           NotifyTpl,
     IN EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,
     IN CONST VOID        *NotifyContext OPTIONAL,
     IN CONST EFI_GUID    *EventGroup OPTIONAL,
     OUT EFI_EVENT        *Event)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gBS_SetMem,
    (IN VOID     *Buffer,
     IN UINTN    Size,
     IN UINT8    Value)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gBS_CopyMem,
    (IN VOID     *Destination,
     IN VOID     *Source,
     IN UINTN    Length)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_OpenProtocol,
    (IN  EFI_HANDLE                Handle,
     IN  EFI_GUID                  *Protocol,
     OUT VOID                      **Interface  OPTIONAL,
     IN  EFI_HANDLE                AgentHandle,
     IN  EFI_HANDLE                ControllerHandle,
     IN  UINT32                    Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CloseProtocol,
    (IN EFI_HANDLE               Handle,
     IN EFI_GUID                 *Protocol,
     IN EFI_HANDLE               AgentHandle,
     IN EFI_HANDLE               ControllerHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_FreePool,
    (IN  VOID                         *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateDevicePath,
    (IN     EFI_GUID                         *Protocol,
     IN OUT EFI_DEVICE_PATH_PROTOCOL         **DevicePath,
     OUT    EFI_HANDLE                       *Device)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_ReinstallProtocolInterface,
    (IN EFI_HANDLE               Handle,
     IN EFI_GUID                 *Protocol,
     IN VOID                     *OldInterface,
     IN VOID                     *NewInterface)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_AllocatePool,
    (IN  EFI_MEMORY_TYPE              PoolType,
     IN  UINTN                        Size,
     OUT VOID                         **Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateHandle,
    (IN     EFI_LOCATE_SEARCH_TYPE   SearchType,
     IN     EFI_GUID                 *Protocol     OPTIONAL,
     IN     VOID                     *SearchKey    OPTIONAL,
     IN OUT UINTN                    *BufferSize,
     OUT    EFI_HANDLE               *Buffer)
    );
};

#endif // MOCK_UEFI_BOOT_SERVICES_TABLE_LIB_H_
