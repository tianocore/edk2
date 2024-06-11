/** @file MockSmmVarCheck.cpp
  Google Test mock for Smm Variable Check Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSmmVarCheck.h>

MOCK_INTERFACE_DEFINITION (MockSmmVarCheck);
MOCK_FUNCTION_DEFINITION (MockSmmVarCheck, SmmRegisterSetVariableCheckHandler, 1, EFIAPI);

EDKII_SMM_VAR_CHECK_PROTOCOL  SMMVARCHECK_PROTOCOL_INSTANCE = {
  SmmRegisterSetVariableCheckHandler, // EFI_MM_READ_SAVE_STATE
  NULL,                               // EDKII_VAR_CHECK_VARIABLE_PROPERTY_SET
  NULL                                // EDKII_VAR_CHECK_VARIABLE_PROPERTY_GET
};

extern "C" {
  EDKII_SMM_VAR_CHECK_PROTOCOL  *gEdkiiSmmVarCheckProtocol = &SMMVARCHECK_PROTOCOL_INSTANCE;
}
