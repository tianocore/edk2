/** @file MockSmmServicesTableLib.cpp
  Google Test mocks for SmmServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <GoogleTest/Library/MockSmmServicesTableLib.h>

MOCK_INTERFACE_DEFINITION (MockSmmServicesTableLib);
MOCK_FUNCTION_DEFINITION (MockSmmServicesTableLib, gSmst_SmmAllocatePool, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmServicesTableLib, gSmst_SmmFreePool, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmServicesTableLib, gSmst_SmmStartupThisAp, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmServicesTableLib, gSmst_SmmInstallProtocolInterface, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmServicesTableLib, gSmst_SmmRegisterProtocolNotify, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmmServicesTableLib, gSmst_SmmLocateProtocol, 3, EFIAPI);

static EFI_SMM_SYSTEM_TABLE2  LocalSmst = {
  { 0, 0, 0, 0, 0 },                 // EFI_TABLE_HEADER
  NULL,                              // SmmFirmwareVendor
  0,                                 // SmmFirmwareRevision
  NULL,                              // EFI_SMM_INSTALL_CONFIGURATION_TABLE2
  { NULL },                          // EFI_SMM_CPU_IO2_PROTOCOL
  gSmst_SmmAllocatePool,             // EFI_ALLOCATE_POOL
  gSmst_SmmFreePool,                 // EFI_FREE_POOL
  NULL,                              // EFI_ALLOCATE_PAGES
  NULL,                              // EFI_FREE_PAGES
  gSmst_SmmStartupThisAp,            // EFI_SMM_STARTUP_THIS_AP
  0,                                 // CurrentlyExecutingCpu
  0,                                 // NumberOfCpus
  NULL,                              // CpuSaveStateSize
  NULL,                              // CpuSaveState
  0,                                 // NumberOfTableEntries
  NULL,                              // EFI_CONFIGURATION_TABLE
  gSmst_SmmInstallProtocolInterface, // EFI_INSTALL_PROTOCOL_INTERFACE
  NULL,                              // EFI_UNINSTALL_PROTOCOL_INTERFACE
  NULL,                              // EFI_HANDLE_PROTOCOL
  gSmst_SmmRegisterProtocolNotify,   // EFI_SMM_REGISTER_PROTOCOL_NOTIFY
  NULL,                              // EFI_LOCATE_HANDLE
  gSmst_SmmLocateProtocol,           // EFI_LOCATE_PROTOCOL
  NULL,                              // EFI_SMM_INTERRUPT_MANAGE
  NULL,                              // EFI_SMM_INTERRUPT_REGISTER
  NULL                               // SmiHandlerUnRegister
};

extern "C" {
  EFI_SMM_SYSTEM_TABLE2  *gSmst = &LocalSmst;
}
