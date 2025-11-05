/** @file
  This file declares a mock of Configuration Manager Protocol.

  Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_CONFIGURATION_MANAGER_PROTOCOL_H_
#define MOCK_CONFIGURATION_MANAGER_PROTOCOL_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/ConfigurationManagerProtocol.h>
}

struct MockConfigurationManagerProtocol {
  MOCK_INTERFACE_DECLARATION (MockConfigurationManagerProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetObject,
    (IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *This,
     IN  CONST CM_OBJECT_ID                         CmObjectId,
     IN  CONST CM_OBJECT_TOKEN                      Token,
     IN  OUT   CM_OBJ_DESCRIPTOR                   *CmObject)
    );
};

extern "C" {
  extern EDKII_CONFIGURATION_MANAGER_PROTOCOL  *gConfigurationManagerProtocol;
}

#endif // MOCK_CONFIGURATION_MANAGER_PROTOCOL_H_
