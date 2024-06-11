/** @file MockNvmExpressPassthru.cpp
  Google Test mock for NvmExpress Pass Thru Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockNvmExpressPassthru.h>

MOCK_INTERFACE_DEFINITION (MockNvmePassThruProtocol);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, PassThru, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, GetNextNamespace, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, BuildDevicePath, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, GetNamespace, 3, EFIAPI);

EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  NVME_PASSTHRU_PROTOCOL_INSTANCE = {
  { 0 },              // EFI_NVM_EXPRESS_PASS_THRU_MODE                  *Mode;
  PassThru,           // EFI_NVM_EXPRESS_PASS_THRU_PASSTHRU              PassThru;
  GetNextNamespace,   // EFI_NVM_EXPRESS_PASS_THRU_GET_NEXT_NAMESPACE    GetNextNamespace;
  BuildDevicePath,    // EFI_NVM_EXPRESS_PASS_THRU_BUILD_DEVICE_PATH     BuildDevicePath;
  GetNamespace        // EFI_NVM_EXPRESS_PASS_THRU_GET_NAMESPACE         GetNamespace;
};

extern "C" {
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *gNvmePassThruProtocol = &NVME_PASSTHRU_PROTOCOL_INSTANCE;
}
