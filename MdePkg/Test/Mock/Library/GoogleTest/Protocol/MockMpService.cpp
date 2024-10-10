/** @file MockMpService.cpp
  Google Test mock for MP service Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockMpService.h>

MOCK_INTERFACE_DEFINITION (MockMpService);
MOCK_FUNCTION_DEFINITION (MockMpService, GetNumberOfProcessors, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMpService, GetProcessorInfo, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMpService, StartupAllAPs, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMpService, StartupThisAP, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMpService, SwitchBSP, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMpService, EnableDisableAP, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockMpService, WhoAmI, 2, EFIAPI);

EFI_MP_SERVICES_PROTOCOL  MP_SERVICE_PROTOCOL_INSTANCE = {
  GetNumberOfProcessors,
  GetProcessorInfo,
  StartupAllAPs,
  StartupThisAP,
  SwitchBSP,
  EnableDisableAP,
  WhoAmI
};

extern "C" {
  EFI_MP_SERVICES_PROTOCOL  *gMpServiceProtocol = &MP_SERVICE_PROTOCOL_INSTANCE;
}
