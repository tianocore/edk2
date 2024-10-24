/** @file
  Google Test mocks for UefiBootServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>

MOCK_INTERFACE_DEFINITION (MockUefiBootServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_GetMemoryMap, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CreateEvent, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CloseEvent, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_InstallProtocolInterface, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_UninstallProtocolInterface, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_HandleProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_RegisterProtocolNotify, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateHandleBuffer, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CreateEventEx, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_SetMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CopyMem, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_OpenProtocol, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_CloseProtocol, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_FreePool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateDevicePath, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_ReinstallProtocolInterface, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_AllocatePool, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_LocateHandle, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_ConnectController, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockUefiBootServicesTableLib, gBS_DisconnectController, 3, EFIAPI);

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

static EFI_BOOT_SERVICES  LocalBs = {
  { 0, 0, 0, 0, 0 },                                                                   // EFI_TABLE_HEADER
  NULL,                                                                                // EFI_RAISE_TPL
  NULL,                                                                                // EFI_RESTORE_TPL
  NULL,                                                                                // EFI_ALLOCATE_PAGES
  NULL,                                                                                // EFI_FREE_PAGES
  gBS_GetMemoryMap,                                                                    // EFI_GET_MEMORY_MAP
  gBS_AllocatePool,                                                                    // EFI_ALLOCATE_POOL
  gBS_FreePool,                                                                        // EFI_FREE_POOL
  gBS_CreateEvent,                                                                     // EFI_CREATE_EVENT
  NULL,                                                                                // EFI_SET_TIMER
  NULL,                                                                                // EFI_WAIT_FOR_EVENT
  NULL,                                                                                // EFI_SIGNAL_EVENT
  gBS_CloseEvent,                                                                      // EFI_CLOSE_EVENT
  NULL,                                                                                // EFI_CHECK_EVENT
  gBS_InstallProtocolInterface,                                                        // EFI_INSTALL_PROTOCOL_INTERFACE
  gBS_ReinstallProtocolInterface,                                                      // EFI_REINSTALL_PROTOCOL_INTERFACE
  gBS_UninstallProtocolInterface,                                                      // EFI_UNINSTALL_PROTOCOL_INTERFACE
  gBS_HandleProtocol,                                                                  // EFI_HANDLE_PROTOCOL
  NULL,                                                                                // VOID
  gBS_RegisterProtocolNotify,                                                          // EFI_REGISTER_PROTOCOL_NOTIFY
  gBS_LocateHandle,                                                                    // EFI_LOCATE_HANDLE
  gBS_LocateDevicePath,                                                                // EFI_LOCATE_DEVICE_PATH
  NULL,                                                                                // EFI_INSTALL_CONFIGURATION_TABLE
  NULL,                                                                                // EFI_IMAGE_LOAD
  NULL,                                                                                // EFI_IMAGE_START
  NULL,                                                                                // EFI_EXIT
  NULL,                                                                                // EFI_IMAGE_UNLOAD
  NULL,                                                                                // EFI_EXIT_BOOT_SERVICES
  NULL,                                                                                // EFI_GET_NEXT_MONOTONIC_COUNT
  NULL,                                                                                // EFI_STALL
  NULL,                                                                                // EFI_SET_WATCHDOG_TIMER
  gBS_ConnectController,                                                               // EFI_CONNECT_CONTROLLER
  gBS_DisconnectController,                                                            // EFI_DISCONNECT_CONTROLLER
  gBS_OpenProtocol,                                                                    // EFI_OPEN_PROTOCOL
  gBS_CloseProtocol,                                                                   // EFI_CLOSE_PROTOCOL
  NULL,                                                                                // EFI_OPEN_PROTOCOL_INFORMATION
  NULL,                                                                                // EFI_PROTOCOLS_PER_HANDLE
  gBS_LocateHandleBuffer,                                                              // EFI_LOCATE_HANDLE_BUFFER
  gBS_LocateProtocol,                                                                  // EFI_LOCATE_PROTOCOL
  gBS_InstallMultipleProtocolInterfaces,                                               // EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES
  (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)gBS_UninstallMultipleProtocolInterfaces, // EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES
  NULL,                                                                                // EFI_CALCULATE_CRC32
  gBS_CopyMem,                                                                         // EFI_COPY_MEM
  gBS_SetMem,                                                                          // EFI_SET_MEM
  gBS_CreateEventEx                                                                    // EFI_CREATE_EVENT_EX
};

extern "C" {
  EFI_BOOT_SERVICES  *gBS         = &LocalBs;
  EFI_HANDLE         gImageHandle = NULL;
  EFI_SYSTEM_TABLE   *gST         = NULL;
}
