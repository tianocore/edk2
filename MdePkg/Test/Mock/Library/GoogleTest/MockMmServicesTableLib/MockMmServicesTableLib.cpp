/** @file MockMmServicesTableLib.cpp
  Google Test mocks for MmServicesTableLib

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockMmServicesTableLib.h>

MOCK_INTERFACE_DEFINITION (MockMmServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmInstallProtocolInterface, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmUninstallProtocolInterface, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmRegisterProtocolNotify, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMmServicesTableLib, gMmst_MmLocateProtocol, 3, EFIAPI);

static EFI_MM_SYSTEM_TABLE  LocalMmst = {
  { 0, 0, 0, 0, 0 },                  // EFI_TABLE_HEADER
  NULL,                               // MmFirmwareVendor
  0,                                  // MmFirmwareRevision
  NULL,                               // EFI_MM_INSTALL_CONFIGURATION_TABLE
  { NULL },                           // EFI_MM_CPU_IO_PROTOCOL
  NULL,                               // EFI_ALLOCATE_POOL
  NULL,                               // EFI_FREE_POOL
  NULL,                               // EFI_ALLOCATE_PAGES
  NULL,                               // EFI_FREE_PAGES
  NULL,                               // EFI_MM_STARTUP_THIS_AP
  0,                                  // CurrentlyExecutingCpu
  0,                                  // NumberOfCpus
  NULL,                               // CpuSaveStateSize
  NULL,                               // CpuSaveState
  0,                                  // NumberOfTableEntries
  NULL,                               // EFI_CONFIGURATION_TABLE
  gMmst_MmInstallProtocolInterface,   // EFI_INSTALL_PROTOCOL_INTERFACE
  gMmst_MmUninstallProtocolInterface, // EFI_UNINSTALL_PROTOCOL_INTERFACE
  NULL,                               // EFI_HANDLE_PROTOCOL
  gMmst_MmRegisterProtocolNotify,     // EFI_MM_REGISTER_PROTOCOL_NOTIFY
  NULL,                               // EFI_LOCATE_HANDLE
  gMmst_MmLocateProtocol,             // EFI_LOCATE_PROTOCOL
  NULL,                               // EFI_MM_INTERRUPT_MANAGE
  NULL,                               // EFI_MM_INTERRUPT_REGISTER
  NULL                                // EFI_MM_INTERRUPT_UNREGISTER
};

extern "C" {
  EFI_MM_SYSTEM_TABLE  *gMmst = &LocalMmst;
}
