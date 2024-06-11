/** @file MockMmServicesTableLib.h
  Google Test mocks for MmServicesTableLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_MM_SERVICES_TABLE_LIB_H_
#define MOCK_MM_SERVICES_TABLE_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/MmServicesTableLib.h>
}

//
// Declarations to handle usage of the MmServicesTableLib by creating mock
//
struct MockMmServicesTableLib {
  MOCK_INTERFACE_DECLARATION (MockMmServicesTableLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmInstallProtocolInterface,
    (
     IN OUT EFI_HANDLE               *Handle,
     IN     EFI_GUID                 *Protocol,
     IN     EFI_INTERFACE_TYPE       InterfaceType,
     IN     VOID                     *Interface
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmUninstallProtocolInterface,
    (
     IN EFI_HANDLE               Handle,
     IN EFI_GUID                 *Protocol,
     IN VOID                     *Interface
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmRegisterProtocolNotify,
    (
     IN  CONST EFI_GUID     *Protocol,
     IN  EFI_MM_NOTIFY_FN   Function,
     OUT VOID               **Registration
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    gMmst_MmLocateProtocol,
    (
     IN  EFI_GUID  *Protocol,
     IN  VOID      *Registration  OPTIONAL,
     OUT VOID      **Interface
    )
    );
};

#endif
