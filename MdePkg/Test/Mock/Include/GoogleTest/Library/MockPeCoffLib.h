/** @file
  Google Test mocks for PeCoffLib

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Library/PeCoffLib.h>
}

struct MockPeCoffLib {
  MOCK_INTERFACE_DECLARATION (MockPeCoffLib);

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PeCoffLoaderGetImageInfo,
    (IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PeCoffLoaderRelocateImage,
    (IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PeCoffLoaderLoadImage,
    (IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PeCoffLoaderImageReadFromMemory,
    (IN     VOID   *FileHandle,
     IN     UINTN  FileOffset,
     IN OUT UINTN  *ReadSize,
     OUT    VOID   *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    VOID,
    PeCoffLoaderRelocateImageForRuntime,
    (IN PHYSICAL_ADDRESS  ImageBase,
     IN PHYSICAL_ADDRESS  VirtImageBase,
     IN UINTN             ImageSize,
     IN VOID              *RelocationData)
    );

  MOCK_FUNCTION_DECLARATION (
    RETURN_STATUS,
    PeCoffLoaderUnloadImage,
    (IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext)
    );
};
