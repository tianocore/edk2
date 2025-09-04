/** @file
  Google Test mocks for DevicePathLib

  Copyright (c) 2025, Yandex. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SECURITY_MANAGEMENT_LIB_H_
#define MOCK_SECURITY_MANAGEMENT_LIB_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/SecurityManagementLib.h>
}

struct MockSecurityManagementLib {
  MOCK_INTERFACE_DECLARATION (MockSecurityManagementLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RegisterSecurityHandler,
    (IN  SECURITY_FILE_AUTHENTICATION_STATE_HANDLER  SecurityHandler,
     IN  UINT32                                      AuthenticationOperation)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ExecuteSecurityHandlers,
    (IN  UINT32                          AuthenticationStatus,
     IN  CONST EFI_DEVICE_PATH_PROTOCOL  *FilePath)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    RegisterSecurity2Handler,
    (IN  SECURITY2_FILE_AUTHENTICATION_HANDLER   Security2Handler,
     IN  UINT32                                  AuthenticationOperation)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ExecuteSecurity2Handlers,
    (IN  UINT32                          AuthenticationOperation,
     IN  UINT32                          AuthenticationStatus,
     IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File  OPTIONAL,
     IN  VOID                            *FileBuffer,
     IN  UINTN                           FileSize,
     IN  BOOLEAN                         BootPolicy)
    );
};

#endif // MOCK_SECURITY_MANAGEMENT_LIB_H_
