/** @file MockNvmExpressPassthru.h
  This file declares a mock of NvmExpress Pass Thru Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_NVM_EXPRESS_PASS_THRU_H_
#define MOCK_NVM_EXPRESS_PASS_THRU_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/NvmExpressPassthru.h>
}

struct MockEfiNvmExpressPassThruProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiNvmExpressPassThruProtocol);

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

MOCK_INTERFACE_DEFINITION (MockEfiNvmExpressPassThruProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiNvmExpressPassThruProtocol, PassThru, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiNvmExpressPassThruProtocol, GetNextNamespace, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiNvmExpressPassThruProtocol, BuildDevicePath, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiNvmExpressPassThruProtocol, GetNamespace, 3, EFIAPI);

#define MOCK_EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL_INSTANCE(NAME)  \
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL NAME##_INSTANCE = {        \
    { 0 },                                                      \
    PassThru,                                                   \
    GetNextNamespace,                                           \
    BuildDevicePath,                                            \
    GetNamespace };                                             \
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // NVM_EXPRESS_PASS_THRU_H_
