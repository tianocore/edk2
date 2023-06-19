/** @file
  Google Test mocks for UefiBootServicesTableLib

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>

MOCK_INTERFACE_DEFINITION(MockUefiBootServicesTableLib);

MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_LocateProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_LocateHandleBuffer, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_DisconnectController, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_FreePool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_ConnectController, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_HandleProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_Stall, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION(MockUefiBootServicesTableLib, gBS_CopyMem, 3, EFIAPI);

static EFI_BOOT_SERVICES localBs = {
  {
    (UINT64)NULL,                                         // Signature
    0,                                                    // Revision
    0,                                                    // HeaderSize
    0,                                                    // CRC32
    0                                                     // Reserved
  },
  NULL,                                                   // RaiseTPL
  NULL,                                                   // RestoreTPL
  NULL,                                                   // AllocatePages
  NULL,                                                   // FreePages
  NULL,                                                   // GetMemoryMap
  NULL,                                                   // AllocatePool
  (EFI_FREE_POOL)gBS_FreePool,                            // FreePool
  NULL,                                                   // CreateEvent
  NULL,                                                   // SetTimer
  NULL,                                                   // WaitForEvent
  NULL,                                                   // SignalEvent
  NULL,                                                   // CloseEvent
  NULL,                                                   // CheckEvent
  NULL,                                                   // InstallProtocolInterface
  NULL,                                                   // ReinstallProtocolInterface
  NULL,                                                   // UninstallProtocolInterface
  (EFI_HANDLE_PROTOCOL)gBS_HandleProtocol,                // HandleProtocol
  (VOID *)NULL,                                           // Reserved
  NULL,                                                   // RegisterProtocolNotify
  NULL,                                                   // LocateHandle
  NULL,                                                   // LocateDevicePath
  NULL,                                                   // InstallConfigurationTable
  NULL,                                                   // LoadImage
  NULL,                                                   // StartImage
  NULL,                                                   // Exit
  NULL,                                                   // UnloadImage
  NULL,                                                   // ExitBootServices
  NULL,                                                   // GetNextMonotonicCount
  (EFI_STALL)gBS_Stall,                                   // Stall
  NULL,                                                   // SetWatchdogTimer
  (EFI_CONNECT_CONTROLLER)gBS_ConnectController,          // ConnectController
  (EFI_DISCONNECT_CONTROLLER)gBS_DisconnectController,    // DisconnectController
  NULL,                                                   // OpenProtocol
  NULL,                                                   // CloseProtocol
  NULL,                                                   // OpenProtocolInformation
  NULL,                                                   // ProtocolsPerHandle
  (EFI_LOCATE_HANDLE_BUFFER)gBS_LocateHandleBuffer,       // LocateHandleBuffer
  (EFI_LOCATE_PROTOCOL)gBS_LocateProtocol,                // LocateProtocol
  NULL,                                                   // InstallMultipleProtocolInterfaces
  NULL,                                                   // UninstallMultipleProtocolInterfaces
  NULL,                                                   // CalculateCrc32
  (EFI_COPY_MEM)gBS_CopyMem,                              // CopyMem
  NULL,                                                   // SetMem
  NULL                                                    // CreateEventEx
};

extern "C" {
  EFI_BOOT_SERVICES* gBS = &localBs;
}
