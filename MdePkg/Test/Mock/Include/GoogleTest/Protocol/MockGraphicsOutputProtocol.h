/** @file
  This file declares a mock of EFI_GRAPHICS_OUTPUT_PROTOCOL.

  Only QueryMode() and SetMode() are mocked so far -- extend with
  additional MOCK_FUNCTION_DECLARATION/DEFINITION pairs (and instance
  fields) as other consumers need more of this protocol's surface. Blt()
  is left NULL in the instance below since nothing under test calls it.
  Mode is a plain data pointer (not a method) -- callers populate their
  own EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE instance directly and assign it
  to gGraphicsOutputProtocol->Mode, no mocking needed for it.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/GraphicsOutput.h>
}

struct MockGraphicsOutputProtocol {
  MOCK_INTERFACE_DECLARATION (MockGraphicsOutputProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GopQueryMode,
    (IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
     IN  UINT32                                ModeNumber,
     OUT UINTN                                 *SizeOfInfo,
     OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GopSetMode,
    (IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
     IN  UINT32                       ModeNumber)
    );
};

extern "C" {
  extern EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGraphicsOutputProtocol;
}
