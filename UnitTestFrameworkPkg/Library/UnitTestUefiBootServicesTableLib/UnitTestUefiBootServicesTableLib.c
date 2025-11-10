/** @file
  This library supports a Boot Services table library implementation that allows code dependent
  upon UefiBootServicesTableLib to operate in an isolated execution environment such as within
  the context of a host-based unit test framework.

  The unit test should initialize the Boot Services database with any required elements
  (e.g. protocols, events, handles, etc.) prior to the services being invoked by code under test.

  It is strongly recommended to clean any global databases (e.g. protocol, event, handles, etc.) after
  every unit test so the tests execute in a predictable manner from a clean state.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UnitTestUefiBootServicesTableLib.h"

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  This function copies Length bytes from SourceBuffer to DestinationBuffer, and returns
  DestinationBuffer.  The implementation must be reentrant, and it must handle the case
  where SourceBuffer overlaps DestinationBuffer.

  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then ASSERT().
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT().

  @param  DestinationBuffer   The pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        The pointer to the source buffer of the memory copy.
  @param  Length              The number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return None.

**/
STATIC
VOID
EFIAPI
EfiCopyMem (
  IN VOID   *DestinationBuffer,
  IN VOID   *SourceBuffer,
  IN UINTN  Length
  )
{
  CopyMem (
    DestinationBuffer,
    SourceBuffer,
    Length
    );
}

/**
  Fills a target buffer with a byte value, and returns the target buffer.

  This function fills Length bytes of Buffer with Value.

  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer    The memory to set.
  @param  Length    The number of bytes to set.
  @param  Value     The value with which to fill Length bytes of Buffer.

  @return None.

**/
STATIC
VOID
EFIAPI
EfiSetMem (
  IN VOID   *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  )
{
  SetMem (
    Buffer,
    Length,
    Value
    );
}

EFI_HANDLE        gImageHandle = NULL;
EFI_SYSTEM_TABLE  *gST         = NULL;

STATIC EFI_BOOT_SERVICES  mBootServices = {
  {
    EFI_BOOT_SERVICES_SIGNATURE,                                                              // Signature
    EFI_BOOT_SERVICES_REVISION,                                                               // Revision
    sizeof (EFI_BOOT_SERVICES),                                                               // HeaderSize
    0,                                                                                        // CRC32
    0                                                                                         // Reserved
  },
  (EFI_RAISE_TPL)UnitTestRaiseTpl,                                                            // RaiseTPL
  (EFI_RESTORE_TPL)UnitTestRestoreTpl,                                                        // RestoreTPL
  (EFI_ALLOCATE_PAGES)UnitTestAllocatePages,                                                  // AllocatePages
  (EFI_FREE_PAGES)UnitTestFreePages,                                                          // FreePages
  (EFI_GET_MEMORY_MAP)UnitTestGetMemoryMap,                                                   // GetMemoryMap
  (EFI_ALLOCATE_POOL)UnitTestAllocatePool,                                                    // AllocatePool
  (EFI_FREE_POOL)UnitTestFreePool,                                                            // FreePool
  (EFI_CREATE_EVENT)UnitTestCreateEvent,                                                      // CreateEvent
  (EFI_SET_TIMER)UnitTestSetTimer,                                                            // SetTimer
  (EFI_WAIT_FOR_EVENT)UnitTestWaitForEvent,                                                   // WaitForEvent
  (EFI_SIGNAL_EVENT)UnitTestSignalEvent,                                                      // SignalEvent
  (EFI_CLOSE_EVENT)UnitTestCloseEvent,                                                        // CloseEvent
  (EFI_CHECK_EVENT)UnitTestCheckEvent,                                                        // CheckEvent
  (EFI_INSTALL_PROTOCOL_INTERFACE)UnitTestInstallProtocolInterface,                           // InstallProtocolInterface
  (EFI_REINSTALL_PROTOCOL_INTERFACE)UnitTestReinstallProtocolInterface,                       // ReinstallProtocolInterface
  (EFI_UNINSTALL_PROTOCOL_INTERFACE)UnitTestUninstallProtocolInterface,                       // UninstallProtocolInterface
  (EFI_HANDLE_PROTOCOL)UnitTestHandleProtocol,                                                // HandleProtocol
  (VOID *)NULL,                                                                               // Reserved
  (EFI_REGISTER_PROTOCOL_NOTIFY)UnitTestRegisterProtocolNotify,                               // RegisterProtocolNotify
  (EFI_LOCATE_HANDLE)UnitTestLocateHandle,                                                    // LocateHandle
  (EFI_LOCATE_DEVICE_PATH)UnitTestLocateDevicePath,                                           // LocateDevicePath
  (EFI_INSTALL_CONFIGURATION_TABLE)UnitTestInstallConfigurationTable,                         // InstallConfigurationTable
  (EFI_IMAGE_LOAD)UnitTestLoadImage,                                                          // LoadImage
  (EFI_IMAGE_START)UnitTestStartImage,                                                        // StartImage
  (EFI_EXIT)UnitTestExit,                                                                     // Exit
  (EFI_IMAGE_UNLOAD)UnitTestUnloadImage,                                                      // UnloadImage
  (EFI_EXIT_BOOT_SERVICES)UnitTestExitBootServices,                                           // ExitBootServices
  (EFI_GET_NEXT_MONOTONIC_COUNT)UnitTestGetNextMonotonicCount,                                // GetNextMonotonicCount
  (EFI_STALL)UnitTestStall,                                                                   // Stall
  (EFI_SET_WATCHDOG_TIMER)UnitTestSetWatchdogTimer,                                           // SetWatchdogTimer
  (EFI_CONNECT_CONTROLLER)UnitTestConnectController,                                          // ConnectController
  (EFI_DISCONNECT_CONTROLLER)UnitTestDisconnectController,                                    // DisconnectController
  (EFI_OPEN_PROTOCOL)UnitTestOpenProtocol,                                                    // OpenProtocol
  (EFI_CLOSE_PROTOCOL)UnitTestCloseProtocol,                                                  // CloseProtocol
  (EFI_OPEN_PROTOCOL_INFORMATION)UnitTestOpenProtocolInformation,                             // OpenProtocolInformation
  (EFI_PROTOCOLS_PER_HANDLE)UnitTestProtocolsPerHandle,                                       // ProtocolsPerHandle
  (EFI_LOCATE_HANDLE_BUFFER)UnitTestLocateHandleBuffer,                                       // LocateHandleBuffer
  (EFI_LOCATE_PROTOCOL)UnitTestLocateProtocol,                                                // LocateProtocol
  (EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)UnitTestInstallMultipleProtocolInterfaces,        // InstallMultipleProtocolInterfaces
  (EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)UnitTestUninstallMultipleProtocolInterfaces,    // UninstallMultipleProtocolInterfaces
  (EFI_CALCULATE_CRC32)UnitTestCalculateCrc32,                                                // CalculateCrc32
  (EFI_COPY_MEM)EfiCopyMem,                                                                   // CopyMem
  (EFI_SET_MEM)EfiSetMem,                                                                     // SetMem
  (EFI_CREATE_EVENT_EX)UnitTestCreateEventEx                                                  // CreateEventEx
};

EFI_BOOT_SERVICES  *gBS = &mBootServices;

/**
  The constructor function caches the pointer of Boot Services Table.

  The constructor function caches the pointer of Boot Services Table through System Table.
  It will ASSERT() if the pointer of System Table is NULL.
  It will ASSERT() if the pointer of Boot Services Table is NULL.
  It will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UnitTestUefiBootServicesTableLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // Cache the Image Handle
  //
  gImageHandle = ImageHandle;
  ASSERT (gImageHandle != NULL);

  //
  // Cache pointer to the EFI System Table
  //

  // Note: The system table is not implemented
  gST = NULL;

  //
  // Cache pointer to the EFI Boot Services Table
  //
  gBS = SystemTable->BootServices;
  ASSERT (gBS != NULL);

  return EFI_SUCCESS;
}
