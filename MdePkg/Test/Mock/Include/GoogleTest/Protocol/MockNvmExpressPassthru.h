/** @file MockNvmExpressPassthru.h
  This file declares a mock of NvmExpress Pass Thru Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/NvmExpressPassthru.h>
}

struct MockNvmePassThruProtocol {
  MOCK_INTERFACE_DECLARATION (MockNvmePassThruProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    PassThru,
    (
     IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
     IN     UINT32                                      NamespaceId,
     IN OUT EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    *Packet,
     IN     EFI_EVENT                                   Event OPTIONAL
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetNextNamespace,
    (
     IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
     IN OUT UINT32                                      *NamespaceId
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    BuildDevicePath,
    (
     IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
     IN     UINT32                                      NamespaceId,
     OUT    EFI_DEVICE_PATH_PROTOCOL                    **DevicePath
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetNamespace,
    (
     IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
     IN     EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
     OUT    UINT32                                      *NamespaceId
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockNvmePassThruProtocol);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, PassThru, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, GetNextNamespace, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, BuildDevicePath, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockNvmePassThruProtocol, GetNamespace, 3, EFIAPI);

static EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  NVME_PASSTHRU_PROTOCOL_INSTANCE = {
  { 0 },
  PassThru,
  GetNextNamespace,
  BuildDevicePath,
  GetNamespace
};

extern "C" {
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *gNvmePassThruProtocol = &NVME_PASSTHRU_PROTOCOL_INSTANCE;
}
