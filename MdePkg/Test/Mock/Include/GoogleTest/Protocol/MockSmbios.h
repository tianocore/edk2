/** @file MockSmbios.h
  This file declares a mock of SMBIOS Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SMBIOS_PROTOCOL_H_
#define MOCK_SMBIOS_PROTOCOL_H_

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

EFI_SMBIOS_PROTOCOL  SMBIOS_PROTOCOL_INSTANCE = {
  Add,          // EFI_SMBIOS_ADD              Add;
  UpdateString, // EFI_SMBIOS_UPDATE_STRING    UpdateString;
  Remove,       // EFI_SMBIOS_REMOVE           Remove;
  GetNext,      // EFI_SMBIOS_GET_NEXT         GetNext;
  0,            // UINT8                       MajorVersion;
  0             // UINT8                       MinorVersion;
};//

extern "C" {
  EFI_SMBIOS_PROTOCOL  *gSmbiosProtocol = &SMBIOS_PROTOCOL_INSTANCE;
}

#endif // MOCK_SMBIOS_PROTOCOL_H_
