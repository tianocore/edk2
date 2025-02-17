/** @file MockMmServicesTableLib.cpp
  Google Test mocks for MmServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockMmServicesTableLib.h>

MOCK_INTERFACE_DEFINITION (MockMmServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmAllocatePool, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmFreePool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmAllocatePages, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmFreePages, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmStartupThisAp, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmInstallProtocolInterface, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmUninstallProtocolInterface, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmHandleProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmRegisterProtocolNotify, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmLocateHandle, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmLocateProtocol, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmiManage, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmInterruptRegister, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmInterruptUnRegister, 1, EFIAPI);

static EFI_MM_SYSTEM_TABLE  LocalMmst = {
  { 0, 0, 0, 0, 0 },                  // EFI_TABLE_HEADER
  NULL,                               // MmFirmwareVendor
  0,                                  // MmFirmwareRevision
  NULL,                               // EFI_MM_INSTALL_CONFIGURATION_TABLE
  { NULL },                           // EFI_MM_CPU_IO_PROTOCOL
  gMmst_MmAllocatePool,               // EFI_ALLOCATE_POOL
  gMmst_MmFreePool,                   // EFI_FREE_POOL
  gMmst_MmAllocatePages,              // EFI_ALLOCATE_PAGES
  gMmst_MmFreePages,                  // EFI_FREE_PAGES
  gMmst_MmStartupThisAp,              // EFI_MM_STARTUP_THIS_AP
  0,                                  // CurrentlyExecutingCpu
  0,                                  // NumberOfCpus
  NULL,                               // CpuSaveStateSize
  NULL,                               // CpuSaveState
  0,                                  // NumberOfTableEntries
  NULL,                               // EFI_CONFIGURATION_TABLE
  gMmst_MmInstallProtocolInterface,   // EFI_INSTALL_PROTOCOL_INTERFACE
  gMmst_MmUninstallProtocolInterface, // EFI_UNINSTALL_PROTOCOL_INTERFACE
  gMmst_MmHandleProtocol,             // EFI_HANDLE_PROTOCOL
  gMmst_MmRegisterProtocolNotify,     // EFI_MM_REGISTER_PROTOCOL_NOTIFY
  gMmst_MmLocateHandle,               // EFI_LOCATE_HANDLE
  gMmst_MmLocateProtocol,             // EFI_LOCATE_PROTOCOL
  gMmst_MmiManage,                    // EFI_MM_INTERRUPT_MANAGE
  gMmst_MmInterruptRegister,          // EFI_MM_INTERRUPT_REGISTER
  gMmst_MmInterruptUnRegister         // EFI_MM_INTERRUPT_UNREGISTER
};

extern "C" {
  EFI_MM_SYSTEM_TABLE  *gMmst = &LocalMmst;
}
