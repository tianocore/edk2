/** @file MockUefiBootServicesTableLib.cpp
  Google Test mocks for UefiBootServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>
#include <GoogleTest/Protocol/MockSimpleTextOut.h>
#include <GoogleTest/Protocol/MockSimpleTextIn.h>

MOCK_INTERFACE_DEFINITION (MockUefiBootServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_RaiseTpl, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_RestoreTpl, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_AllocatePages, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_FreePages, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_GetMemoryMap, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_AllocatePool, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_FreePool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CreateEvent, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_SetTimer, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_WaitForEvent, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_SignalEvent, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CloseEvent, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CheckEvent, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_InstallProtocolInterface, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_ReinstallProtocolInterface, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_UninstallProtocolInterface, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_HandleProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_RegisterProtocolNotify, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateHandle, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateDevicePath, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_InstallConfigurationTable, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LoadImage, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_StartImage, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_Exit, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_UnloadImage, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_ExitBootServices, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_GetNextMonotonicCount, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_Stall, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_SetWatchdogTimer, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_ConnectController, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_DisconnectController, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_OpenProtocol, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CloseProtocol, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_OpenProtocolInformation, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_ProtocolsPerHandle, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateHandleBuffer, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CalculateCrc32, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CopyMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_SetMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CreateEventEx, 6, EFIAPI);

//
// Gmock does not support variadic functions, so we implement the following functions that process
// multiple protocols in a single call.
// Usage of these functions in a test will require EXPECT_CALLs to be used for each time the
// a function processes a single protocol.
//
extern "C" {
  EFI_STATUS
  EFIAPI
  gBS_InstallMultipleProtocolInterfaces (
    IN OUT EFI_HANDLE  *Handle,
    ...
    )
  {
    VA_LIST     Args;
    EFI_STATUS  Status;
    EFI_GUID    *Protocol;
    VOID        *Interface;
    UINTN       Index;

    VA_START (Args, Handle);
    for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
      //
      // If protocol is NULL, then it's the end of the list
      //
      Protocol = VA_ARG (Args, EFI_GUID *);
      if (Protocol == NULL) {
        break;
      }

      Interface = VA_ARG (Args, VOID *);

      //
      // Install it
      //
      Status = gBS_InstallProtocolInterface (Handle, Protocol, EFI_NATIVE_INTERFACE, Interface);
    }

    VA_END (Args);
    return Status;
  }

  EFI_STATUS
  EFIAPI
  gBS_UninstallMultipleProtocolInterfaces (
    IN EFI_HANDLE  *Handle,
    ...
    )
  {
    VA_LIST     Args;
    EFI_STATUS  Status;
    EFI_GUID    *Protocol;
    VOID        *Interface;
    UINTN       Index;

    VA_START (Args, Handle);
    for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
      //
      // If protocol is NULL, then it's the end of the list
      //
      Protocol = VA_ARG (Args, EFI_GUID *);
      if (Protocol == NULL) {
        break;
      }

      Interface = VA_ARG (Args, VOID *);

      //
      // Uninstall it
      //
      Status = gBS_UninstallProtocolInterface (Handle, Protocol, Interface);
    }

    VA_END (Args);
    return Status;
  }
}

static EFI_BOOT_SERVICES  MockEfiBootServicesInstance = {
  { 0, 0, 0, 0, 0 },  // EFI_TABLE_HEADER
  (EFI_RAISE_TPL)gBS_RaiseTpl,
  (EFI_RESTORE_TPL)gBS_RestoreTpl,
  (EFI_ALLOCATE_PAGES)gBS_AllocatePages,
  (EFI_FREE_PAGES)gBS_FreePages,
  (EFI_GET_MEMORY_MAP)gBS_GetMemoryMap,
  (EFI_ALLOCATE_POOL)gBS_AllocatePool,
  (EFI_FREE_POOL)gBS_FreePool,
  (EFI_CREATE_EVENT)gBS_CreateEvent,
  (EFI_SET_TIMER)gBS_SetTimer,
  (EFI_WAIT_FOR_EVENT)gBS_WaitForEvent,
  (EFI_SIGNAL_EVENT)gBS_SignalEvent,
  (EFI_CLOSE_EVENT)gBS_CloseEvent,
  (EFI_CHECK_EVENT)gBS_CheckEvent,
  (EFI_INSTALL_PROTOCOL_INTERFACE)gBS_InstallProtocolInterface,
  (EFI_REINSTALL_PROTOCOL_INTERFACE)gBS_ReinstallProtocolInterface,
  (EFI_UNINSTALL_PROTOCOL_INTERFACE)gBS_UninstallProtocolInterface,
  (EFI_HANDLE_PROTOCOL)gBS_HandleProtocol,
  NULL, // VOID* Reserved
  (EFI_REGISTER_PROTOCOL_NOTIFY)gBS_RegisterProtocolNotify,
  (EFI_LOCATE_HANDLE)gBS_LocateHandle,
  (EFI_LOCATE_DEVICE_PATH)gBS_LocateDevicePath,
  (EFI_INSTALL_CONFIGURATION_TABLE)gBS_InstallConfigurationTable,
  (EFI_IMAGE_LOAD)gBS_LoadImage,
  (EFI_IMAGE_START)gBS_StartImage,
  (EFI_EXIT)gBS_Exit,
  (EFI_IMAGE_UNLOAD)gBS_UnloadImage,
  (EFI_EXIT_BOOT_SERVICES)gBS_ExitBootServices,
  (EFI_GET_NEXT_MONOTONIC_COUNT)gBS_GetNextMonotonicCount,
  (EFI_STALL)gBS_Stall,
  (EFI_SET_WATCHDOG_TIMER)gBS_SetWatchdogTimer,
  (EFI_CONNECT_CONTROLLER)gBS_ConnectController,
  (EFI_DISCONNECT_CONTROLLER)gBS_DisconnectController,
  (EFI_OPEN_PROTOCOL)gBS_OpenProtocol,
  (EFI_CLOSE_PROTOCOL)gBS_CloseProtocol,
  (EFI_OPEN_PROTOCOL_INFORMATION)gBS_OpenProtocolInformation,
  (EFI_PROTOCOLS_PER_HANDLE)gBS_ProtocolsPerHandle,
  (EFI_LOCATE_HANDLE_BUFFER)gBS_LocateHandleBuffer,
  (EFI_LOCATE_PROTOCOL)gBS_LocateProtocol,
  (EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)gBS_InstallMultipleProtocolInterfaces,
  (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)gBS_UninstallMultipleProtocolInterfaces,
  (EFI_CALCULATE_CRC32)gBS_CalculateCrc32,
  (EFI_COPY_MEM)gBS_CopyMem,
  (EFI_SET_MEM)gBS_SetMem,
  (EFI_CREATE_EVENT_EX)gBS_CreateEventEx
};

MOCK_EFI_SIMPLE_TEXT_INPUT_PROTOCOL_INSTANCE (ConInMock)
MOCK_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_INSTANCE (ConOutMock)
MOCK_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_INSTANCE (StdErrMock)

EFI_SYSTEM_TABLE MockEfiSystemTableInstance = {
  { 0, 0, 0, 0, 0 },          // EFI_TABLE_HEADER Hdr
  NULL,                       // CHAR8* FirmwareVendor
  0,                          // UINT32 FirmwareRevision
  NULL,                       // EFI_HANDLE ConsoleInHandle
  ConInMock,
  NULL,                       // EFI_HANDLE ConsoleOutHandle
  ConOutMock,
  NULL,                       // EFI_HANDLE StdErrHandle
  StdErrMock,
  NULL,                       // EFI_RUNTIME_SERVICES*
  &MockEfiBootServicesInstance,
  0,                          // UINTN NumberOfTableEntries
  NULL                        // EFI_CONFIGURATION_TABLE *ConfigurationTable
};

extern "C" {
  EFI_BOOT_SERVICES  *gBS         = &MockEfiBootServicesInstance;
  EFI_HANDLE         gImageHandle = NULL;
  EFI_SYSTEM_TABLE   *gST         = &MockEfiSystemTableInstance;
}
