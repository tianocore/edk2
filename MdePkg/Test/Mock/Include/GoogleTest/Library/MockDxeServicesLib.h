/** @file MockDxeServicesLib.h
  Google Test mocks for DxeServicesLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DXE_SERVICES_LIB_H_
#define DXE_SERVICES_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <PiDxe.h>
}

struct MockDxeServicesLib {
  MOCK_INTERFACE_DECLARATION (MockDxeServicesLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetSectionFromAnyFvByFileType,
    (
     IN  EFI_FV_FILETYPE   FileType,
     IN  UINTN             FileInstance,
     IN  EFI_SECTION_TYPE  SectionType,
     IN  UINTN             SectionInstance,
     OUT VOID              **Buffer,
     OUT UINTN             *Size
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetSectionFromAnyFv,
    (
     IN  CONST EFI_GUID    *NameGuid,
     IN  EFI_SECTION_TYPE  SectionType,
     IN  UINTN             SectionInstance,
     OUT VOID              **Buffer,
     OUT UINTN             *Size
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetSectionFromFv,
    (
     IN  CONST EFI_GUID    *NameGuid,
     IN  EFI_SECTION_TYPE  SectionType,
     IN  UINTN             SectionInstance,
     OUT VOID              **Buffer,
     OUT UINTN             *Size
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetSectionFromFfs,
    (
     IN  EFI_SECTION_TYPE  SectionType,
     IN  UINTN             SectionInstance,
     OUT VOID              **Buffer,
     OUT UINTN             *Size
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    GetFileBufferByFilePath,
    (
     IN BOOLEAN                         BootPolicy,
     IN CONST EFI_DEVICE_PATH_PROTOCOL  *FilePath,
     OUT      UINTN                     *FileSize,
     OUT UINT32                         *AuthenticationStatus
    )
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetFileDevicePathFromAnyFv,
    (
     IN CONST  EFI_GUID                  *NameGuid,
     IN        EFI_SECTION_TYPE          SectionType,
     IN        UINTN                     SectionInstance,
     OUT       EFI_DEVICE_PATH_PROTOCOL  **FvFileDevicePath
    )
    );

  MOCK_FUNCTION_DECLARATION (
    VOID *,
    AllocatePeiAccessiblePages,
    (
     IN EFI_MEMORY_TYPE  MemoryType,
     IN UINTN            Pages
    )
    );
};

#endif
