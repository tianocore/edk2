/** @file MockSmbios.h
  This file declares a mock of SMBIOS Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/Smbios.h>
}

struct MockSmbiosProtocol {
  MOCK_INTERFACE_DECLARATION (MockSmbiosProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Add,
    (
     IN CONST      EFI_SMBIOS_PROTOCOL     *This,
     IN            EFI_HANDLE              ProducerHandle OPTIONAL,
     IN OUT        EFI_SMBIOS_HANDLE       *SmbiosHandle,
     IN            EFI_SMBIOS_TABLE_HEADER *Record
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    UpdateString,
    (
     IN CONST EFI_SMBIOS_PROTOCOL *This,
     IN       EFI_SMBIOS_HANDLE   *SmbiosHandle,
     IN       UINTN               *StringNumber,
     IN       CHAR8               *String
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Remove,
    (
     IN CONST EFI_SMBIOS_PROTOCOL *This,
     IN       EFI_SMBIOS_HANDLE   SmbiosHandle
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetNext,
    (
     IN CONST EFI_SMBIOS_PROTOCOL     *This,
     IN OUT EFI_SMBIOS_HANDLE       *SmbiosHandle,
     IN EFI_SMBIOS_TYPE         *Type OPTIONAL,
     OUT EFI_SMBIOS_TABLE_HEADER **Record,
     OUT EFI_HANDLE              *ProducerHandle OPTIONAL
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockSmbiosProtocol);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, Add, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, UpdateString, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, Remove, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSmbiosProtocol, GetNext, 5, EFIAPI);

static EFI_SMBIOS_PROTOCOL  SMBIOS_PROTOCOL_INSTANCE = {
  Add,
  UpdateString,
  Remove,
  GetNext,
  0,
  0
};

extern "C" {
  EFI_SMBIOS_PROTOCOL  *gSmbiosProtocol = &SMBIOS_PROTOCOL_INSTANCE;
}
