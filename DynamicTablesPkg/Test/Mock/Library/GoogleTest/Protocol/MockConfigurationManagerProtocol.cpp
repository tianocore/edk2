/** @file
  Google Test mock for Configuration Manager Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockConfigurationManagerProtocol.h>

MOCK_INTERFACE_DEFINITION (MockConfigurationManagerProtocol);
MOCK_FUNCTION_DEFINITION (MockConfigurationManagerProtocol, GetObject, 4, EFIAPI);

EDKII_CONFIGURATION_MANAGER_PROTOCOL  CONFIG_MGR_PROTOCOL_INSTANCE = {
  CREATE_REVISION (1, 0), // Revision
  GetObject,              // GetObject
  NULL,                   // SetObject
  NULL                    // PlatRepoInfo
};

extern "C" {
  EDKII_CONFIGURATION_MANAGER_PROTOCOL  *gConfigurationManagerProtocol = &CONFIG_MGR_PROTOCOL_INSTANCE;
}
