/** @file MockFirmwareVolume2.h
  This file declares a mock of Firmware Volume Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Pi/PiFirmwareVolume.h>
  #include <Pi/PiFirmwareFile.h>
  #include <Protocol/FirmwareVolume2.h>
}

struct MockFirmwareVolume2Protocol {
  MOCK_INTERFACE_DECLARATION (MockFirmwareVolume2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetVolumeAttributes,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     OUT       EFI_FV_ATTRIBUTES             *FvAttributes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetVolumeAttributes,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN OUT    EFI_FV_ATTRIBUTES             *FvAttributes
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FvReadFile,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *NameGuid,
     IN OUT    VOID                          **Buffer,
     IN OUT    UINTN                         *BufferSize,
     OUT       EFI_FV_FILETYPE               *FoundType,
     OUT       EFI_FV_FILE_ATTRIBUTES        *FileAttributes,
     OUT       UINT32                        *AuthenticationStatus
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReadSection,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *NameGuid,
     IN        EFI_SECTION_TYPE              SectionType,
     IN        UINTN                         SectionInstance,
     IN OUT    VOID                          **Buffer,
     IN OUT    UINTN                         *BufferSize,
     OUT       UINT32                        *AuthenticationStatus
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FvWriteFile,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN        UINT32                        NumberOfFiles,
     IN        EFI_FV_WRITE_POLICY           WritePolicy,
     IN        EFI_FV_WRITE_FILE_DATA        *FileData
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetNextFile,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN OUT    VOID                          *Key,
     IN OUT    EFI_FV_FILETYPE               *FileType,
     OUT       EFI_GUID                      *NameGuid,
     OUT       EFI_FV_FILE_ATTRIBUTES        *Attributes,
     OUT       UINTN                         *Size
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetInfo,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *InformationType,
     IN OUT    UINTN                         *BufferSize,
     OUT       VOID                          *Buffer
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetInfo,
    (
     IN CONST  EFI_FIRMWARE_VOLUME2_PROTOCOL *This,
     IN CONST  EFI_GUID                      *InformationType,
     IN        UINTN                         BufferSize,
     IN CONST  VOID                          *Buffer
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockFirmwareVolume2Protocol);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, GetVolumeAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, SetVolumeAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, FvReadFile, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, ReadSection, 7, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, FvWriteFile, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, GetNextFile, 6, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, GetInfo, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockFirmwareVolume2Protocol, SetInfo, 4, EFIAPI);

static EFI_FIRMWARE_VOLUME2_PROTOCOL  FIRMWARE_VOLUME2_PROTOCOL_MOCK = {
  GetVolumeAttributes,   // EFI_FV_GET_ATTRIBUTES    GetVolumeAttributes;
  SetVolumeAttributes,   // EFI_FV_SET_ATTRIBUTES    SetVolumeAttributes;
  FvReadFile,            // EFI_FV_READ_FILE         ReadFile;
  ReadSection,           // EFI_FV_READ_SECTION      ReadSection;
  FvWriteFile,           // EFI_FV_WRITE_FILE        WriteFile;
  GetNextFile,           // EFI_FV_GET_NEXT_FILE     GetNextFile;
  0,                     // UINT32                   KeySize;
  0,                     // EFI_HANDLE               ParentHandle;
  GetInfo,               // EFI_FV_GET_INFO          GetInfo;
  SetInfo                // EFI_FV_SET_INFO          SetInfo;
};

extern "C" {
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *gFirmwareVolume2Protocol = &FIRMWARE_VOLUME2_PROTOCOL_MOCK;
}
