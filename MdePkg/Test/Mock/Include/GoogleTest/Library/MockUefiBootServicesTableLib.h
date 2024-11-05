/** @file MockUefiBootServicesTableLib.h
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

  //
  // Task Priority Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_TPL,
    gBS_RaiseTpl,
    (IN EFI_TPL NewTpl)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gBS_RestoreTpl,
    (IN EFI_TPL OldTpl)
    );

  //
  // Memory Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_AllocatePages,
    (IN     EFI_ALLOCATE_TYPE      Type,
     IN     EFI_MEMORY_TYPE        MemoryType,
     IN     UINTN                  Pages,
     IN OUT EFI_PHYSICAL_ADDRESS    *Memory)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_FreePages,
    (IN EFI_PHYSICAL_ADDRESS  Memory,
     IN UINTN                Pages)
    );

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
    gBS_AllocatePool,
    (IN  EFI_MEMORY_TYPE              PoolType,
     IN  UINTN                        Size,
     OUT VOID                         **Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_FreePool,
    (IN  VOID                         *Buffer)
    );

  //
  // Event & Timer Services
  //
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
    gBS_SetTimer,
    (IN EFI_EVENT  Event,
     IN EFI_TIMER_DELAY  Type,
     IN UINT64  TriggerTime)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_WaitForEvent,
    (IN  UINTN       NumberOfEvents,
     IN  EFI_EVENT   *Event,
     OUT UINTN       *Index)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_SignalEvent,
    (IN EFI_EVENT  Event)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CloseEvent,
    (IN EFI_EVENT Event)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CheckEvent,
    (IN EFI_EVENT  Event)
    );

  //
  // Protocol Handler Services
  //
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
    gBS_ReinstallProtocolInterface,
    (IN EFI_HANDLE  Handle,
     IN EFI_GUID    *Protocol,
     IN VOID        *OldInterface,
     IN VOID        *NewInterface)
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
    gBS_RegisterProtocolNotify,
    (IN  EFI_GUID                 *Protocol,
     IN  EFI_EVENT                Event,
     OUT VOID                     **Registration)
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

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LocateDevicePath,
    (IN     EFI_GUID                         *Protocol,
     IN OUT EFI_DEVICE_PATH_PROTOCOL         **DevicePath,
     OUT    EFI_HANDLE                       *Device)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_InstallConfigurationTable,
    (IN EFI_GUID  *Guid,
     IN VOID      *Table)
    );

  //
  // Image Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_LoadImage,
    (IN  BOOLEAN                    BootPolicy,
     IN  EFI_HANDLE                 ParentImageHandle,
     IN  EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
     IN  VOID                       *SourceBuffer  OPTIONAL,
     IN  UINTN                      SourceSize,
     OUT EFI_HANDLE                 *ImageHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_StartImage,
    (IN  EFI_HANDLE                 ImageHandle,
     OUT UINTN                      *ExitDataSize,
     OUT CHAR16                     **ExitData  OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_Exit,
    (IN EFI_HANDLE  ImageHandle,
     IN EFI_STATUS  ExitStatus,
     IN UINTN       ExitDataSize,
     IN CHAR16      *ExitData  OPTIONAL)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_UnloadImage,
    (IN EFI_HANDLE  ImageHandle)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_ExitBootServices,
    (IN EFI_HANDLE  ImageHandle,
     IN UINTN       MapKey)
    );

  //
  // Miscellaneous Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_GetNextMonotonicCount,
    (OUT UINT64  *Count)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_Stall,
    (IN UINTN  Microseconds)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_SetWatchdogTimer,
    (IN UINTN  Timeout,
     IN UINT64 WatchdogCode,
     IN UINTN DataSize,
     IN CHAR16 *WatchdogData OPTIONAL)
    );

  //
  // Driver Support Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_ConnectController,
    (IN  EFI_HANDLE                    ControllerHandle,
     IN  EFI_HANDLE                    *DriverImageHandle    OPTIONAL,
     IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath  OPTIONAL,
     IN  BOOLEAN                       Recursive)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_DisconnectController,
    (IN  EFI_HANDLE                     ControllerHandle,
     IN  EFI_HANDLE                     DriverImageHandle  OPTIONAL,
     IN  EFI_HANDLE                     ChildHandle        OPTIONAL)
    );

  //
  // Open and Close Protocol Services
  //
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
    gBS_OpenProtocolInformation,
    (IN  EFI_HANDLE                            Handle,
     IN  EFI_GUID                              *Protocol,
     OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY   **EntryBuffer,
     OUT UINTN                                *EntryCount)
    );

  //
  // Library Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_ProtocolsPerHandle,
    (IN  EFI_HANDLE      Handle,
     OUT EFI_GUID        ***ProtocolBuffer,
     OUT UINTN           *ProtocolBufferCount)
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

  //
  // 32-bit CRC Services
  //
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gBS_CalculateCrc32,
    (IN  VOID      *Data,
     IN  UINTN     DataSize,
     OUT UINT32    *Crc32)
    );

  //
  // Miscellaneous Services
  //
  MOCK_FUNCTION_DECLARATION (
    VOID,
    gBS_CopyMem,
    (IN VOID     *Destination,
     IN VOID     *Source,
     IN UINTN    Length)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    gBS_SetMem,
    (IN VOID     *Buffer,
     IN UINTN    Size,
     IN UINT8    Value)
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
};

#endif // MOCK_UEFI_BOOT_SERVICES_TABLE_LIB_H_
